#include "render.hpp"
#include "imgui_inc.hpp"


namespace render {
  static EventSourceBeforeFrame s_before_frame;
  static EventSourceDrawFrame s_draw_frame;

  void init() {
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    al_init_user_event_source(&s_before_frame);
    al_init_user_event_source(&s_draw_frame);
  }

  void uninit() {
    al_destroy_user_event_source(&s_before_frame);
    al_destroy_user_event_source(&s_draw_frame);
  }

  ALLEGRO_EVENT_SOURCE* get_before_frame_event_source() {
    return &s_before_frame;
  }

  ALLEGRO_EVENT_SOURCE* get_draw_frame_event_source() {
    return &s_before_frame;
  }

  void start_frame() {
    ALLEGRO_USER_EVENT event{};
    al_emit_user_event(&s_before_frame, &event, nullptr);
    al_clear_color(al_map_rgb(0, 0, 0));
  }

  void draw_frame() {
    ALLEGRO_USER_EVENT event{};
    al_emit_user_event(&s_draw_frame, &event, nullptr);
  }

  void finish_frame() {
    al_flip_display();
  }

  static void process_system_events(EventReactor& system_events, bool& need_to_stop) {
    while (!system_events.empty()) {
      ALLEGRO_EVENT event;
      system_events.get(event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
          need_to_stop = true;
          break;
      } else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
          ImGui_ImplAllegro5_InvalidateDeviceObjects();
          al_acknowledge_resize(event.display.source);
          ImGui_ImplAllegro5_CreateDeviceObjects();
      }
    }
  }

  void render_loop(ALLEGRO_DISPLAY* display) {
    auto systemEvents = EventReactor();
    systemEvents.register_source(al_get_keyboard_event_source());
    systemEvents.register_source(al_get_display_event_source(display));

    bool needToStop = false;

    while (!needToStop) {
      start_frame();
      draw_frame();
      finish_frame();
      process_system_events(systemEvents, needToStop);
    }
  }
}
