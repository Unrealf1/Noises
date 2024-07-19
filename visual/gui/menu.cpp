#include "menu.hpp"

#include <imgui_inc.hpp>
#include <render/noise_texture.hpp>
#include <format>
#include <log.hpp>
#ifndef __EMSCRIPTEN__
#include <ImGuiFileDialog.h>
#endif



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
      m_last_generation_real_time = event.realDuration;
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
    ImGui::Text("Generated in %.1f %s CPU time", timeValue, timeName);
    if (m_last_generation_real_time.count() > 0) {
      auto s = std::chrono::duration_cast<std::chrono::seconds>(m_last_generation_real_time);
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_last_generation_real_time);
      auto us = std::chrono::duration_cast<std::chrono::microseconds>(m_last_generation_real_time);
      auto [value, name] = ms.count() == 0 ? std::pair{float(us.count()), "us"}
                           : s.count() == 0 ? std::pair{float(us.count()) / 1000.0f, "ms"}
                           : std::pair{float(ms.count()) / 1000.0f, "s"};
      ImGui::SameLine();
      ImGui::Text("%.1f %s real time", value, name);
    }
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
    const char* algorithms[] = {"bilinear", "bicubic (derivative from grid)", "bicubic (zero derivative)", "nearest neighboor"};
    int algo = int(m_perlin_noise_params.interpolation_algorithm);
    ImGui::ListBox("Interpolation", &algo, algorithms, sizeof(algorithms) / sizeof(const char*), 3);
    ImGui::SliderInt("Random seed", &m_perlin_noise_params.random_seed, 0, 10000);
    m_perlin_noise_params.interpolation_algorithm = PerlinNoiseParameters::InterpolationAlgorithm(algo);

  } else if (m_noise_idx == int(MenuNoisesIndices::interpolation)) {
    ImGui::SliderInt2("Texture size", m_interpolated_texture_params.size, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp);
    static const char* labels[16] = {
      "ColorPicker1",
      "ColorPicker2",
      "ColorPicker3",
      "ColorPicker4",
      "ColorPicker5",
      "ColorPicker6",
      "ColorPicker7",
      "ColorPicker8",
      "ColorPicker9",
      "ColorPicker10",
      "ColorPicker11",
      "ColorPicker12",
      "ColorPicker13",
      "ColorPicker14",
      "ColorPicker15",
      "ColorPicker16"
    };
    for (int row = 0; row <= 3; ++row) {
      for (int column = 0; column <= 3; ++column) {
        int index = (row * 4 + column) * 3;
        ImGui::ColorEdit3(labels[index / 3], &m_interpolated_texture_params.colors[index], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        if (column != 3) {
          ImGui::SameLine();
        }
      }
    }
    const char* algorithms[] = {"bilinear", "bicubic", "nearest neighboor"};
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

#ifndef __EMSCRIPTEN__
  ImGui::SameLine();
  const char* const fileDialogKey = "draw_save_dialog_key";
  if (ImGui::Button("Save")) {
    ImGuiFileDialog::Instance()->OpenDialog(fileDialogKey, "Choose file", ".png,.jpg,.webp", ".", 1, nullptr, ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite);
  }

  if (ImGuiFileDialog::Instance()->Display(fileDialogKey)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      ecs.each([&](NoiseTexture& texture){
        info("saving to \"{}\"", ImGuiFileDialog::Instance()->GetFilePathName());
        bool didSave = al_save_bitmap(ImGuiFileDialog::Instance()->GetFilePathName().c_str(), texture.m_draw_bitmap.get_raw());
        if (!didSave) {
          error("Failed to save texture.");
        }
      });
    }

    ImGuiFileDialog::Instance()->Close();
  }
#endif

  // TODO: show statistics? Like distribution of colors?
}

