#include "texture_generation_module.hpp"

#include <gui/menu.hpp>
#include <render/noise_texture.hpp>
#include <math.hpp>
#include <perlin.hpp>
#include <interpolation.hpp>
#include <random>
#include <ctime>


static flecs::entity s_menu_event_receiver;


static void generate_perlin_noise_texture(flecs::world& ecs, const Menu::EventGeneratePerlinNoiseTexture& event) {
  ecs.each([](flecs::entity entity, const NoiseTexture&){
    entity.destruct();
  });
  NoiseTexture texture(event.size[0], event.size[1]);

  auto seed = [&]{
    if (event.random_seed <= 0) {
      std::random_device dev{};
      return dev();
    } else {
      return static_cast<unsigned int>(event.random_seed);
    }
  }();
  std::default_random_engine eng(seed);

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

struct SectorCoords {
  int width;
  int height;

  int leftColumn;
  int topRow;

  int dx;
  int dy;
};

static SectorCoords calc_sector_coords(int x, int y, int width, int height) {
  auto sectorWidth = width / 3;
  auto sectorHeight = height / 3;

  auto sectorLeftColumn = x / sectorWidth;
  auto sectorLeft = sectorLeftColumn * sectorWidth;
  auto dx = x - sectorLeft;

  auto sectorTopRow = y / sectorHeight;
  auto sectorTop = sectorTopRow * sectorHeight;
  auto dy = y - sectorTop;

  return {
    .width = sectorWidth,
    .height = sectorHeight,
    .leftColumn = sectorLeftColumn,
    .topRow = sectorTopRow,
    .dx = dx,
    .dy = dy
  };
}

static ALLEGRO_COLOR bilinear_interpolation(int x, int y, int width, int height, const Menu::EventGenerateInterpolatedTexture& event) {
  auto [sectorWidth, sectorHeight, sectorLeftColumn, sectorTopRow, dx, dy] = calc_sector_coords(x, y, width, height);
 
  float kx = float(dx) / float(sectorWidth);
  float ky = float(dy) / float(sectorHeight);

  int left = sectorLeftColumn;
  int right = std::min(sectorLeftColumn + 1, 3);
  int top = sectorTopRow;
  int bot = std::min(sectorTopRow + 1, 3);

  auto index = [](int x, int y) {
    return (x + y * 4) * 3;
  };

  int topLeft = index(left, top);
  int topRight = index(right, top);

  int botLeft = index(left, bot);
  int botRight = index(right, bot);

  vec3 topLeftV{ event.colors[topLeft], event.colors[topLeft + 1], event.colors[topLeft + 2] };
  vec3 topRightV{ event.colors[topRight], event.colors[topRight + 1], event.colors[topRight + 2] };
  vec3 botLeftV{ event.colors[botLeft], event.colors[botLeft + 1], event.colors[botLeft + 2] };
  vec3 botRightV{ event.colors[botRight], event.colors[botRight + 1], event.colors[botRight + 2] };

  vec3 result = interpolation::bilinear(topLeftV, topRightV, botLeftV, botRightV, kx, ky);

  return al_map_rgb_f(result.x, result.y, result.z);
}

static ALLEGRO_COLOR nearest_neighboor(int x, int y, int width, int height, const Menu::EventGenerateInterpolatedTexture& event) {
  auto [sectorWidth, sectorHeight, sectorLeftColumn, sectorTopRow, dx, dy] = calc_sector_coords(x, y, width, height);

  bool left = dx <= sectorWidth / 2;
  bool top = dy <= sectorHeight / 2;

  int neighboorX = left ? sectorLeftColumn : sectorLeftColumn + 1;
  int neighboorY = top ? sectorTopRow : sectorTopRow + 1;

  const float* neighboor = &event.colors[(neighboorX + neighboorY * 4) * 3];

  return al_map_rgb_f(neighboor[0], neighboor[1], neighboor[2]);
}

static interpolation::BicubicCoefficients<vec3> calc_bicubic_coefficients(int left_column, int top_row, int sector_width, int sector_height, const Menu::EventGenerateInterpolatedTexture& event) {
  auto idx = [](int x, int y) { return x + y * 4; };
  auto F = [&event, &idx, &left_column, &top_row](int x, int y) -> vec3 {
    const float* ptr = &(event.colors[idx(left_column + x, top_row + y) * 3]);
    return vec3(ptr[0], ptr[1], ptr[2]);
  };

  const auto zeroDerivative = vec3(0, 0, 0);
  auto dFx = [&F, &sector_width, &left_column, &zeroDerivative](int x, int y) {
    return left_column == 1 ? (1.0f / float(sector_width)) * (F(x + 1, y) - F(x - 1, y)) : zeroDerivative;
  };
  auto dFy = [&F, &sector_height, &top_row, &zeroDerivative](int x, int y) {
    return top_row == 1 ? (1.0f / float(sector_height)) * (F(x, y + 1) - F(x, y - 1)) : zeroDerivative;
  };
  auto dFxy = [&dFx, &sector_height, &left_column, &top_row, &zeroDerivative](int x, int y) {
    return left_column == 1 && top_row == 1
      ? (1.0f / float(sector_height)) * (dFx(x, y + 1) - dFx(x, y - 1))
      : zeroDerivative;
  };

  return interpolation::calc_bicubic_coefficients(F(0, 0), F(1, 0), F(0, 1), F(1, 1),
                                                  dFx(0, 0), dFx(1, 0), dFx(0, 1), dFx(1, 1),
                                                  dFy(0, 0), dFy(1, 0), dFy(0, 1), dFy(1, 1),
                                                  dFxy(0, 0), dFxy(1, 0), dFxy(0, 1), dFxy(1, 1));
}

static interpolation::BicubicCoefficients<vec3> s_bicubic_coefs[9];

static ALLEGRO_COLOR bicubic_wiki(int x, int y, int width, int height, const Menu::EventGenerateInterpolatedTexture&) {
  auto [sectorWidth, sectorHeight, sectorLeftColumn, sectorTopRow, dx, dy] = calc_sector_coords(x, y, width, height);

  if (sectorTopRow >= 3) {
    sectorTopRow = 2;
    dy = height - 1;
  }

  if (sectorLeftColumn >= 3) {
    sectorLeftColumn = 2;
    dx = width - 1;
  }

  const auto& coefs = s_bicubic_coefs[sectorLeftColumn + sectorTopRow * 3];

  const float w = float(sectorWidth);
  const float h = float(sectorHeight);

  const float ndx = float(dx) / w;
  const float ndy = float(dy) / h;

  vec3 wikiRes = interpolation::bicubic(ndx, ndy, coefs);

  return al_map_rgb_f(wikiRes.x, wikiRes.y, wikiRes.z);
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
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        s_bicubic_coefs[i + j * 3] = calc_bicubic_coefficients(i, j, width / 3, height / 3, event);
      }
    }
    interpolator = bicubic_wiki;
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

