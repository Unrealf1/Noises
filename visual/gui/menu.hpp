#pragma once

#include <gui/gui.hpp>
#include <array>
#include <ecs/camera_module.hpp>
#include <ecs/util.hpp>
#include <chrono>
#include <perlin.hpp>

enum class MenuNoisesIndices { perlin, interpolation, white };
static constexpr std::array s_noises {"perlin", "interpolation", "white"};

// its nice to have default size be divided by 3, so interpolation example looks good by default
constexpr int s_default_texture_size = 900;

class Menu : public GuiMenuContents {
public:
  struct EventGenerateWhiteNoiseTexture {
    int size[2] = {s_default_texture_size, s_default_texture_size};
    float black_prob = 0.5f;
  };

  struct EventGeneratePerlinNoiseTexture {
    int size[2] = {s_default_texture_size, s_default_texture_size};

    int grid_size[2] = {31, 31};
    float grid_step[2] = {30.0f, 30.0f};

    float offset[2] = {0.0f, 0.0f};
    bool normalize_offsets = false;
    PerlinNoiseParameters::InterpolationAlgorithm interpolation_algorithm = PerlinNoiseParameters::InterpolationAlgorithm::bicubic;
    int random_seed = 0;
  };

  struct EventGenerateInterpolatedTexture {
    int size[2] = {s_default_texture_size, s_default_texture_size};
    float colors[3 * 16] = {
      1, 0, 0,  1, 0, 0,  0, 0, 1,  0, 0, 1,
      1, 0, 0,  1, 1, 1,  0, 0, 1,  0, 0, 1,
      0, 1, 0,  0, 1, 0,  1, 0, 0,  1, 1, 1,
      0, 1, 0,  0, 1, 0,  1, 1, 1,  1, 1, 1
    };
    enum class Algorithm {
      bilinear, bicubic, nearest_neighboor
    } algorithm = Algorithm::bicubic;
  };

  struct EventReceiver{};

  struct EventGenerationFinished{
    double secondsTaken;
    std::chrono::steady_clock::duration realDuration;
  };
  struct EventShowInterpTruePixels : public EmptyEvent {};
  struct EventHideInterpTruePixels : public EmptyEvent {};

public:
  Menu(flecs::world&, flecs::entity menu_eid);
  void draw(flecs::world&) override;

  int m_noise_idx = 0;
  flecs::entity m_event_receiver;

private:
  flecs::query<CameraState> m_camera_state_query;
  int m_current_texture_size[2] = {0, 0};
  double m_last_generation_time_seconds = 0.0;
  std::chrono::steady_clock::duration m_last_generation_real_time{};

  EventGenerateWhiteNoiseTexture m_white_noise_params;
  EventGeneratePerlinNoiseTexture m_perlin_noise_params;
  EventGenerateInterpolatedTexture m_interpolated_texture_params;
};

