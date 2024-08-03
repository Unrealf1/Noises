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
