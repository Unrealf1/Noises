#include "gui.hpp"

#include <imgui_inc.hpp>
#include <array>


int test[2];
int test_noise_idx = 0;

void Menu::draw(flecs::world& ecs) {
  ImGui::Text("FPS = %f", ImGui::GetIO().Framerate);
  // TODO: show offset and zoom?
  // TODO: set grid size
  //   query for components? or just 
  ImGui::SliderInt2("Texture size", test, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp);
  // TODO: set noise type
  std::array noises {"white", "perlin"};
  ImGui::ListBox("Noise type", &test_noise_idx, noises.data(), noises.size(), 3);
  // TODO: set noise type parameters
  if (ImGui::Button("Generate")) {
    // TODO: send event to generate new texture
  }

  // TODO: show statistics? Like distribution of colors?
}

