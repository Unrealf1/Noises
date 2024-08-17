#pragma once

#include <flecs_incl.hpp>
#include <gui/menu.hpp>


void generate_perlin_noise_texture(flecs::world&, const Menu::EventGeneratePerlinNoiseTexture& event);
void init_perlin_systems_generation_systems(flecs::world&);
void clear_perlin_noise_continuation(flecs::world&);

