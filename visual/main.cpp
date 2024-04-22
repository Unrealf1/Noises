#include <noises/noises.hpp>

#include <spdlog/spdlog.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include "imgui_inc.hpp"


int main() {
  spdlog::info("visualization: noises version is {}\n", NOISES_VERSION);
  al_init();
  al_init_primitives_addon();
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
}

