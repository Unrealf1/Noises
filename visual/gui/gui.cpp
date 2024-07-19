#include "gui.hpp"
#include <imgui_inc.hpp>


void GuiMenuContents::draw(flecs::world&) { }

void GuiMenu::draw(flecs::world& ecs) {
  // TODO: move out of common code
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  if (ImGui::Begin(title.c_str(), nullptr, flags)) {
    contents->draw(ecs);
  }
  ImGui::End();
}

