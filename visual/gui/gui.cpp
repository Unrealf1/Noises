#include "gui.hpp"
#include <imgui_inc.hpp>
#include <spdlog/spdlog.h>


void GuiMenuContents::draw() { }

void GuiMenu::draw() {
  if (ImGui::Begin(title.c_str())) {
    contents->draw();
  }
  ImGui::End();
}

constexpr int maxBufferSize = 100;
static char buffer[maxBufferSize];

void TestMenu::draw() {
  ImGui::Text("FPS = %f", ImGui::GetIO().Framerate);
  if (ImGui::Button("Test button")) {
    spdlog::info("test button pressed! Now buffer is \"{}\"", std::string_view(buffer));
  }

  ImGui::InputText("test text input", buffer, maxBufferSize);
}
