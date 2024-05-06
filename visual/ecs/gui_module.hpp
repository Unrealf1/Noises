#pragma once

#include <flecs_incl.hpp>


namespace phase {
  flecs::entity RenderGui();
}

struct GuiModule {
  GuiModule(flecs::world&);
};
