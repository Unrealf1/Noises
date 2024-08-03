#include "texture_generation_module.hpp"

#include <gui/menu.hpp>
#include <render/noise_texture.hpp>
#include <ecs/render_module.hpp>
#include <render/drawable_bitmap.hpp>
#include <math.hpp>
#include <perlin.hpp>
#include <interpolation.hpp>
#include <random>
#include <ctime>
#include <utility>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <log.hpp>

using namespace std::chrono_literals;
using real_clock_t = std::chrono::steady_clock;


static flecs::entity s_menu_event_receiver;

static constexpr int s_num_threads = 4;

struct PerlinNoisePerThreadInfo {
  flecs::entity m_texture;
  int m_next_x;
  int m_until_x;
  const PerlinNoise* m_noise;
  std::atomic<size_t>* m_threads_finished;
  std::atomic<bool>* m_need_abort;
};

struct PerlinNoiseGenerationContinuation {
  std::clock_t m_time_spent;
  real_clock_t::duration m_real_time_spent;
  std::unique_ptr<PerlinNoise> m_noise;
  std::unique_ptr<std::atomic<size_t>> m_threads_finished;
  std::unique_ptr<std::atomic<bool>> m_need_abort;
  std::vector<std::thread> m_additional_threads;
  PerlinNoisePerThreadInfo m_main_thread_info;
  int m_columns_per_thread;
  int m_texture_width;
};

static void do_perlin_generation(PerlinNoisePerThreadInfo& info, auto continueCallback) {
  NoiseTexture* ptr = info.m_texture.get_mut<NoiseTexture>();
  if (ptr == nullptr) {
    return;
  }
  NoiseTexture& texture = *ptr;
  std::shared_lock lock(*texture.m_memory_bitmap_mutex);
  auto height = texture.height();

  auto bitmapOverride = texture.scoped_write_to_memory_bitmap();
  for (int& x = info.m_next_x; x < info.m_until_x; ++x) {
    for (int y = 0; y < height; ++y) {
      float value = (*info.m_noise)(float(x), float(y));
      auto brightness = uint8_t(255.0f * value);
      texture.set(x, y, al_map_rgb(brightness, brightness, brightness));
    }

    if (!continueCallback()) {
      return;
    }
  }
}

static void additional_thread_perlin_func(PerlinNoisePerThreadInfo thread_info) {
  while (thread_info.m_next_x < thread_info.m_until_x) {
    auto* texture = thread_info.m_texture.get<NoiseTexture>();
    texture->m_prepearing_for_draw->wait(true);

    bool needAbort = false;
    do_perlin_generation(thread_info, [&] {
      needAbort = thread_info.m_need_abort->load();
      return !needAbort && !(texture->m_prepearing_for_draw->load());
    });
    if (needAbort) {
      info("thread aborted ({}/{})", thread_info.m_next_x, thread_info.m_until_x);
      return;
    }
    info("thread pausing ({}/{})", thread_info.m_next_x, thread_info.m_until_x);
  }
  info("thread finished work (until {})", thread_info.m_until_x);
  thread_info.m_threads_finished->fetch_add(1);
}

static int calc_perlin_thread_finish(int thread_idx, int columns_per_thread, int width) {
  return thread_idx == s_num_threads - 1 ? width : (thread_idx + 1) * columns_per_thread;
}

static void init_perlin_noise_generation_threads(PerlinNoiseGenerationContinuation& continuation) {
  continuation.m_additional_threads.reserve(s_num_threads - 1);
  info("initializing threads. {} {}", continuation.m_columns_per_thread, continuation.m_texture_width);
  for (int i = 1; i < s_num_threads; ++i) { // first thread is the main thread
    PerlinNoisePerThreadInfo newThreadInfo {
      .m_texture = continuation.m_main_thread_info.m_texture,
      .m_next_x = i * continuation.m_columns_per_thread,
      .m_until_x = calc_perlin_thread_finish(i, continuation.m_columns_per_thread, continuation.m_texture_width),
      .m_noise = continuation.m_noise.get(),
      .m_threads_finished = continuation.m_threads_finished.get(),
      .m_need_abort = continuation.m_need_abort.get()
    };
    info("creating thread {}. Will work from {} to {}", i, newThreadInfo.m_next_x, newThreadInfo.m_until_x);
    continuation.m_additional_threads.emplace_back(additional_thread_perlin_func, newThreadInfo);
  }
}

static bool continue_perlin_noise_generation(PerlinNoiseGenerationContinuation& continuation, std::chrono::milliseconds time_budget) {
  auto startTime = std::clock(); // processor time
  auto startRealTime = real_clock_t::now();

  NoiseTexture& texture = *continuation.m_main_thread_info.m_texture.get_mut<NoiseTexture>();
  texture.mark_modified();
  
  if (continuation.m_main_thread_info.m_next_x == continuation.m_main_thread_info.m_until_x) {
    // main thread finished
    std::this_thread::sleep_for(time_budget);
  } else {
    do_perlin_generation(continuation.m_main_thread_info, [&]{ 
      auto curRealTimeSpent = real_clock_t::now() - startRealTime;
      return curRealTimeSpent < time_budget;
    });
  }

  info("pausing generation. Spent {}ms. {} threads finished, main finished - {}",
    std::chrono::duration_cast<std::chrono::milliseconds>(real_clock_t::now() - startRealTime).count(),
    continuation.m_threads_finished->load(),
    continuation.m_main_thread_info.m_next_x == continuation.m_main_thread_info.m_until_x);

  continuation.m_time_spent += std::clock() - startTime;
  continuation.m_real_time_spent += real_clock_t::now() - startRealTime;
  
  auto allThreadsFinished = continuation.m_threads_finished->load() == continuation.m_additional_threads.size()
                            && continuation.m_main_thread_info.m_next_x == continuation.m_main_thread_info.m_until_x;
  if (allThreadsFinished) {
    for (auto& t : continuation.m_additional_threads) {
      t.join();
    }
  }

  return allThreadsFinished;
}

static void clear_previous_texture(flecs::world& ecs) {
  ecs.each([](flecs::entity entity, PerlinNoiseGenerationContinuation& continuation){
    continuation.m_need_abort->store(true);
    for (auto& thread : continuation.m_additional_threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    entity.destruct();
  });
  ecs.each([](flecs::entity entity, const NoiseTexture&){
    entity.destruct();
  });
}


static void generate_perlin_noise_texture(flecs::world& ecs, const Menu::EventGeneratePerlinNoiseTexture& event) {
  clear_previous_texture(ecs);
  auto textureEntity = ecs.entity()
    .emplace<NoiseTexture>(event.size[0], event.size[1])
    .emplace<DrawableBitmap>(
      Bitmap(event.size[0], event.size[1]),
      vec2{0.0f, 0.0f}
     );

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
  auto realStartTime = real_clock_t::now();

  PerlinNoiseGenerationContinuation continuation{
    .m_time_spent = 0,
    .m_real_time_spent = {},
    .m_noise = std::make_unique<PerlinNoise>(PerlinNoiseParameters{
      .grid_size_x = event.grid_size[0],
      .grid_size_y = event.grid_size[1],

      .grid_step_x = event.grid_step[0],
      .grid_step_y = event.grid_step[1],

      .offset_x = event.offset[0],
      .offset_y = event.offset[1],

      .normalize_offsets = event.normalize_offsets,
      .interpolation_algorithm = event.interpolation_algorithm
    }, eng),
    .m_threads_finished = std::make_unique<std::atomic<size_t>>(0),
    .m_need_abort = std::make_unique<std::atomic<bool>>(false),
    .m_additional_threads = {},
    .m_main_thread_info = {
      .m_texture = textureEntity,
      .m_next_x = 0,
      .m_until_x = 0,
      .m_noise = continuation.m_noise.get(),
      .m_threads_finished = continuation.m_threads_finished.get(),
      .m_need_abort = continuation.m_need_abort.get(),
    },
    .m_columns_per_thread = event.size[0] / s_num_threads,
    .m_texture_width = event.size[0],
  };

  continuation.m_time_spent = std::clock() - startTime;
  continuation.m_real_time_spent = real_clock_t::now() - realStartTime;

  continuation.m_main_thread_info.m_until_x = calc_perlin_thread_finish(0, continuation.m_columns_per_thread, continuation.m_texture_width);
  info("main thread will work from {} to {}", continuation.m_main_thread_info.m_next_x, continuation.m_main_thread_info.m_until_x);
  
  ecs.entity().emplace<PerlinNoiseGenerationContinuation>(std::move(continuation));
}

static void generate_white_noise_texture(flecs::world& ecs, const Menu::EventGenerateWhiteNoiseTexture& event) {
  clear_previous_texture(ecs);
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
    .set<DrawableBitmap>({
      .bitmap = Bitmap(event.size[0], event.size[1]),
      .top_left = {0.0f, 0.0f}
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

static void generate_interpolated_texture(flecs::world& ecs, const Menu::EventGenerateInterpolatedTexture& event) {
  clear_previous_texture(ecs);
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
    .set<DrawableBitmap>({
      .bitmap = Bitmap(event.size[0], event.size[1]),
      .top_left = {0.0f, 0.0f}
     });
  
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

  ecs.system<PerlinNoiseGenerationContinuation>("Perlin noise generation")
    .kind(flecs::OnUpdate)
    .each([](const flecs::iter& it, size_t entity_index, PerlinNoiseGenerationContinuation& continuation) {
      auto ecs = it.world();

      if (continuation.m_additional_threads.empty())
        init_perlin_noise_generation_threads(continuation);

      bool didFinish = continue_perlin_noise_generation(continuation, 25ms);
      if (didFinish) {
        ecs.each([&ecs, &continuation](flecs::entity entity, Menu::EventReceiver){
          ecs.event<Menu::EventGenerationFinished>()
            .ctx(Menu::EventGenerationFinished{
              .secondsTaken = double(continuation.m_time_spent) / double(CLOCKS_PER_SEC),
              .realDuration = continuation.m_real_time_spent,
            })
            .entity(entity)
            .emit();
        });
        it.entity(entity_index).destruct();
      }
    });

  ecs.system<NoiseTexture, DrawableBitmap>("Prepare noise texture draw")
    .kind(phase::BeforeRender())
    .each([](NoiseTexture& texture, DrawableBitmap& drawable) {
      texture.prepare_for_draw(drawable.bitmap);
    });
}
