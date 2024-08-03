#pragma once

#include <cmath>


struct vec3{
  float x;
  float y;
  float z;

  vec3& operator+=(const vec3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  vec3& operator-=(const vec3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  vec3& operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }

  vec3& operator/=(float scalar) {
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
  }

  float length() const {
    return std::sqrt(x * x + y * y + z * z);
  }

  float length_sq() const {
    return x * x + y * y + z * z;
  }
};

inline vec3 operator+(const vec3& first, const vec3& second) {
  auto copy = first;
  return copy += second;
}

inline vec3 operator-(const vec3& first, const vec3& second) {
  auto copy = first;
  return copy -= second;
}

inline vec3 operator*(float scalar, const vec3& vec) {
  auto copy = vec;
  return copy *= scalar;
}

inline vec3 operator*(const vec3& vec, float scalar) {
  return scalar * vec;
}

inline vec3 operator/(const vec3& vec, float scalar) {
  return 1.0f / scalar * vec;
}

inline float distance(const vec3& first, const vec3& second) {
  return (first - second).length();
}


struct vec2{
  float x;
  float y;

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
