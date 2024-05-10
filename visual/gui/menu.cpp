#include "menu.hpp"

#include <imgui_inc.hpp>
#include <render/noise_texture.hpp>

#include <spdlog/spdlog.h>


Menu::Menu(flecs::world& ecs, flecs::entity menu_eid) {
  m_event_receiver = menu_eid;
  m_event_receiver.add<Menu::EventReceiver>();
  m_inspection_state_query = ecs.query<InspectionState>();
  ecs.each([this](NoiseTexture& texture){
    m_current_texture_size[0] = texture.width();
    m_current_texture_size[1] = texture.height();
  });

  m_event_receiver
    .observe([&ecs, this](const Menu::EventGenerationFinished& event){
      m_last_generation_time_seconds = event.secondsTaken;
    });
}

void Menu::draw(flecs::world& ecs) {
  // General info
  ImGui::Text("FPS = %f", double(ImGui::GetIO().Framerate));

  m_inspection_state_query.each([this](InspectionState& state) {
    ImGui::Text("Offset: %.1f, %.1f", double(state.x_offset), double(state.y_offset));
    ImGui::SameLine(0.0f, 10.0f);
    if (ImGui::Button("Reset##Offset")) {
      state.x_offset = 0.0f;
      state.y_offset = 0.0f;
    }

    ImGui::Text("Zoom: %.1f%%", double(state.zoom * 100.0f));
    ImGui::SameLine(0.0f, 10.0f);
    if (ImGui::Button("Reset##Zoom")) {
      state.zoom = 1.0f;
    }

    ImGui::Text("Size: %d, %d", m_current_texture_size[0], m_current_texture_size[1]);

    auto [timeName, timeValue] = [this]{
      return m_last_generation_time_seconds >= 1.0 ?
        std::pair("s", m_last_generation_time_seconds) : m_last_generation_time_seconds >= 1e-3 ?
        std::pair("ms", m_last_generation_time_seconds * 1e3) : 
        std::pair("us", m_last_generation_time_seconds * 1e6);
    }();
    ImGui::Text("Generated in %.1f %s", timeValue, timeName);
  });
  
  ImGui::Separator();
  ImGui::Separator();

  // Generate new texture
  ImGui::ListBox("Noise type", &(m_noise_idx), s_noises.data(), int(s_noises.size()), 3);
  if (m_noise_idx == int(MenuNoisesIndices::white)) {
    ImGui::SliderInt2("Texture size", m_white_noise_params.size, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat("Black probability", &m_white_noise_params.black_prob, 0.0f, 1.0f);

  } else if (m_noise_idx == int(MenuNoisesIndices::perlin)) {
    ImGui::SliderInt2("Texture size", m_perlin_noise_params.size, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderInt2("Vector grid size", m_perlin_noise_params.grid_size, 1, 10000);
    ImGui::SliderFloat2("Grid step", m_perlin_noise_params.grid_step, 0.1f, 10000.0f);
    ImGui::Checkbox("Normalize offset vectors", &m_perlin_noise_params.normalize_offsets);
    const char* algorithms[] = {"bilinear", "bicubic", "nearest neighboor"};
    int algo = int(m_perlin_noise_params.interpolation_algorithm);
    ImGui::ListBox("Interpolation", &algo, algorithms, sizeof(algorithms) / sizeof(const char*), 3);
    m_perlin_noise_params.interpolation_algorithm = PerlinNoiseParameters::InterpolationAlgorithm(algo);

  } else if (m_noise_idx == int(MenuNoisesIndices::interpolation)) {
    ImGui::SliderInt2("Texture size", m_perlin_noise_params.size, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp);
    for (int row = 0; row <= 3; ++row) {
      for (int column = 0; column <= 3; ++column) {
        spdlog::info("{} , {}", row, column);
        ImGui::ColorEdit3("", &m_interpolated_texture_params.colors[(row * 4 + column) * 3]);
        if (column != 3) {
          ImGui::SameLine();
        }
      }
    }
    const char* algorithms[] = {"bilinear", "nearest neighboor"};
    int algo = int(m_interpolated_texture_params.algorithm);
    ImGui::ListBox("Algorithm", &algo, algorithms, sizeof(algorithms) / sizeof(const char*), 3);
    m_interpolated_texture_params.algorithm = Menu::EventGenerateInterpolatedTexture::Algorithm(algo);
  }

  if (ImGui::Button("Generate")) {
    if (m_noise_idx == int(MenuNoisesIndices::white)) {
      ecs.event<Menu::EventGenerateWhiteNoiseTexture>()
        .ctx(m_white_noise_params)
        .entity(m_event_receiver)
        .emit();
      m_current_texture_size[0] = m_white_noise_params.size[0];
      m_current_texture_size[1] = m_white_noise_params.size[1];
    } else if (m_noise_idx == int(MenuNoisesIndices::perlin)) {
      ecs.event<Menu::EventGeneratePerlinNoiseTexture>()
        .ctx(m_perlin_noise_params)
        .entity(m_event_receiver)
        .emit();
      m_current_texture_size[0] = m_perlin_noise_params.size[0];
      m_current_texture_size[1] = m_perlin_noise_params.size[1];
    } else if (m_noise_idx == int(MenuNoisesIndices::interpolation)) {
      ecs.event<Menu::EventGenerateInterpolatedTexture>()
        .ctx(m_interpolated_texture_params)
        .entity(m_event_receiver)
        .emit();
      m_current_texture_size[0] = m_interpolated_texture_params.size[0];
      m_current_texture_size[1] = m_interpolated_texture_params.size[1];
    }
  }

  // TODO: show statistics? Like distribution of colors?
}

