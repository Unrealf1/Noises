#include "texture_generation_module.hpp"

#include <gui/menu.hpp>
#include <render/noise_texture.hpp>
#include <random>
#include <perlin.hpp>


static flecs::entity s_menu_event_receiver;


void generate_perlin_noise_texture(flecs::world& ecs, const Menu::EventGeneratePerlinNoiseTexture& event) {
  ecs.each([](flecs::entity entity, const NoiseTexture&){
    entity.destruct();
  });
  NoiseTexture texture(event.size[0], event.size[1]);

  std::random_device dev{};
  std::default_random_engine eng(dev());

  PerlinNoise noise({
    .grid_size_x = event.grid_size[0],
    .grid_size_y = event.grid_size[1],

    .grid_step_x = event.grid_step[0],
    .grid_step_y = event.grid_step[1],

    .offset_x = event.offset[0],
    .offset_y = event.offset[1],

    .normalize_offsets = event.normalize_offsets
  }, eng);

  auto width = texture.width();
  auto height = texture.height();
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      float value = noise(float(x), float(y));
      auto brightness = uint8_t(255.0f * value);
      texture.set(x, y, al_map_rgb(brightness, brightness, brightness));
    }
  }
  ecs.entity().emplace<NoiseTexture>(std::move(texture));
}

void generate_white_noise_texture(flecs::world& ecs, const Menu::EventGenerateWhiteNoiseTexture& event) {
  ecs.each([](flecs::entity entity, const NoiseTexture&){
    entity.destruct();
  });
  NoiseTexture texture(event.size[0], event.size[1]);

  std::random_device dev{};
  std::default_random_engine eng(dev());

  std::bernoulli_distribution distr(event.black_prob);

  auto width = texture.width();
  auto height = texture.height();
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      uint8_t r = distr(eng) ? 0 : 255;
      uint8_t g = distr(eng) ? 0 : 255;
      uint8_t b = distr(eng) ? 0 : 255;
      texture.set(x, y, al_map_rgb(r, g, b));
    }
  }
  ecs.entity().emplace<NoiseTexture>(std::move(texture));
}
#include <spdlog/spdlog.h>
TextureGenerationModule::TextureGenerationModule(flecs::world& ecs) {
  ecs.module<TextureGenerationModule>();   

  ecs.each([this](flecs::entity receiver, Menu::EventReceiver){
    m_menu_event_receiver = receiver;
  });

  spdlog::info("menu event receiver: {}", m_menu_event_receiver);

  
  m_menu_event_receiver
    .observe([&ecs](const Menu::EventGenerateWhiteNoiseTexture& event){
      generate_white_noise_texture(ecs, event);
    });

  m_menu_event_receiver
    .observe([&ecs](const Menu::EventGeneratePerlinNoiseTexture& event){
      generate_perlin_noise_texture(ecs, event);
    });
}

