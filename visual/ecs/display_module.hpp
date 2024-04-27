#pragma once

#include <flecs.h>


namespace phase {
  flecs::entity SystemEvents();
}

struct DisplayModule {
  DisplayModule(flecs::world&);
};

