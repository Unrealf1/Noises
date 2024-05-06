#include <app/stop.hpp>
#include <app/init.hpp>
#include <flecs_incl.hpp>


int main() {
  flecs::world ecs;
  app::init(ecs);

  while (!app::should_stop()) {
    ecs.progress();
  }
}

