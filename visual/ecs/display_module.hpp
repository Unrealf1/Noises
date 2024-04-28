#pragma once

#include <flecs.h>
#include <allegro5/allegro5.h>


namespace phase {
  flecs::entity SystemEvents();
}

struct DisplayHolder {
  ALLEGRO_DISPLAY* display;
};

struct DisplayModule {
  DisplayModule(flecs::world&);
};

