#include "render.hpp"
#include "imgui_inc.hpp"

#include <allegro_util.hpp>
#include <allegro5/allegro_image.h>


namespace render {
  void init() {
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    al_init_image_addon();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
  }

  void uninit() { }

  void start_frame() {
    al_clear_to_color(al_map_rgb(0, 0, 0));
  }

  void finish_frame() {
    al_flip_display();
  }
}

