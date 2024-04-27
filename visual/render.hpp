#pragma once

#include "allegro_util.hpp"


namespace render{
  struct EventSourceBeforeFrame : public ALLEGRO_EVENT_SOURCE { };

  struct EventSourceDrawFrame : public ALLEGRO_EVENT_SOURCE { };


  void init();
  void uninit();

  void start_frame();
  void draw_frame();
  void finish_frame();

  ALLEGRO_EVENT_SOURCE* get_before_frame_event_source();
  ALLEGRO_EVENT_SOURCE* get_draw_frame_event_source();

  void render_loop(ALLEGRO_DISPLAY* display);
}
