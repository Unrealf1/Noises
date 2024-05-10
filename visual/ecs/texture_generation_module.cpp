#include "texture_generation_module.hpp"

#include <gui/menu.hpp>
#include <render/noise_texture.hpp>
#include <perlin.hpp>
#include <random>
#include <ctime>


static flecs::entity s_menu_event_receiver;


static void generate_perlin_noise_texture(flecs::world& ecs, const Menu::EventGeneratePerlinNoiseTexture& event) {
  ecs.each([](flecs::entity entity, const NoiseTexture&){
    entity.destruct();
  });
  NoiseTexture texture(event.size[0], event.size[1]);

  std::random_device dev{};
  std::default_random_engine eng(dev());

  auto startTime = std::clock();

  PerlinNoise noise({
    .grid_size_x = event.grid_size[0],
    .grid_size_y = event.grid_size[1],

    .grid_step_x = event.grid_step[0],
    .grid_step_y = event.grid_step[1],

    .offset_x = event.offset[0],
    .offset_y = event.offset[1],

    .normalize_offsets = event.normalize_offsets,
    .interpolation_algorithm = event.interpolation_algorithm
  }, eng);

  auto width = texture.width();
  auto height = texture.height();
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      float value = noise(float(x), float(y));
      auto brightness = uint8_t(255.0f * value);
      texture.set(x, y, al_map_rgb(brightness, brightness, brightness));
    }
  }
  auto timeTaken = std::clock() - startTime;
  ecs.each([&ecs, &timeTaken](flecs::entity entity, Menu::EventReceiver){
    ecs.event<Menu::EventGenerationFinished>()
      .ctx(Menu::EventGenerationFinished{.secondsTaken = double(timeTaken) / double(CLOCKS_PER_SEC)})
      .entity(entity)
      .emit();
  });

  ecs.entity().emplace<NoiseTexture>(std::move(texture));
}

static void generate_white_noise_texture(flecs::world& ecs, const Menu::EventGenerateWhiteNoiseTexture& event) {
  ecs.each([](flecs::entity entity, const NoiseTexture&){
    entity.destruct();
  });
  NoiseTexture texture(event.size[0], event.size[1]);

  std::random_device dev{};
  std::default_random_engine eng(dev());

  std::bernoulli_distribution distr(event.black_prob);

  auto startTime = std::clock();

  auto width = texture.width();
  auto height = texture.height();
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      uint8_t r = distr(eng) ? 0 : 255;
      uint8_t g = distr(eng) ? 0 : 255;
      uint8_t b = distr(eng) ? 0 : 255;
      texture.set(x, y, al_map_rgb(r, g, b));
    }
  }
  auto timeTaken = std::clock() - startTime;
  ecs.each([&ecs, &timeTaken](flecs::entity entity, Menu::EventReceiver){
    ecs.event<Menu::EventGenerationFinished>()
      .ctx(Menu::EventGenerationFinished{.secondsTaken = double(timeTaken) / double(CLOCKS_PER_SEC)})
      .entity(entity)
      .emit();
  });
  ecs.entity().emplace<NoiseTexture>(std::move(texture));
}

static ALLEGRO_COLOR bilinear_interpolation(int x, int y, int width, int height, const Menu::EventGenerateInterpolatedTexture& event) {
  auto sectorWidth = width / 3;
  auto sectorHeight = height / 3;

  auto sectorLeftColumn = x / sectorWidth;
  auto sectorLeft = sectorLeftColumn * sectorWidth;
  auto dx = x - sectorLeft;

  auto sectorTopRow = y / sectorHeight;
  auto sectorTop = sectorTopRow * sectorHeight;
  auto dy = y - sectorTop;
 
  float kx = float(dx) / float(sectorWidth);
  float ky = float(dy) / float(sectorHeight);

  int left = sectorLeftColumn;
  int right = sectorLeftColumn + 1;
  int top = sectorTopRow;
  int bot = sectorTopRow + 1;
  if (left < 0 || right >= 4 || top < 0 || bot >= 4) {
    return al_map_rgb(255, 255, 255);
  }

  auto index = [](int x, int y) {
    return (x + y * 4) * 3;
  };

  int topLeft = index(left, top);
  int topRight = index(right, top);

  int botLeft = index(left, bot);
  int botRight = index(right, bot);

  float topInterp[3] = {
    std::lerp(event.colors[topLeft], event.colors[topRight], kx),
    std::lerp(event.colors[topLeft + 1], event.colors[topRight + 1], kx),
    std::lerp(event.colors[topLeft + 2], event.colors[topRight + 2], kx),
  };

  float botInterp[3] = {
    std::lerp(event.colors[botLeft], event.colors[botRight], kx),
    std::lerp(event.colors[botLeft + 1], event.colors[botRight + 1], kx),
    std::lerp(event.colors[botLeft + 2], event.colors[botRight + 2], kx),
  };

  float result[3] = {
    std::lerp(topInterp[0], botInterp[0], ky),
    std::lerp(topInterp[1], botInterp[1], ky),
    std::lerp(topInterp[2], botInterp[2], ky)
  };
  return al_map_rgb_f(result[0], result[1], result[2]);
}

static ALLEGRO_COLOR nearest_neighboor(int x, int y, int width, int height, const Menu::EventGenerateInterpolatedTexture& event) {
  auto sectorWidth = width / 3;
  auto sectorHeight = height / 3;

  auto sectorLeftColumn = x / sectorWidth;
  auto sectorLeft = sectorLeftColumn * sectorWidth;
  auto dx = x - sectorLeft;

  auto sectorTopRow = y / sectorHeight;
  auto sectorTop = sectorTopRow * sectorHeight;
  auto dy = y - sectorTop;

  bool left = dx <= sectorWidth / 2;
  bool top = dy <= sectorHeight / 2;

  int neighboorX = left ? sectorLeftColumn : sectorLeftColumn + 1;
  int neighboorY = top ? sectorTopRow : sectorTopRow + 1;

  const float* neighboor = &event.colors[(neighboorX + neighboorY * 4) * 3];

  return al_map_rgb_f(neighboor[0], neighboor[1], neighboor[2]);
}

static ALLEGRO_COLOR operator+(ALLEGRO_COLOR first, ALLEGRO_COLOR second) {
  float c1[3];
  al_unmap_rgb_f(first, &c1[0], &c1[1], &c1[2]);

  float c2[3];
  al_unmap_rgb_f(second, &c2[0], &c2[1], &c2[2]);

  c1[0] += c2[0];
  c1[1] += c2[1];
  c1[2] += c2[2];

  return al_map_rgb_f(c1[0], c1[1], c1[2]);
}

static ALLEGRO_COLOR operator*(float scalar, ALLEGRO_COLOR color) {
  float f[3];
  al_unmap_rgb_f(color, &f[0], &f[1], &f[2]);
  f[0] *= scalar;
  f[1] *= scalar;
  f[2] *= scalar;
  return al_map_rgb_f(f[0], f[1], f[2]);
}

static ALLEGRO_COLOR bicubic(int x, int y, int width, int height, const Menu::EventGenerateInterpolatedTexture& event) {
  auto sectorWidth = width / 3;
  auto sectorHeight = height / 3;

  auto sectorLeftColumn = x / sectorWidth;
  auto sectorLeft = sectorLeftColumn * sectorWidth;
  auto dx = x - sectorLeft;

  auto sectorTopRow = y / sectorHeight;
  auto sectorTop = sectorTopRow * sectorHeight;
  auto dy = y - sectorTop;
  
  if (sectorLeftColumn != 1 || sectorTopRow != 1) {
    return al_map_rgb(255, 255, 255);
  }

  float dxn = float(dx) / float(sectorWidth);
  float dyn = float(dy) / float(sectorHeight);

  float x0 = dxn - 2.0f;
  float x1 = dxn - 1.0f;
  float x2 = dxn;
  float x3 = dxn + 1.0f;

  float y0 = dyn - 2.0f;
  float y1 = dyn - 1.0f;
  float y2 = dyn;
  float y3 = dyn + 1.0f;

  // formula taken from wikipedia :(
  float b[16] = {
    0.25f * (x1) * (x0) * (x3) * (y1) * (y0) * (y3),        // 1
    - 0.25f * x2 * (x3) * (x0) * (y1) * (y0) * (y3),            // 2
    - 0.25f * y2 * (x1) * (x0) * (x3) * (y3) * (y0),            // 3
    0.25f * x2 * y2 * (x3) * (x0) * (y3) * (y0),                     // 4
    - 1.0f / 12.0f * x2 * (x1) * (x0) * (y1) * (y0) * (y3),     // 5
    - 1.0f / 12.0f * y2 * (x1) * (x0) * (x3) * (y1) * (y0),     // 6
    1.0f / 12.0f * x2 * y2 * (x1) * (x0) * (y3) * (y0),             // 7
    1.0f / 12.0f * x2 * y2 * (x3) * (x0) * (y1) * (y0),             // 8
    1.0f / 12.0f * x2 * (x1) * (x3) * (y1) * (y0) * (y3),       // 9
    1.0f / 12.0f * y2 * (x1) * (x0) * (x3) * (y1) * (y3),       // 10
    1.0f / 36.0f * x2 * y2 * (x1) * (x0) * (y1) * (y0),             // 11
    - 1.0f / 12.0f * x2 * y2 * (x1) * (x3) * (y3) * (y0),           // 12
    - 1.0f / 12.0f * x2 * y2 * (x3) * (x0) * (y1) * (y3),           // 13
    - 1.0f / 36.0f * x2 * y2 * (x1) * (x3) * (y1) * (y0),           // 14
    - 1.0f / 36.0f * x2 * y2 * (x1) * (x0) * (y1) * (y3),           // 15
    1.0f / 36.0f * x2 * y2 * (x1) * (x3) * (y1) * (y3)              // 16
  };

  auto v = [&event](int x, int y) {
    const float* floats = &event.colors[(x + y * 4) * 3];
    return al_map_rgb_f(floats[0], floats[1], floats[2]);
  };

  return b[0] * v(1, 1)
    + b[1]  * v(1, 2)
    + b[2]  * v(2, 1)
    + b[3]  * v(2, 2)
    + b[4]  * v(1, 0)
    + b[5]  * v(0, 1)
    + b[6]  * v(2, 0)
    + b[7]  * v(0, 2)
    + b[8]  * v(1, 3)
    + b[9]  * v(3, 1)
    + b[10] * v(0, 0)
    + b[11] * v(2, 3)
    + b[12] * v(3, 2)
    + b[13] * v(0, 3)
    + b[14] * v(3, 0)
    + b[15] * v(3, 3);
}

static void generate_interpolated_texture(flecs::world& ecs, const Menu::EventGenerateInterpolatedTexture& event) {
  ecs.each([](flecs::entity entity, const NoiseTexture&){
    entity.destruct();
  });
  NoiseTexture texture(event.size[0], event.size[1]);
  // Texture has 16 points. Will interpolate between them

  auto width = texture.width();
  auto height = texture.height();

  auto startTime = std::clock();

  ALLEGRO_COLOR (*interpolator) (int, int, int, int, const Menu::EventGenerateInterpolatedTexture&);
  if (event.algorithm == Menu::EventGenerateInterpolatedTexture::Algorithm::bilinear) {
    interpolator = bilinear_interpolation;
  } else if (event.algorithm == Menu::EventGenerateInterpolatedTexture::Algorithm::nearest_neighboor) {
    interpolator = nearest_neighboor;
  } else if (event.algorithm == Menu::EventGenerateInterpolatedTexture::Algorithm::bicubic) {
    interpolator = bicubic;
  }

  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      texture.set(x, y, interpolator(x, y, width, height, event));
    }
  }
  auto timeTaken = std::clock() - startTime;
  ecs.each([&ecs, &timeTaken](flecs::entity entity, Menu::EventReceiver){
    ecs.event<Menu::EventGenerationFinished>()
      .ctx(Menu::EventGenerationFinished{.secondsTaken = double(timeTaken) / double(CLOCKS_PER_SEC)})
      .entity(entity)
      .emit();
  });
  ecs.entity().emplace<NoiseTexture>(std::move(texture));
  
}

TextureGenerationModule::TextureGenerationModule(flecs::world& ecs) {
  ecs.module<TextureGenerationModule>();   

  ecs.each([this](flecs::entity receiver, Menu::EventReceiver){
    m_menu_event_receiver = receiver;
  });
 
  m_menu_event_receiver
    .observe([&ecs](const Menu::EventGenerateWhiteNoiseTexture& event){
      generate_white_noise_texture(ecs, event);
    });

  m_menu_event_receiver
    .observe([&ecs](const Menu::EventGeneratePerlinNoiseTexture& event){
      generate_perlin_noise_texture(ecs, event);
    });

  m_menu_event_receiver
    .observe([&ecs](const Menu::EventGenerateInterpolatedTexture& event) {
      generate_interpolated_texture(ecs, event);    
    });
}

