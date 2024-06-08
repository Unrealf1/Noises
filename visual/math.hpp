#pragma once

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
};

vec3 operator+(const vec3& first, const vec3& second) {
  auto copy = first;
  return copy += second;
}

vec3 operator-(const vec3& first, const vec3& second) {
  auto copy = first;
  return copy -= second;
}

vec3 operator*(float scalar, const vec3& vec) {
  auto copy = vec;
  return copy *= scalar;
}

vec3 operator*(const vec3& vec, float scalar) {
  return scalar * vec;
}

