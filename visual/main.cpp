#include <stop.hpp>
#include <flecs.h>

#include <ecs/render_module.hpp>
#include <ecs/display_module.hpp>


void import_modules(flecs::world& ecs) {
  ecs.import<RenderModule>();
  ecs.import<DisplayModule>();
}

int main() {
  flecs::world ecs;
  import_modules(ecs);

  while (!app::should_stop()) {
    ecs.progress();
  }
}

