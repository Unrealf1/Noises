#include <app/stop.hpp>
#include <app/init.hpp>
#include <flecs_incl.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
static flecs::world* g_ecs;
#endif


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
  }, 0, 1);
#else
  while (!app::should_stop()) {
    ecs.progress();
  }
#endif
}

