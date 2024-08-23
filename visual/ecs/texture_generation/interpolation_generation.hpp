#pragma once

#include <flecs_incl.hpp>
#include <gui/menu.hpp>


void generate_interpolated_texture(flecs::world&, const Menu::EventGenerateInterpolatedTexture& event);

void init_interpolated_generation_systems(flecs::world&);

