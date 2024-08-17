#include "texture_generation_module.hpp"

#include <gui/menu.hpp>
#include <render/noise_texture.hpp>
#include <ecs/render_module.hpp>
#include <render/drawable_bitmap.hpp>
#include <math.hpp>
#include <ecs/texture_generation/interpolation_generation.hpp>
#include <ecs/texture_generation/white_noise_generation.hpp>
#include <ecs/texture_generation/perlin_generation.hpp>


static flecs::entity s_menu_event_receiver;

static void clear_previous_texture(flecs::world& ecs) {
  clear_perlin_noise_continuation(ecs);
  ecs.each([](flecs::entity entity, const NoiseTexture&){
    entity.destruct();
  });
}

TextureGenerationModule::TextureGenerationModule(flecs::world& ecs) {
  ecs.module<TextureGenerationModule>();   

  ecs.each([this](flecs::entity receiver, Menu::EventReceiver){
    m_menu_event_receiver = receiver;
  });
 
  m_menu_event_receiver
    .observe([&ecs](const Menu::EventGenerateWhiteNoiseTexture& event){
      clear_previous_texture(ecs);
      generate_white_noise_texture(ecs, event);
    });

  init_perlin_systems_generation_systems(ecs);
  m_menu_event_receiver
    .observe([&ecs](const Menu::EventGeneratePerlinNoiseTexture& event){
      clear_previous_texture(ecs);
      generate_perlin_noise_texture(ecs, event);
    });

  m_menu_event_receiver
    .observe([&ecs](const Menu::EventGenerateInterpolatedTexture& event) {
      clear_previous_texture(ecs);
      generate_interpolated_texture(ecs, event);    
    });

  ecs.observer<NoiseTexture, DrawableBitmap>()
    .event(flecs::OnSet)
    .each([](NoiseTexture& noise, DrawableBitmap& bitmap){
      bitmap.top_left = vec2(ivec2{-noise.width() / 2, -noise.height() / 2});
    });

  ecs.system<NoiseTexture, DrawableBitmap>("Prepare noise texture draw")
    .kind(phase::BeforeRender())
    .each([](NoiseTexture& texture, DrawableBitmap& drawable) {
      texture.prepare_for_draw(drawable.bitmap);
    });
}

