#pragma once

#include <gui/gui.hpp>
#include <array>
#include <ecs/texture_inspection_module.hpp>
#include <chrono>
#include <perlin.hpp>

enum class MenuNoisesIndices { white, perlin, interpolation };
static constexpr std::array s_noises {"white", "perlin", "interpolation"};

class Menu : public GuiMenuContents {
public:
  struct EventGenerateWhiteNoiseTexture {
    int size[2] = {500, 500};
    float black_prob = 0.5f;
  };

  struct EventGeneratePerlinNoiseTexture {
    int size[2] = {500, 500};
    int grid_size[2] = {20, 20};
    float grid_step[2] = {25.0f, 25.0f};
    float offset[2] = {0.0f, 0.0f};
    bool normalize_offsets = false;
    PerlinNoiseParameters::InterpolationAlgorithm interpolation_algorithm = PerlinNoiseParameters::InterpolationAlgorithm::bilinear;
  };

  struct EventGenerateInterpolatedTexture {
    int size[2] = {500, 500};
    float colors[3 * 16] = {
      1, 0, 0,
      0, 1, 0,
      0, 0, 1,
      0, 0, 0,
      1, 1, 1,
      0, 1, 1,
      1, 0, 1,
      1, 1, 0,
      1, 0, 0,
      0, 1, 0,
      0, 0, 1,
      0, 0, 0,
      1, 1, 1,
      0, 1, 1,
      1, 0, 1,
      1, 1, 0
    };
    enum class Algorithm {
      bilinear, nearest_neighboor
    } algorithm = Algorithm::nearest_neighboor;
  };

  struct EventReceiver{};

  struct EventGenerationFinished{
    double secondsTaken;
  };
public:
  Menu(flecs::world&, flecs::entity menu_eid);
  void draw(flecs::world&) override;

  int m_noise_idx = 0;
  flecs::entity m_event_receiver;

private:
  flecs::query<InspectionState> m_inspection_state_query;
  int m_current_texture_size[2] = {0, 0};
  double m_last_generation_time_seconds = 0.0;

  EventGenerateWhiteNoiseTexture m_white_noise_params;
  EventGeneratePerlinNoiseTexture m_perlin_noise_params;
  EventGenerateInterpolatedTexture m_interpolated_texture_params;
};

