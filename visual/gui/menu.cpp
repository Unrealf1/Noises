#include "gui.hpp"

#include <imgui_inc.hpp>
#include <ecs/texture_inspection_module.hpp>
#include <array>
#include <render/noise_texture.hpp>


int new_texture_size[2];
std::array noises {"white", "perlin"};
int noise_idx = 0;

int current_texture_size[2];

static flecs::query<InspectionState> s_inspection_state_query2;

Menu::Menu(flecs::world& ecs) {
  s_inspection_state_query2 = ecs.query<InspectionState>();
  ecs.each([](NoiseTexture& texture){
    current_texture_size[0] = texture.width();
    current_texture_size[1] = texture.height();

    new_texture_size[0] = current_texture_size[0];
    new_texture_size[1] = current_texture_size[1];
  });
}

void Menu::draw(flecs::world& ecs) {
  // General info
  ImGui::Text("FPS = %f", ImGui::GetIO().Framerate);

  s_inspection_state_query2.each([](InspectionState& state) {
    ImGui::Text("Offset: %.1f, %.1f", state.x_offset, state.y_offset);
    ImGui::SameLine(0.0f, 10.0f);
    if (ImGui::Button("Reset##Offset")) {
      state.x_offset = 0.0f;
      state.y_offset = 0.0f;
    }

    ImGui::Text("Zoom: %.1f%%", state.zoom * 100.0f);
    ImGui::SameLine(0.0f, 10.0f);
    if (ImGui::Button("Reset##Zoom")) {
      state.zoom = 1.0f;
    }

    ImGui::Text("Size: %d, %d", current_texture_size[0], current_texture_size[1]);
  });
  
  ImGui::Separator();
  ImGui::Separator();

  // Generate new texture
  ImGui::ListBox("Noise type", &noise_idx, noises.data(), int(noises.size()), 3);
  // TODO: set noise type parameters

  ImGui::SliderInt2("Texture size", new_texture_size, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp);
  if (ImGui::Button("Generate")) {
    // TODO: send event to generate new texture
  }

  // TODO: show statistics? Like distribution of colors?
}

