#pragma once

#include <allegro_util.hpp>
#include <math.hpp>


struct DrawableBitmap {
  Bitmap bitmap;
  vec2 center;
};

struct DrawableBitmapScale : public vec2 {
  using vec2::operator=;
};

