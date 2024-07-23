#pragma once

#include <vector>
#include <random>
#include <numbers>


struct PerlinNoiseParameters {
  int grid_size_x;
  int grid_size_y;

  float grid_step_x;
  float grid_step_y;

  float offset_x = 0.0f;
  float offset_y = 0.0f;

  bool normalize_offsets = false;
  enum class InterpolationAlgorithm {
    bilinear, bicubic, bicubic_zero, nearest_neighboor
  } interpolation_algorithm = InterpolationAlgorithm::bilinear;
};

class PerlinNoise {
public:
  PerlinNoise(const PerlinNoiseParameters& parameters, auto& random_generator)
  : m_parameters(parameters)
  , m_grid_data(size_t(parameters.grid_size_x * parameters.grid_size_y * 2)) {
    std::uniform_real_distribution<float> angleDistr(0.0f, std::numbers::pi_v<float> * 2.0f);
    for (size_t i = 0; i < m_grid_data.size(); i += 2) {
      float angle = angleDistr(random_generator);

      float dx = std::cos(angle);
      float dy = std::sin(angle);
      m_grid_data[i] = dx;
      m_grid_data[i + 1] = dy;
    }
  }

  
  float operator()(float x, float y) const;

private:
  PerlinNoiseParameters m_parameters;
  std::vector<float> m_grid_data;
};

