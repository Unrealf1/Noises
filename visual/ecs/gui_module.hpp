#pragma once

#include <flecs.h>


namespace phase {
  flecs::entity RenderGui();
}

struct GuiModule {
  GuiModule(flecs::world&);
};
