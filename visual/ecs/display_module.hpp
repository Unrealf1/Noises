#pragma once

#include <flecs_incl.hpp>
#include <allegro5/allegro5.h>


namespace phase {
  flecs::entity SystemEvents();
}

struct EventMouseAxes : public ALLEGRO_MOUSE_EVENT { };
struct EventMouseButtonDown : public ALLEGRO_MOUSE_EVENT { };
struct EventMouseButtonUp : public ALLEGRO_MOUSE_EVENT { };
struct EventMouseEnterDisplay : public ALLEGRO_MOUSE_EVENT { };
struct EventMouseLeaveDisplay : public ALLEGRO_MOUSE_EVENT { };
struct EventMouseWarped : public ALLEGRO_MOUSE_EVENT { };

struct EventKeyDown : public ALLEGRO_KEYBOARD_EVENT { };
struct EventKeyUp : public ALLEGRO_KEYBOARD_EVENT { };

struct EventDisplayResize : public ALLEGRO_DISPLAY_EVENT { };

flecs::entity get_input_event_receiver();

struct DisplayHolder {
  ALLEGRO_DISPLAY* display;
};

struct DisplayModule {
  DisplayModule(flecs::world&);
};

