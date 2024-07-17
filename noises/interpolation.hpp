#pragma once


namespace interpolation{
  template<typename T>
  T lerp(T a, T b, float k) {
    return a + k * (b - a);
  }

  template<typename T>
  T bilinear(T top_left, T top_right, T bot_left, T bot_right, float dx, float dy) {
    T top_lerp = lerp(top_left, top_right, dx);  
    T bot_lerp = lerp(bot_left, bot_right, dx);  
    return lerp(top_lerp, bot_lerp, dy);
  }

  template<typename T>
  struct BicubicCoefficients{
    T a[16];
  };

  template<typename T>
  BicubicCoefficients<T> calc_bicubic_coefficients(T F_top_left, T F_top_right, T F_bot_left, T F_bot_right,
                                                   T dFx_top_left, T dFx_top_right, T dFx_bot_left, T dFx_bot_right,
                                                   T dFy_top_left, T dFy_top_right, T dFy_bot_left, T dFy_bot_right,
                                                   T dFxy_top_left, T dFxy_top_right, T dFxy_bot_left, T dFxy_bot_right) {
    float wikiInverseA[16 * 16] = {
      1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      -3,  3,  0,  0,  -2,  -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      2,  -2,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  -3,  3,  0,  0,  -2,  -1,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  2,  -2,  0,  0,  1,  1,  0,  0,
      -3,  0,  3,  0,  0,  0,  0,  0,  -2,  0,  -1,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  -3,  0,  3,  0,  0,  0,  0,  0,  -2,  0,  -1,  0,
      9,  -9,  -9,  9,  6,  3,  -6,  -3,  6,  -6,  3,  -3,  4,  2,  2,  1,
      -6,  6,  6,  -6,  -3,  -3,  3,  3,  -4,  4,  -2,  2,  -2,  -2,  -1,  -1,
      2,  0,  -2,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  2,  0,  -2,  0,  0,  0,  0,  0,  1,  0,  1,  0,
      -6,  6,  6,  -6,  -4,  -2,  4,  2,  -3,  3,  -3,  3,  -2,  -1,  -2,  -1,
      4,  -4,  -4,  4,  2,  2,  -2,  -2,  2,  -2,  2,  -2,  1,  1,  1,  1
    };

    T wikiX[16] = {
      F_top_left, F_top_right, F_bot_left, F_bot_right,
      dFx_top_left, dFx_top_right, dFx_bot_left, dFx_bot_right,
      dFy_top_left, dFy_top_right, dFy_bot_left, dFy_bot_right,
      dFxy_top_left, dFxy_top_right, dFxy_bot_left, dFxy_bot_right
    };

    BicubicCoefficients<T> wikiCoefs;
    for (int i = 0; i < 16; ++i) {
      wikiCoefs.a[i] = T{};
      for (int j = 0; j < 16; ++j) {
        wikiCoefs.a[i] += wikiInverseA[16 * i + j] * wikiX[j];
      }
    }
    return wikiCoefs;
  }

  template<typename T>
  T bicubic(float dx, float dy, const BicubicCoefficients<T>& coefs) {
    const float xs[4] = { 1.f, dx, dx * dx, dx * dx * dx };
    const float ys[4] = { 1.f, dy, dy * dy, dy * dy * dy };

    T res{};
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        res += coefs.a[4 * j + i] * xs[i] * ys[j];
      }
    }

    return res;
  }
}

