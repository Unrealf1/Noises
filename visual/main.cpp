#include <noises/noises.hpp>

#include <render.hpp>

#include <flecs.h>
#include <spdlog/spdlog.h>

struct Position{
  float x, y;
};

struct Velocity{
  float x, y;
};

int main() {
  spdlog::info("visualization: noises version is {}\n", NOISES_VERSION);
  flecs::world ecs;

  flecs::system sys = ecs.system<Position, const Velocity>("Move")
    .each([](Position& p, const Velocity &v) {
        // Each is invoked for each entity
        p.x += v.x;
        p.y += v.y;
    });



  render::init();

  auto beforeFrameSource = render::get_before_frame_event_source();
  auto drawFrameSource = render::get_draw_frame_event_source();

  auto display = al_create_display(200, 200);

  render::render_loop(display);

  render::uninit();
}

