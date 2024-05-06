#include "menu.hpp"

#include <imgui_inc.hpp>
#include <render/noise_texture.hpp>


Menu::Menu(flecs::world& ecs, flecs::entity menu_eid) {
  m_event_receiver = menu_eid;
  m_inspection_state_query = ecs.query<InspectionState>();
  ecs.each([this](NoiseTexture& texture){
    m_current_texture_size[0] = texture.width();
    m_current_texture_size[1] = texture.height();

    m_new_texture_size[0] = m_current_texture_size[0];
    m_new_texture_size[1] = m_current_texture_size[1];
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
  });
  
  ImGui::Separator();
  ImGui::Separator();

  // Generate new texture
  ImGui::ListBox("Noise type", &(m_noise_idx), s_noises.data(), int(s_noises.size()), 3);
  // TODO: set noise type parameters

  ImGui::SliderInt2("Texture size", m_new_texture_size, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp);
  if (ImGui::Button("Generate")) {
    ecs.event<Menu::EventGenerateWhiteNoiseTexture>()
      .ctx(Menu::EventGenerateWhiteNoiseTexture{
        .size = {m_new_texture_size[0], m_new_texture_size[1]},
        .black_prob = 0.5f
      })
      .entity(m_event_receiver)
      .emit();
    m_current_texture_size[0] = m_new_texture_size[0];
    m_current_texture_size[1] = m_new_texture_size[1];
  }

  // TODO: show statistics? Like distribution of colors?
}

