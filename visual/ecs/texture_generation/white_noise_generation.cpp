#include "white_noise_generation.hpp"

#include <render/noise_texture.hpp>
#include <render/drawable_bitmap.hpp>
#include <chrono>

using namespace std::chrono_literals;
using real_clock_t = std::chrono::steady_clock;


void generate_white_noise_texture(flecs::world& ecs, const Menu::EventGenerateWhiteNoiseTexture& event) {
  NoiseTexture texture(event.size[0], event.size[1]);

  std::random_device dev{};
  std::default_random_engine eng(dev());

  std::bernoulli_distribution distr(event.black_prob);

  auto startTime = std::clock();

  auto width = texture.width();
  auto height = texture.height();
  texture.mark_modified();

  {
  auto bitmapOverride = texture.scoped_write_to_memory_bitmap();
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      uint8_t r = distr(eng) ? 0 : 255;
      uint8_t g = distr(eng) ? 0 : 255;
      uint8_t b = distr(eng) ? 0 : 255;
      texture.set(x, y, al_map_rgb(r, g, b));
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
    .emplace<DrawableBitmap>(
      Bitmap(event.size[0], event.size[1]),
      vec2{0.0f, 0.0f}
     );
}

