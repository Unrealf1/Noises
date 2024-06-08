#include "perlin.hpp"

#include <cmath>
#include <utility>
#include <algorithm>


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

  float* topLeftValue = &m_grid_data[size_t(left + top * m_parameters.grid_size_x) * 2];
  float* topRightValue = &m_grid_data[size_t(right + top * m_parameters.grid_size_x) * 2];
  float* botRightValue = &m_grid_data[size_t(right + bot * m_parameters.grid_size_x) * 2];
  float* botLeftValue = &m_grid_data[size_t(left + bot * m_parameters.grid_size_x) * 2];

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

      auto topInterp = std::lerp(topLeftDot, topRightDot, horInterpK);
      auto botInterp = std::lerp(botLeftDot, botRightDot, horInterpK);
      result = std::lerp(topInterp, botInterp, vertInterpK);
      break;
    }
    case PerlinNoiseParameters::InterpolationAlgorithm::bicubic: {
      // TODO: fallback to bilinear
      if (left == 0 || right == m_parameters.grid_size_x - 1) {
        return 0.0f;
      }
      if (top == 0 || bot == m_parameters.grid_size_y - 1) {
        return 0.0f;
      }
      
      // For this we need 16 points
      // -------------
      // |0  1  2  3  |
      // |4  5  6  7  |
      // |8  9  10 11 |
      // |12 13 14 15 |
      // -------------
      // Where 5, 6, 9, 10 are old topLeft, topRight, botLeft, botRight

      auto dot = [this, &centeredX, &centeredY](float* pos, float* val) {
        float offset[2] = { pos[0] - centeredX, pos[1] - centeredY };
        if (m_parameters.normalize_offsets) {
          float offsetLen = std::sqrt(offset[0] * offset[0] + offset[1] * offset[1]);
          offset[0] /= offsetLen;
          offset[1] /= offsetLen;
        } 
        return offset[0] * val[0] + offset[1] * val[1];
      };

      float allDots[16];
      for (int xOffset = -1; xOffset <= 2; ++xOffset) {
        for (int yOffset = -1; yOffset <= 2; ++yOffset) {
          auto index = ((xOffset + 1) + (yOffset + 1) * 4);
          auto globalX = left + xOffset;
          auto globalY = top + yOffset;
          float pos[2];
          pos[0] = m_parameters.grid_step_x * float(globalX);
          pos[1] = m_parameters.grid_step_y * float(globalY);

          float vec[2];
          vec[0] = m_grid_data[size_t(globalX + globalY * m_parameters.grid_size_x) * 2];
          vec[1] = m_grid_data[size_t(globalX + globalY * m_parameters.grid_size_x) * 2 + 1];
          allDots[index] = dot(pos, vec);
        }
      }

      float dx = centeredX / m_parameters.grid_step_x - float(left);
      float dy = centeredY / m_parameters.grid_step_y - float(top);

      float x0 = dx - 2.0f;
      float x1 = dx - 1.0f;
      float x2 = dx;
      float x3 = dx + 1.0f;

      float y0 = dy - 2.0f;
      float y1 = dy - 1.0f;
      float y2 = dy;
      float y3 = dy + 1.0f;

      // formula taken from wikipedia :(
      float b[16] = {
        0.25f * (x1) * (x0) * (x3) * (y1) * (y0) * (y3),        // 1
        - 0.25f * x2 * (x3) * (x0) * (y1) * (y0) * (y3),            // 2
        - 0.25f * y2 * (x1) * (x0) * (x3) * (y3) * (y0),            // 3
        0.25f * x2 * y2 * (x3) * (x0) * (y3) * (y0),                     // 4
        - 1.0f / 12.0f * x2 * (x1) * (x0) * (y1) * (y0) * (y3),     // 5
        - 1.0f / 12.0f * y2 * (x1) * (x0) * (x3) * (y1) * (y0),     // 6
        1.0f / 12.0f * x2 * y2 * (x1) * (x0) * (y3) * (y0),             // 7
        1.0f / 12.0f * x2 * y2 * (x3) * (x0) * (y1) * (y0),             // 8
        1.0f / 12.0f * x2 * (x1) * (x3) * (y1) * (y0) * (y3),       // 9
        1.0f / 12.0f * y2 * (x1) * (x0) * (x3) * (y1) * (y3),       // 10
        1.0f / 36.0f * x2 * y2 * (x1) * (x0) * (y1) * (y0),             // 11
        - 1.0f / 12.0f * x2 * y2 * (x1) * (x3) * (y3) * (y0),           // 12
        - 1.0f / 12.0f * x2 * y2 * (x3) * (x0) * (y1) * (y3),           // 13
        - 1.0f / 36.0f * x2 * y2 * (x1) * (x3) * (y1) * (y0),           // 14
        - 1.0f / 36.0f * x2 * y2 * (x1) * (x0) * (y1) * (y3),           // 15
        1.0f / 36.0f * x2 * y2 * (x1) * (x3) * (y1) * (y3)              // 16
      };

      auto v = [&allDots] (int x, int y) {
        return allDots[x + y * 4];
      };
      result = b[0] * v(1, 1)
        + b[1]  * v(1, 2)
        + b[2]  * v(2, 1)
        + b[3]  * v(2, 2)
        + b[4]  * v(1, 0)
        + b[5]  * v(0, 1)
        + b[6]  * v(2, 0)
        + b[7]  * v(0, 2)
        + b[8]  * v(1, 3)
        + b[9]  * v(3, 1)
        + b[10] * v(0, 0)
        + b[11] * v(2, 3)
        + b[12] * v(3, 2)
        + b[13] * v(0, 3)
        + b[14] * v(3, 0)
        + b[15] * v(3, 3);
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

