#include "gui.hpp"
#include <imgui_inc.hpp>
#include <spdlog/spdlog.h>


void GuiMenuContents::draw(flecs::world&) { }

void GuiMenu::draw(flecs::world& ecs) {
  if (ImGui::Begin(title.c_str(), nullptr, flags)) {
    contents->draw(ecs);
  }
  ImGui::End();
}

