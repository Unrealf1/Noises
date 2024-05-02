#pragma once

#include <flecs.h>


struct InspectionState {
  float x_offset = 0.0f;
  float y_offset = 0.0f;
  float zoom = 1.0f;
  float zoom_step = 1.1f;
  float max_zoom = 100.0f;
  float min_zoom = 0.01f;
  bool is_dragging = false;
  float keyboard_pan_speed = 1.0f;
  float keyboard_pan_speed_normal = 1.0f;
  float keyboard_pan_speed_fast = 3.0f;
  float keyboard_pan_dx = 0.0f;
  float keyboard_pan_dy = 0.0f;
};

struct TextureInspectionModule {
  TextureInspectionModule(flecs::world&);
};
