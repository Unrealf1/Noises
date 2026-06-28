#include <app/stop.hpp>
#include <app/init.hpp>
#include <app/runtime.hpp>
#include <flecs_incl.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
static flecs::world* g_ecs;
#endif


static size_t g_cur_frame = 0;

size_t cur_frame() {
  return g_cur_frame;
}

int main() {
  flecs::world ecs;
  app::init(ecs);

#ifdef __EMSCRIPTEN__
  g_ecs = &ecs;
  emscripten_set_main_loop([]{
    if (app::should_stop()) {
      emscripten_cancel_main_loop();
    }
    g_ecs->progress();
    g_cur_frame += 1;
  }, 0, 1);
#else
  while (!app::should_stop()) {
    ecs.progress();
    g_cur_frame += 1;
  }
#endif
}

