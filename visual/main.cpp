#include <app/stop.hpp>
#include <app/init.hpp>
#include <flecs.h>


int main() {
  flecs::world ecs;
  app::init(ecs);

  while (!app::should_stop()) {
    ecs.progress();
  }
}

