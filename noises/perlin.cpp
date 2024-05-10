#include "perlin.hpp"

#include <cmath>


float PerlinNoise::operator()(float x, float y) {
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

  float* topLeftValue = &m_grid_data[(left + top * m_parameters.grid_size_x) * 2];
  float* topRightValue = &m_grid_data[(right + top * m_parameters.grid_size_x) * 2];
  float* botRightValue = &m_grid_data[(right + bot * m_parameters.grid_size_x) * 2];
  float* botLeftValue = &m_grid_data[(left + bot * m_parameters.grid_size_x) * 2];

  float topLeftDot = topLeftOffset[0] * topLeftValue[0] + topLeftOffset[1] * topLeftValue[1];
  float topRightDot = topRightOffset[0] * topRightValue[0] + topRightOffset[1] * topRightValue[1];
  float botRightDot = botRightOffset[0] * botRightValue[0] + botRightOffset[1] * botRightValue[1];
  float botLeftDot = botLeftOffset[0] * botLeftValue[0] + botLeftOffset[1] * botLeftValue[1];

  // interpolate between this values
  auto horInterpK = (centeredX - leftPos) / m_parameters.grid_step_x;
  auto vertInterpK = (centeredY - topPos) / m_parameters.grid_step_y;

  auto topInterp = std::lerp(topLeftDot, topRightDot, horInterpK);
  auto botInterp = std::lerp(botLeftDot, botRightDot, horInterpK);
  auto result = std::lerp(topInterp, botInterp, vertInterpK);

  float cellDiagonal = std::sqrt(m_parameters.grid_step_x * m_parameters.grid_step_x + m_parameters.grid_step_y * m_parameters.grid_step_y);
  float normalizedResult = result / cellDiagonal;
  return (1.0f + normalizedResult) / 2.0f;
}

