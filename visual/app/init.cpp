#include "init.hpp"

#include <render/noise_texture.hpp>
#include <ecs/render_module.hpp>
#include <ecs/display_module.hpp>
#include <ecs/gui_module.hpp>
#include <ecs/texture_inspection_module.hpp>
#include <ecs/texture_generation_module.hpp>


static flecs::query<DisplayHolder> s_display_query;
static flecs::query<InspectionState> s_inspection_state_query;
static flecs::query<NoiseTexture> s_noise_texture_query;

void init_queries(flecs::world& ecs) {
  s_display_query = ecs.query<DisplayHolder>();
  s_inspection_state_query = ecs.query<InspectionState>();
  s_noise_texture_query = ecs.query<NoiseTexture>();
}

void import_modules(flecs::world& ecs) {
  ecs.import<RenderModule>();
  ecs.import<DisplayModule>();
  ecs.import<GuiModule>();
  ecs.import<TextureInspectionModule>();
  ecs.import<TextureGenerationModule>();
}

void create_systems(flecs::world& ecs) {
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
}

void app::init(flecs::world& ecs) {
  init_queries(ecs);
  import_modules(ecs);
  create_systems(ecs);
}

