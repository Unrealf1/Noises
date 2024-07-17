#pragma once

#include <flecs_incl.hpp>


struct InspectionState {
  float x_offset = 0.0f;
  float y_offset = 0.0f;
#ifdef __EMSCRIPTEN__ // browser has much smaller texture to  work with
  float zoom = 1.5f;
#else
  float zoom = 1.0f;
#endif
  float zoom_step = 1.1f;
  float max_zoom = 100.0f;
  float min_zoom = 0.01f;
  bool is_dragging = false;
  float keyboard_pan_speed = 0.0f; // pixels per second
  float keyboard_pan_speed_normal = 100.0f;
  float keyboard_pan_speed_fast = 200.0f;
  float keyboard_pan_dx = 0.0f;
  float keyboard_pan_dy = 0.0f;
};

struct TextureInspectionModule {
  TextureInspectionModule(flecs::world&);
};
