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

static ALLEGRO_COLOR operator-(ALLEGRO_COLOR first, ALLEGRO_COLOR second) {
  float c1[3];
  al_unmap_rgb_f(first, &c1[0], &c1[1], &c1[2]);

  float c2[3];
  al_unmap_rgb_f(second, &c2[0], &c2[1], &c2[2]);

  c1[0] -= c2[0];
  c1[1] -= c2[1];
  c1[2] -= c2[2];

  return al_map_rgb_f(c1[0], c1[1], c1[2]);
}

static ALLEGRO_COLOR operator-=(ALLEGRO_COLOR& first, ALLEGRO_COLOR second) {
  first = first - second;
  return first;
}

static ALLEGRO_COLOR operator+=(ALLEGRO_COLOR& first, ALLEGRO_COLOR second) {
  first = first + second;
  return first;
}

static ALLEGRO_COLOR operator*(float scalar, ALLEGRO_COLOR color) {
  float f[3];
  al_unmap_rgb_f(color, &f[0], &f[1], &f[2]);
  f[0] *= scalar;
  f[1] *= scalar;
  f[2] *= scalar;
  return al_map_rgb_f(f[0], f[1], f[2]);
}

static ALLEGRO_COLOR operator*(ALLEGRO_COLOR color, float scalar) {
  return scalar * color;
}

static ALLEGRO_COLOR operator/(ALLEGRO_COLOR color, float scalar) {
  return (1.0f / scalar) * color;
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
  
  // We want to interpolate with a cubic function p(x, y) of two variables
  // That equals to the original function f(x, y) in known points
  // And its derivatives are equal to the function derivatives in known points
  // Known values of f here are values of the pixels in the corners of the sector
  // Derivatives of f will be estimated by adjacent values. But in noise algorithms
  // can be acquired directly

  // For convenience, assume that top left sector corner is (0, 0)
  // then x0 = 0; x1 = sectorWidth = w
  //      y0 = 0; y1 = sectorHeight = h

  // General form of the interpolating functino is this
  // p(x, y) = a[i][j] * x^i * y^j for i in [0..3] for j in [0..3]
  // Now we need to calculate a[i][j]. For that we will use constraints defined above and calculated derivatives:

  // dp/dx(x, y) = a[i][j] * i * x^(i - 1) * y^j for i in [0..3] for j in [0..3]
  // dp/dy(x, y) = a[i][j] * x^i * j * y^(j - 1) for i in [0..3] for j in [0..3]
  // dp/dxy(x, y) = a[i][j] * i * x^(i - 1) * j * y^(j - 1) for i in [0..3] for j in [0..3]

  // + f(0, 0) = p(0, 0) = a[0][0] , as other summands are zero
  // + f(0, h) = p(0, h) = a[0][j] * 1 * h^j for j in [0..3]
  // + f(w, 0) = p(w, 0) = a[i][0] * w^i * 1 for i in [0..3]
  // f(w, h) = p(w, h) = a[i][j] * w^i * h^j for i in [0..3] for j in [0..3]

  // + dp/dx(0, 0) = a[1][0]
  // + dp/dx(0, h) = a[1][j] * h^j for j in [0..3]
  // + dp/dx(w, 0) = a[i][0] * i * w^(i - 1) for i in [1..3] , NOTE THE RANGE OF `i`!
  // dp/dx(w, h) = a[i][j] * i * w^(i - 1) * h^j for i in [1..3] for j in [0..3]

  // + dp/dy(0, 0) = a[0][1]
  // + dp/dy(0, h) = a[0][j] * j * h ^ (j - 1) for j in [1..3]
  // + dp/dy(w, 0) = a[i][1] * w ^ i for i in [0..3]
  // dp/dy(w, h) = a[i][j] * w ^ i * j * h^(j - 1) for i in [0..3] for j in [1..3]

  // + dp/dxdy(0, 0) = a[1][1]
  // + dp/dxdy(0, h) = a[1][j] * j * h^(j - 1) for j in [1..3]
  // + dp/dxy(w, 0) = a[i][1] * i * w^(i - 1) for i in [1..3]
  // dp/dxy(w, h) = a[i][j] * i * w^(i - 1) * j * h^(j - 1) for i in [1..3] for j in [1..3]

  // Now we have 16 linear equations with 16 unknown values, as everything but a[i][j] is already known
  // This is the solution:
  // a[0][0] = f(0, 0)
  // a[1][0] = df/dx(0, 0)
  // a[0][1] = df/dy(0, 0)
  // a[1][1] = df/dxy(0, 0)

  // a[0][2] = ( 3 * f(0, h) - 3 * a[0][0] - 2 * a[0][1] * h - h * df/dy(0, h) ) / (h^2)
  // a[0][3] = ( h * df/dy(0, h) + a[0][1] * h - 2 * f(0, h) + 2 * a[0][0] ) / (h^3)
  // a[2][0] = ( 3 * f(w, 0) - 3 * a[0][0] - 2 * a[1][0] * w - w * df/dx(w, 0) ) / (w^2)
  // a[3][0] = ( w * df/dx(w, 0) + a[1][0] * w - 2 * f(w, 0) + 2 * a[0][0] ) / (w^3)

  // a[1][2] = ( 3 * df/dx(0, h) - 3 * a[1][0] - 2 * h * a[1][1] - df/dxy(0, h) * h ) / (h^2)
  // a[1][3] = ( h * df/dxy(0, h) + a[1][1] * h - 2 * df/dx(0, h) + 2 * a[1][0] ) / (h^3)
  // a[2][1] = ( 3 * df/dy(w, 0) - 3 * a[0][1] - 2 * w * a[1][1] - df/dxy(w, 0) * w ) / (w^2)
  // a[3][1] = ( w * df/dxy(w, 0) + a[1][1] * w - 2 * df/dy(w, 0) + 2 * a[0][1] ) / (w^3)

  // a[2][2] = ?
  // a[2][3] = ?
  // a[3][2] = ?
  // a[3][3] = ?
  ALLEGRO_COLOR a[16];
  auto idx = [](int x, int y) { return x + y * 4; };
  auto A = [&a, &idx](int x, int y) -> ALLEGRO_COLOR& { return a[idx(x, y)]; };
  auto F = [&event, &idx, &sectorLeftColumn, &sectorTopRow](int x, int y) -> ALLEGRO_COLOR {
    const float* ptr = &(event.colors[idx(sectorLeftColumn + x, sectorTopRow + y) * 3]);
    return al_map_rgb_f(ptr[0], ptr[1], ptr[2]);
  };
  const auto zeroDerivative = al_map_rgb(0, 0, 0);
  auto dFx = [&F, &sectorWidth, &sectorLeftColumn, &zeroDerivative](int x, int y) {
    return sectorLeftColumn == 1 ? (1.0f / float(sectorWidth)) * (F(x + 1, y) - F(x - 1, y)) : zeroDerivative;
  };
  auto dFy = [&F, &sectorHeight, &sectorTopRow, &zeroDerivative](int x, int y) {
    return sectorTopRow == 1 ? (1.0f / float(sectorHeight)) * (F(x, y + 1) - F(x, y - 1)) : zeroDerivative;
  };
  auto dFxy = [&dFx, &sectorHeight, &sectorLeftColumn, &sectorTopRow, &zeroDerivative](int x, int y) {
    return sectorLeftColumn == 1 && sectorTopRow == 1
      ? (1.0f / float(sectorHeight)) * (dFx(x, y + 1) - dFx(x, y - 1))
      : zeroDerivative;
  };

  float w = float(sectorWidth);
  float w2 = w * w;
  float w3 = w2 * w;

  float h = float(sectorHeight);
  float h2 = h * h;
  float h3 = h2 * h;

  A(0, 0) = F(0, 0);
  A(1, 0) = dFx(0, 0);
  A(0, 1) = dFy(0, 0);
  A(1, 1) = dFxy(0, 0);

  A(0, 2) = ( 3 * F(0, 1) - 3 * A(0, 0) - 2 * A(0, 1) * h - h * dFy(0, 1) ) / h2;
  A(0, 3) = ( h * dFy(0, 1) + A(0, 1) * h - 2 * F(0, 1) + 2 * A(0, 0) ) / h3;
  A(2, 0) = ( 3 * F(1, 0) - 3 * A(0, 0) - 2 * A(1, 0) * w - w * dFx(1, 0) ) / w2;
  A(3, 0) = ( w * dFx(1, 0) + A(1, 0) * w - 2 * F(1, 0) + 2 * A(0, 0) ) / w3;

  A(1, 2) = ( 3 * dFx(0, 1) - 3 * A(1, 0) - 2 * h * A(1, 1) - dFxy(0, 1) * h ) / h2;
  A(1, 3) = ( h * dFxy(0, 1) + A(1, 1) * h - 2 * dFx(0, 1) + 2 * A(1, 0) ) / h3;
  A(2, 1) = ( 3 * dFy(1, 0) - 3 * A(0, 1) - 2 * w * A(1, 1) - dFxy(1, 0) * w ) / w2;
  A(3, 1) = ( w * dFxy(1, 0) + A(1, 1) * w - 2 * dFy(1, 0) + 2 * A(0, 1) ) / w3;

  ALLEGRO_COLOR C[4];
  float ws[] = {1.0f, w, w2, w3};
  float hs[] = {1.0f, h, h2, h3};
  C[0] = F(1, 1);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (i >= 2 && j >= 2) {
        continue;
      }
      C[0] -= A(i, j) * ws[i] * hs[j];
    }
  }

  C[1] = dFx(1, 1);
  for (int i = 1; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (i >= 2 && j >= 2) {
        continue;
      }
      C[1] -= float(i) * A(i, j) * ws[i - 1] * hs[j];
    }
  }

  C[2] = dFy(1, 1);
  for (int i = 0; i < 4; ++i) {
    for (int j = 1; j < 4; ++j) {
      if (i >= 2 && j >= 2) {
        continue;
      }
      C[2] -= float(j) * A(i, j) * ws[i] * hs[j - 1];
    }
  }

  C[3] = dFxy(1, 1);
  for (int i = 1; i < 4; ++i) {
    for (int j = 1; j < 4; ++j) {
      if (i >= 2 && j >= 2) {
        continue;
      }
      C[3] -= float(i) * float(j) * A(i, j) * ws[i - 1] * hs[j - 1];
    }
  }

  A(3, 2) = ( 2 * C[2] * h + 3 * C[1] * w - C[3] * w * h - 6 * C[0] ) / ( 6 * w3 * h3 );
  A(3, 3) = ( C[1] * w - 2 * C[0] - A(3, 2) * w3 * h2 ) / (w3 * h3);
  A(2, 3) = ( 4 * A(3, 2) * w3 * h2 + C[3] * w * h + 6 * C[0] - 5 * C[1] * w ) / ( 2 * w2 * h3);
  A(2, 2) = ( 3 * C[1] * w - C[3] * w * h - 4 * A(3, 2) * w3 * h2 ) / (2 * w2 * h2);

  ALLEGRO_COLOR result = al_map_rgb(0, 0, 0);
  float dxs[4] = { 1.0f, dx, dx * dx, dx * dx * dx };
  float dys[4] = { 1.0f, dy, dy * dy, dy * dy * dy };
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result += A(i, j) * dxs[i] * dys[j];
    }
  }
  return result;
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

