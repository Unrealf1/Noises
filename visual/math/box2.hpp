#pragma once

#include <math/vec2.hpp>

struct Box2 {
  vec2 top_left;
  vec2 bot_right;

  bool intersects(const Box2& other) const {
    return top_left.x <= other.bot_right.x
      && top_left.y <= other.bot_right.y
      && bot_right.x >= other.top_left.x
      && bot_right.y >= other.top_left.y;
  }

  vec2 dims() const {
    return bot_right - top_left;
  }
};
