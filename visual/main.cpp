#include <noises/noises.hpp>

#include <render.hpp>

#include <spdlog/spdlog.h>


int main() {
  spdlog::info("visualization: noises version is {}\n", NOISES_VERSION);

  render::init();

  auto beforeFrameSource = render::get_before_frame_event_source();
  auto drawFrameSource = render::get_draw_frame_event_source();

  auto display = al_create_display(200, 200);

  render::render_loop(display);

  render::uninit();
}

