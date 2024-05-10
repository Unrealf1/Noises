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
  return al_map_rgb(0, 0, 0);
  /*
  float kx = float(x) / float(width);
  float ky = float(y) / float(height);

  float topInterp[3] = {
    std::lerp(event.topLeft[0], event.topRight[0], kx),
    std::lerp(event.topLeft[1], event.topRight[1], kx),
    std::lerp(event.topLeft[2], event.topRight[2], kx)
  };
    
  float botInterp[3] = {
    std::lerp(event.botLeft[0], event.botRight[0], kx),
    std::lerp(event.botLeft[1], event.botRight[1], kx),
    std::lerp(event.botLeft[2], event.botRight[2], kx)
  };

  float result[3] = {
    std::lerp(topInterp[0], botInterp[0], ky),
    std::lerp(topInterp[1], botInterp[1], ky),
    std::lerp(topInterp[2], botInterp[2], ky)
  };
  return al_map_rgb_f(result[0], result[1], result[2]);*/
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

  const float* neighboor = left & top ? &event.colors[sectorLeftColumn + sectorTopRow * 4]
                   : left & !top ? &event.colors[sectorLeftColumn + (sectorTopRow + 1) * 4]
                   : !left & top ? &event.colors[sectorLeftColumn + 1 + sectorTopRow * 4]
                   : &event.colors[(sectorLeftColumn + 1) + (sectorTopRow + 1) * 4];

  return al_map_rgb_f(neighboor[0], neighboor[1], neighboor[2]);
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

