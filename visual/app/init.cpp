#include "init.hpp"

#include <gui/gui.hpp>
#include <render/noise_texture.hpp>
#include <ecs/render_module.hpp>
#include <ecs/display_module.hpp>
#include <ecs/texture_inspection_module.hpp>
#include <random>
#include <spdlog/spdlog.h>


static flecs::query<DisplayHolder> s_display_query;
static flecs::query<InspectionState> s_inspection_state_query;

void create_systems(flecs::world& ecs) {
  s_display_query = ecs.query<DisplayHolder>();
  s_inspection_state_query = ecs.query<InspectionState>();

  // TODO: store display pointer somewhere in the NoiseTexture?
  ecs.system<NoiseTexture>("Draw noise texture")
    .kind(phase::Render())
    .each([&](NoiseTexture& texture){
      s_display_query.each([&](const DisplayHolder& holder){
        s_inspection_state_query.each([&](const InspectionState& inspectionState){
          texture.draw(holder.display, inspectionState);
        });
      });
    });

  ecs.system<NoiseTexture>("Prepare noise texture draw")
    .kind(phase::BeforeRender())
    .each([](NoiseTexture& texture){
      texture.prepare_for_draw();
    });

  ecs.system<NoiseTexture>("Prepare noise texture update")
    .kind(flecs::PreUpdate)
    .each([](NoiseTexture& texture) {
      texture.prepare_for_update();
    });


  ecs.system<NoiseTexture>("Update noise texture")
    .interval(3.0) // Run at 1Hz
    .kind(flecs::OnUpdate)
    .each([](NoiseTexture& texture){
      std::random_device dev{};
      std::default_random_engine eng(dev());
      std::bernoulli_distribution distr(0.5);

      auto white = al_map_rgb(255, 255, 255);
      auto black = al_map_rgb(0, 0, 0);
      auto width = texture.width();
      auto height = texture.height();
      for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
          auto r = distr(eng) ? 255 : 0;
          auto g = distr(eng) ? 255 : 0;
          auto b = distr(eng) ? 255 : 0;
          texture.set(x, y, al_map_rgb(r, g, b));
        }
      }
    });
}

void create_entities(flecs::world& ecs) {
  // create noise texture
  ecs.entity()
    .emplace<NoiseTexture>(500, 500);

  // create needed GUI
  ecs.entity()
    .set<GuiMenu>(GuiMenu{
      .title = "Test menu",
      .contents = std::unique_ptr<TestMenu>(new TestMenu())
    });
}


void app::init(flecs::world& ecs) {
  create_systems(ecs);
  create_entities(ecs);
}
