#pragma once

#include <cmath>


struct vec2;
struct ivec2;

struct vec2{
  float x;
  float y;

  explicit operator ivec2() const;

  vec2& operator+=(vec2 other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  vec2& operator-=(vec2 other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  vec2& operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
  }

  vec2& operator/=(float scalar) {
    x /= scalar;
    y /= scalar;
    return *this;
  }

  float length() const {
    return std::sqrt(x * x + y * y);
  }

  float length_sq() const {
    return x * x + y * y;
  }
};

inline vec2 operator+(vec2 first, vec2 second) {
  auto copy = first;
  return copy += second;
}

inline vec2 operator-(vec2 first, vec2 second) {
  auto copy = first;
  return copy -= second;
}

inline vec2 operator*(float scalar, vec2 vec) {
  auto copy = vec;
  return copy *= scalar;
}

inline vec2 operator*(vec2 vec, float scalar) {
  return scalar * vec;
}

inline vec2 operator/(const vec2& vec, float scalar) {
  return 1.0f / scalar * vec;
}

inline float distance(vec2 first, vec2 second) {
  return (first - second).length();
}

struct ivec2{
  int x;
  int y;

  explicit operator vec2() const;

  ivec2& operator+=(ivec2 other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  ivec2& operator-=(ivec2 other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }
};

inline ivec2 operator+(ivec2 first, ivec2 second) {
  auto copy = first;
  return copy += second;
}

inline ivec2 operator-(ivec2 first, ivec2 second) {
  auto copy = first;
  return copy -= second;
}

inline vec2::operator ivec2() const {
  return {int(x), int(y)};
}

inline ivec2::operator vec2() const {
  return {float(x), float(y)};
}
