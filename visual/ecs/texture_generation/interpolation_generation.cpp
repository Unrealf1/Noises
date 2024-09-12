#include "interpolation_generation.hpp"

#include <allegro_util.hpp>
#include <interpolation.hpp>
#include <render/noise_texture.hpp>
#include <render/drawable_bitmap.hpp>
#include <utility>
#include <chrono>
#include <log.hpp>


using namespace std::chrono_literals;
using real_clock_t = std::chrono::steady_clock;

struct TrueInterpolationPixelVisualization : public Tag {};
struct InterpolatedTextureInfo : public Menu::EventGenerateInterpolatedTexture {};


static void clear_true_pixels(flecs::world& ecs) {
  ecs.each([](flecs::entity eid, TrueInterpolationPixelVisualization){
    eid.destruct();
  });
}

void init_interpolated_generation_systems(flecs::world& ecs) {
  ecs.observer<Menu::EventReceiver>()
    .event<Menu::EventShowInterpTruePixels>()
    .each([](flecs::iter& it, size_t, Menu::EventReceiver){
      auto world = it.world();
      world.each([&world](const InterpolatedTextureInfo& info, const DrawableBitmap& noiseBitmap){
        clear_true_pixels(world);
        const int iTexSize = std::max(std::min(info.size[0], info.size[1]) / 10, 3);
        const float texSize = static_cast<float>(iTexSize);
        const float borderSize = std::max(texSize / 10.0f, 1.0f);
        const vec2 offset = noiseBitmap.center - vec2(ivec2{info.size[0], info.size[1]}) / 2.0f;
        for (int i = 0; i < 4; ++i) {
          for (int j = 0; j < 4; ++j) {
            const float* color = &info.colors[(i + j * 4) * 3];
            vec2 center = offset + vec2(ivec2{ i * info.size[0] / 3, j * info.size[1] / 3 });
            Bitmap bitmap(iTexSize, iTexSize);

            // This is needed, so texture is drawn correctly in webgl environment.
            // Not sure of exact reasons, but probably something to do with FBOs
            // https://www.allegro.cc/manual/5/al_set_target_bitmap
            al_lock_bitmap(bitmap.get_raw(), ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
            TargetBitmapOverride targetOverride(bitmap.get_raw());

            al_draw_filled_rectangle(0.0f, 0.0f, texSize, texSize, al_map_rgb_f(color[0], color[1], color[2]));
            al_draw_rectangle(0.0f, 0.0f, texSize, texSize, al_map_rgb(0, 0, 0), borderSize);

            al_unlock_bitmap(bitmap.get_raw());
            world.entity()
              .emplace<DrawableBitmap>(std::move(bitmap), center)
              .add<TrueInterpolationPixelVisualization>();
          }
        }
      });
  });

  ecs.observer<Menu::EventReceiver>()
    .event<Menu::EventHideInterpTruePixels>()
    .event<Menu::EventGenerateWhiteNoiseTexture>()
    .event<Menu::EventGeneratePerlinNoiseTexture>()
    .each([](flecs::iter& it, size_t, Menu::EventReceiver){
      auto world = it.world();
      clear_true_pixels(world);
    });
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

void generate_interpolated_texture(flecs::world& ecs, const Menu::EventGenerateInterpolatedTexture& event) {
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
  } else {
    std::unreachable();
  }

  texture.mark_modified();
  {
  auto bitmapOverride = texture.scoped_write_to_memory_bitmap();
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      texture.set(x, y, interpolator(x, y, width, height, event));
    }
  }
  } // end of bitmap override scope
  auto timeTaken = std::clock() - startTime;
  ecs.each([&ecs, &timeTaken](flecs::entity entity, Menu::EventReceiver){
    ecs.event<Menu::EventGenerationFinished>()
      .ctx(Menu::EventGenerationFinished{
        .secondsTaken = double(timeTaken) / double(CLOCKS_PER_SEC), 
        .realDuration = 0us
      })
      .entity(entity)
      .emit();
  });

  ecs.entity()
    .emplace<NoiseTexture>(std::move(texture))
    .emplace<InterpolatedTextureInfo>(event)
    .emplace<DrawableBitmap>(
      Bitmap(event.size[0], event.size[1]),
      vec2{0.0f, 0.0f}
     );
}

