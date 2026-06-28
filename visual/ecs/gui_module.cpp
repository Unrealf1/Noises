#include "gui_module.hpp"

#include <allegro_util.hpp>
#include <ecs/render_module.hpp>
#include <ecs/display_module.hpp>
#include <gui/gui.hpp>
#include <gui/menu.hpp>
#include <imgui_inc.hpp>


static flecs::entity s_RenderGui;
static flecs::query<DisplayHolder> s_display_query;

namespace phase {
  flecs::entity RenderGui() {
    return s_RenderGui;
  }
}

GuiModule::GuiModule(flecs::world& ecs) {
  ecs.module<GuiModule>();

  auto& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;


  s_RenderGui = ecs.entity()
    .add(flecs::Phase)
    .depends_on(phase::Render());

  phase::AfterRender()
    .depends_on(phase::RenderGui());

  s_display_query = ecs.query<DisplayHolder>();

  ecs.system("start gui frame")
    .kind(phase::RenderGui())
    .run([](flecs::iter&){
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
    .run([](flecs::iter&){
      ImGui::Render();
      s_display_query.each([](const DisplayHolder& display){
        auto targetOverride = TargetBitmapOverride(al_get_backbuffer(display.display));
        ImGui_ImplAllegro5_RenderDrawData(ImGui::GetDrawData());
      });
    });

  auto menu = ecs.entity();
  menu.set<GuiMenu>(GuiMenu{
    .title = "Menu",
    .contents = std::unique_ptr<Menu>(new Menu(ecs, menu)),
    .flags = ImGuiWindowFlags_NoResize |  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize
  });
}
