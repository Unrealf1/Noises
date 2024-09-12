#include "perlin_generation.hpp"

#include <atomic>
#include <ctime>
#include <perlin.hpp>
#include <memory>
#include <thread>
#include <render/noise_texture.hpp>
#include <render/drawable_bitmap.hpp>
#include <ecs/util.hpp>
#include <ecs/render_module.hpp>
#include <log.hpp>

using namespace std::chrono_literals;
using real_clock_t = std::chrono::steady_clock;
using perlin_noise_holder_t = std::unique_ptr<PerlinNoise>;
static constexpr int s_num_threads = 4;


struct PerlinGradientsScaler {
  float max_zoom = 1.0f;
};
struct PerlinGradientBitmap : public Tag {};

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
  PerlinNoise* m_noise;
  std::unique_ptr<std::atomic<size_t>> m_threads_finished;
  std::unique_ptr<std::atomic<bool>> m_need_abort;
  std::vector<std::thread> m_additional_threads;
  PerlinNoisePerThreadInfo m_main_thread_info;
  int m_columns_per_thread;
  int m_texture_width;
};

static int calc_perlin_thread_finish(int thread_idx, int columns_per_thread, int width) {
  return thread_idx == s_num_threads - 1 ? width : (thread_idx + 1) * columns_per_thread;
}

void generate_perlin_noise_texture(flecs::world& ecs, const Menu::EventGeneratePerlinNoiseTexture& event) {
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

  auto noise = std::make_unique<PerlinNoise>(PerlinNoiseParameters{
    .grid_size_x = event.grid_size[0],
    .grid_size_y = event.grid_size[1],

    .grid_step_x = event.grid_step[0],
    .grid_step_y = event.grid_step[1],

    .offset_x = event.offset[0],
    .offset_y = event.offset[1],

    .normalize_offsets = event.normalize_offsets,
    .interpolation_algorithm = event.interpolation_algorithm
  }, eng);

  PerlinNoiseGenerationContinuation continuation{
    .m_time_spent = 0,
    .m_real_time_spent = {},
    .m_noise = noise.get(),
    .m_threads_finished = std::make_unique<std::atomic<size_t>>(0),
    .m_need_abort = std::make_unique<std::atomic<bool>>(false),
    .m_additional_threads = {},
    .m_main_thread_info = {
      .m_texture = textureEntity,
      .m_next_x = 0,
      .m_until_x = 0,
      .m_noise = noise.get(),
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
  textureEntity.set<perlin_noise_holder_t>(std::move(noise));
}


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

static void init_perlin_noise_generation_threads(PerlinNoiseGenerationContinuation& continuation) {
  continuation.m_additional_threads.reserve(s_num_threads - 1);
  info("initializing threads. {} {}", continuation.m_columns_per_thread, continuation.m_texture_width);
  for (int i = 1; i < s_num_threads; ++i) { // first thread is the main thread
    PerlinNoisePerThreadInfo newThreadInfo {
      .m_texture = continuation.m_main_thread_info.m_texture,
      .m_next_x = i * continuation.m_columns_per_thread,
      .m_until_x = calc_perlin_thread_finish(i, continuation.m_columns_per_thread, continuation.m_texture_width),
      .m_noise = continuation.m_noise,
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

static void clear_gradient_visualization(flecs::world& ecs) {
  ecs.each([](flecs::entity eid, PerlinGradientBitmap){
    eid.destruct();
  });
  ecs.each([](flecs::entity eid, PerlinGradientsScaler) {
    eid.destruct();
  });
}

void init_perlin_systems_generation_systems(flecs::world& ecs) {
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

  ecs.observer<Menu::EventReceiver>()
    .event<Menu::EventShowPerlinGradients>()
    .each([](flecs::iter& it, size_t, Menu::EventReceiver){
      auto world = it.world();
      clear_gradient_visualization(world);
      float maxAllowedZoom = 1.0f;
      world.each([&it, &maxAllowedZoom](const perlin_noise_holder_t& noise, DrawableBitmap& bitmap){
        const int texSide = 50;
        const ivec2 texturePixelSize = {texSide, texSide};
        const vec2 textureCenter = vec2(texturePixelSize) / 2;
        const float lineLength = float(texSide) / 2.0f;
        const float circleRadius = float(texSide) / 10.0f;
        const auto color = al_map_rgb(255, 0, 0);
        const float width = 3.0f;

        auto& params = noise->m_parameters;

        maxAllowedZoom = std::min(
          params.grid_step_x / float(texSide) / 2.0f,
          params.grid_step_y / float(texSide) / 2.0f
        );

        auto textureSize = vec2(ivec2{
          .x = bitmap.bitmap.width(),
          .y = bitmap.bitmap.height()
        });
        vec2 offset = bitmap.center - textureSize / 2.0f;
        for (int i = 0; i < params.grid_size_y; ++i) {
          for (int j = 0; j < params.grid_size_x; ++j) {
            const vec2 texturePosition = vec2{
              .x = float(j) * params.grid_step_x,
              .y = float(i) * params.grid_step_y,
            } + offset;
            const float* gridNodeData = noise->get_grid_node_data(j, i);
            const vec2 textureDirection = {gridNodeData[0], gridNodeData[1]};

            auto oldFormat = al_get_new_bitmap_format();
            al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
            Bitmap bitmap(texturePixelSize.x, texturePixelSize.y);
            al_set_new_bitmap_format(oldFormat);

            TargetBitmapOverride targetOverride(bitmap.get_raw());

            al_clear_to_color(al_map_rgba(0, 0, 0, 0));
            al_draw_circle(textureCenter.x, textureCenter.y, circleRadius, color, width);

            vec2 endPoint = textureCenter + textureDirection * lineLength;
            al_draw_line(textureCenter.x, textureCenter.y,
                         endPoint.x, endPoint.y,
                         color, width);

            it.world().entity()
              .emplace<DrawableBitmap>(std::move(bitmap), texturePosition)
              .emplace<DrawableBitmapScale>(1.0f, 1.0f)
              .add<PerlinGradientBitmap>();
          }
        }
      });
      world.entity().emplace<PerlinGradientsScaler>(maxAllowedZoom);
  });

  ecs.observer<Menu::EventReceiver>()
    .event<Menu::EventHidePerlinGradients>()
    .event<Menu::EventGenerateWhiteNoiseTexture>()
    .event<Menu::EventGenerateInterpolatedTexture>()
    .each([](flecs::iter& it, size_t, Menu::EventReceiver){
      auto world = it.world();
      clear_gradient_visualization(world);
    });

  ecs.system<const PerlinGradientsScaler>("Perlin noise gradient visualization")
    .kind(phase::BeforeRender())
    .each([](const flecs::iter& it, size_t, const PerlinGradientsScaler scaler) {
        vec2 textureZoom = {1.0f, 1.0f};
        //TODO: add cached queries
        it.world().each([&textureZoom, &scaler](const CameraState& cam_state){
          float rawZoom = 1.0f / cam_state.zoom;
          float zoom = std::min(rawZoom, scaler.max_zoom);
          textureZoom.x = zoom;
          textureZoom.y = zoom;
        });

        it.world().each([&textureZoom](DrawableBitmapScale& scale,
                                       PerlinGradientBitmap) {
          scale = textureZoom;
        });
    });
}

void clear_perlin_noise_continuation(flecs::world& ecs) {
  ecs.each([](flecs::entity entity, PerlinNoiseGenerationContinuation& continuation){
    continuation.m_need_abort->store(true);
    for (auto& thread : continuation.m_additional_threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    entity.destruct();
  });
}

