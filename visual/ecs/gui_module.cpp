#include "gui_module.hpp"

#include <allegro_util.hpp>
#include <ecs/render_module.hpp>
#include <gui/gui.hpp>
#include <imgui_inc.hpp>


static flecs::entity s_RenderGui;

namespace phase {
  flecs::entity RenderGui() {
    return s_RenderGui;
  }
}

GuiModule::GuiModule(flecs::world& ecs) {
  ecs.module<GuiModule>();

  s_RenderGui = ecs.entity()
    .add(flecs::Phase)
    .depends_on(phase::Render());

  phase::AfterRender()
    .depends_on(phase::RenderGui());

  ecs.system("start gui frame")
    .kind(phase::RenderGui())
    .iter([](flecs::iter&){
      ImGui_ImplAllegro5_NewFrame();
      ImGui::NewFrame();
    });

  ecs.system<GuiMenu>("draw ecs menus")
    .kind(phase::RenderGui())
    .each([&ecs](GuiMenu& menu){
      menu.draw(ecs);
    });

  ecs.system("finish gui frame")
    .kind(phase::RenderGui())
    .iter([](flecs::iter&){
      ImGui::Render();
      ImGui_ImplAllegro5_RenderDrawData(ImGui::GetDrawData());
    });
}
