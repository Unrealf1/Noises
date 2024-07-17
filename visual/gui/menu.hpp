#pragma once

#include <gui/gui.hpp>
#include <array>
#include <ecs/texture_inspection_module.hpp>
#include <chrono>
#include <perlin.hpp>

enum class MenuNoisesIndices { perlin, interpolation, white };
static constexpr std::array s_noises {"perlin", "interpolation", "white"};

#ifdef __EMSCRIPTEN__ // browser is much slower than native code
// its nice to have default size be divided by 3, so interpolation example looks good by default
    constexpr int s_default_texture_size = 120;
#else
    constexpr int s_default_texture_size = 501;
#endif

class Menu : public GuiMenuContents {
public:
  struct EventGenerateWhiteNoiseTexture {
    int size[2] = {s_default_texture_size, s_default_texture_size};
    float black_prob = 0.5f;
  };

  struct EventGeneratePerlinNoiseTexture {
    int size[2] = {s_default_texture_size, s_default_texture_size};

#ifdef __EMSCRIPTEN__ // browser is much slower than native code
    int grid_size[2] = {10, 10};
    float grid_step[2] = {12.0f, 12.0f};
#else
    int grid_size[2] = {20, 20};
    float grid_step[2] = {25.0f, 25.0f};
#endif

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

