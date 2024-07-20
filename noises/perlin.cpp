#include "perlin.hpp"

#include <cmath>
#include <utility>
#include <algorithm>
#include <interpolation.hpp>


float PerlinNoise::operator()(float x, float y) const {
  float centeredX = x - m_parameters.offset_x;
  float centeredY = y - m_parameters.offset_y;

  float fIndexX = centeredX / m_parameters.grid_step_x;
  float fIndexY = centeredY / m_parameters.grid_step_y;

  int left = int(fIndexX);
  int top = int(fIndexY);
  int right = left + 1;
  int bot = top + 1;

  if (left < 0 || right >= m_parameters.grid_size_x) {
    return 0.0f;
  }

  if (top < 0 || bot >= m_parameters.grid_size_y) {
    return 0.0f;
  }

  float leftPos = m_parameters.grid_step_x * float(left);
  float rightPos = m_parameters.grid_step_x * float(right);
  float topPos = m_parameters.grid_step_y * float(top);
  float botPos = m_parameters.grid_step_y * float(bot);

  float topLeftOffset[2] = { leftPos - centeredX, topPos - centeredY };
  float topRightOffset[2] = { rightPos - centeredX, topPos - centeredY };
  float botRightOffset[2] = { rightPos - centeredX, botPos - centeredY };
  float botLeftOffset[2] = { leftPos - centeredX, botPos - centeredY };

  if (m_parameters.normalize_offsets) {
    float topLeftOffsetLen = std::sqrt(topLeftOffset[0] * topLeftOffset[0] + topLeftOffset[1] * topLeftOffset[1]);
    topLeftOffset[0] /= topLeftOffsetLen;
    topLeftOffset[1] /= topLeftOffsetLen;

    float topRightOffsetLen = std::sqrt(topRightOffset[0] * topRightOffset[0] + topRightOffset[1] * topRightOffset[1]);
    topRightOffset[0] /= topRightOffsetLen;
    topRightOffset[1] /= topRightOffsetLen;

    float botRightOffsetLen = std::sqrt(botRightOffset[0] * botRightOffset[0] + botRightOffset[1] * botRightOffset[1]);
    botRightOffset[0] /= botRightOffsetLen;
    botRightOffset[1] /= botRightOffsetLen;

    float botLeftOffsetLen = std::sqrt(botLeftOffset[0] * botLeftOffset[0] + botLeftOffset[1] * botLeftOffset[1]);
    botLeftOffset[0] /= botLeftOffsetLen;
    botLeftOffset[1] /= botLeftOffsetLen;
  }

  const float* topLeftValue = &m_grid_data[size_t(left + top * m_parameters.grid_size_x) * 2];
  const float* topRightValue = &m_grid_data[size_t(right + top * m_parameters.grid_size_x) * 2];
  const float* botRightValue = &m_grid_data[size_t(right + bot * m_parameters.grid_size_x) * 2];
  const float* botLeftValue = &m_grid_data[size_t(left + bot * m_parameters.grid_size_x) * 2];

  float topLeftDot = topLeftOffset[0] * topLeftValue[0] + topLeftOffset[1] * topLeftValue[1];
  float topRightDot = topRightOffset[0] * topRightValue[0] + topRightOffset[1] * topRightValue[1];
  float botRightDot = botRightOffset[0] * botRightValue[0] + botRightOffset[1] * botRightValue[1];
  float botLeftDot = botLeftOffset[0] * botLeftValue[0] + botLeftOffset[1] * botLeftValue[1];

  float result = 0.0f;
  // interpolate between this values
  switch (m_parameters.interpolation_algorithm) {
    case PerlinNoiseParameters::InterpolationAlgorithm::bilinear: {
      auto horInterpK = (centeredX - leftPos) / m_parameters.grid_step_x;
      auto vertInterpK = (centeredY - topPos) / m_parameters.grid_step_y;

      result = interpolation::bilinear(topLeftDot, topRightDot, botLeftDot, botRightDot, horInterpK, vertInterpK);
      break;
    }
    case PerlinNoiseParameters::InterpolationAlgorithm::bicubic: {
      auto coefs = interpolation::calc_bicubic_coefficients(
          topLeftDot, topRightDot, botLeftDot, botRightDot,
          topLeftValue[0], topRightValue[0], botLeftValue[0], botRightValue[0],
          topLeftValue[1], topRightValue[1], botLeftValue[1], botRightValue[1],
          0.0f, 0.0f, 0.0f, 0.0f
      );

      auto horInterpK = (centeredX - leftPos) / m_parameters.grid_step_x;
      auto vertInterpK = (centeredY - topPos) / m_parameters.grid_step_y;

      result = interpolation::bicubic(horInterpK, vertInterpK, coefs);
      break;
    }
    case PerlinNoiseParameters::InterpolationAlgorithm::bicubic_zero: {
      auto coefs = interpolation::calc_bicubic_coefficients(
          topLeftDot, topRightDot, botLeftDot, botRightDot,
          0.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 0.0f
      );

      auto horInterpK = (centeredX - leftPos) / m_parameters.grid_step_x;
      auto vertInterpK = (centeredY - topPos) / m_parameters.grid_step_y;

      result = interpolation::bicubic(horInterpK, vertInterpK, coefs);
      break;
    }
    case PerlinNoiseParameters::InterpolationAlgorithm::nearest_neighboor: {
      auto dx = centeredX - leftPos;
      auto dy = centeredY - rightPos;

      bool isLeft = dx <= m_parameters.grid_step_x / 2.0f;
      bool isTop = dy <= m_parameters.grid_step_y / 2.0f;

      result = isLeft & isTop ? topLeftDot
        : isLeft & !isTop ? botLeftDot
        : !isLeft & isTop ? topRightDot
        : botRightDot;
      break;
    }
    default:
      std::unreachable();
  }
  float cellDiagonal = std::sqrt(m_parameters.grid_step_x * m_parameters.grid_step_x + m_parameters.grid_step_y * m_parameters.grid_step_y);
  float normalizedResult = m_parameters.normalize_offsets ? result : result / cellDiagonal;
  return std::clamp((1.0f + normalizedResult) / 2.0f, 0.0f, 1.0f);
}

