#include <app/stop.hpp>
#include <app/init.hpp>
#include <flecs.h>

#include <ecs/render_module.hpp>
#include <ecs/display_module.hpp>
#include <ecs/gui_module.hpp>


void import_modules(flecs::world& ecs) {
  ecs.import<RenderModule>();
  ecs.import<DisplayModule>();
  ecs.import<GuiModule>();
}

int main() {
  flecs::world ecs;
  import_modules(ecs);

  app::init(ecs);

  while (!app::should_stop()) {
    ecs.progress();
  }
}

