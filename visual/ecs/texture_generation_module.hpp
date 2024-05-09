#pragma once

#include <flecs_incl.hpp>


struct TextureGenerationModule {
public:
  TextureGenerationModule(flecs::world&);

private:
  flecs::entity m_menu_event_receiver;
};

