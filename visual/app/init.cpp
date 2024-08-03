#include "init.hpp"

#include <render/noise_texture.hpp>
#include <ecs/render_module.hpp>
#include <ecs/display_module.hpp>
#include <ecs/gui_module.hpp>
#include <ecs/camera_module.hpp>
#include <ecs/texture_generation_module.hpp>


void import_modules(flecs::world& ecs) {
  ecs.import<RenderModule>();
  ecs.import<DisplayModule>();
  ecs.import<GuiModule>();
  ecs.import<CameraModule>();
  ecs.import<TextureGenerationModule>();
}

void app::init(flecs::world& ecs) {
  import_modules(ecs);
}

