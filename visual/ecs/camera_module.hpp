#pragma once

#include <flecs_incl.hpp>
#include <math.hpp>


struct CameraState {
  vec2 center{0.0f, 0.0f};
  ivec2 display_dimentions;
  Box2 view;

  float zoom = 1.0f;
  float zoom_step = 1.1f;
  float max_zoom = 100.0f;
  float min_zoom = 0.01f;

  bool is_dragging = false;

  float keyboard_pan_speed = 0.0f; // pixels per second
  float keyboard_pan_speed_normal = 100.0f;
  float keyboard_pan_speed_fast = 200.0f;
  vec2 keyboard_pan{0.0f, 0.0f};
};

struct CameraModule {
  CameraModule(flecs::world&);
};
