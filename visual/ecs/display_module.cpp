#include "display_module.hpp"
#include "render_module.hpp"

#include <allegro_util.hpp>
#include <app/stop.hpp>
#include <imgui_inc.hpp>


static flecs::entity s_SystemEvents;

namespace phase {
  flecs::entity SystemEvents() {
    return s_SystemEvents;
  }
}

struct SystemEvents : public EventQueue {
  SystemEvents(ALLEGRO_DISPLAY* display) : EventQueue() {
    register_source(al_get_display_event_source(display));
    register_source(al_get_mouse_event_source());
    register_source(al_get_keyboard_event_source());
  }

  void process(flecs::world& ecs) {
    while (!empty()) {
      ALLEGRO_EVENT event;
      get(event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
        app::stop();
        break;
      } else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
        ImGui_ImplAllegro5_InvalidateDeviceObjects();
        al_acknowledge_resize(event.display.source);
        ImGui_ImplAllegro5_CreateDeviceObjects();
      }

      ImGui_ImplAllegro5_ProcessEvent(&event);
      if (event.any.source == al_get_mouse_event_source()) {
        if (ImGui::GetIO().WantCaptureMouse) {
          continue;
        }

        if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
          auto ecsEvent = ecs.entity()
            .emplace<EventMouseAxes>(event.mouse);
          ecs.event(ecsEvent).emit();
        } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
          auto ecsEvent = ecs.entity()
            .emplace<EventMouseButtonDown>(event.mouse);
          ecs.event(ecsEvent).emit();
        } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
          auto ecsEvent = ecs.entity()
            .emplace<EventMouseButtonUp>(event.mouse);
          ecs.event(ecsEvent).emit();
        } else if (event.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY) {
          auto ecsEvent = ecs.entity()
            .emplace<EventMouseEnterDisplay>(event.mouse);
          ecs.event(ecsEvent).emit();
        } else if (event.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY) {
          auto ecsEvent = ecs.entity()
            .emplace<EventMouseLeaveDisplay>(event.mouse);
          ecs.event(ecsEvent).emit();
        } else if (event.type == ALLEGRO_EVENT_MOUSE_WARPED) {
          auto ecsEvent = ecs.entity()
            .emplace<EventMouseWarped>(event.mouse);
          ecs.event(ecsEvent).emit();
        }
      }

      if (event.any.source == al_get_keyboard_event_source()) {
        if (ImGui::GetIO().WantCaptureKeyboard) {
          continue;
        }
      }
    }
  }
};

DisplayModule::DisplayModule(flecs::world& ecs) {
  al_set_new_display_flags(ALLEGRO_RESIZABLE);
  auto display = al_create_display(500, 500);
  ImGui_ImplAllegro5_Init(display);

  ecs.module<DisplayModule>();   

  ecs.entity("Display")
    .set<DisplayHolder>({.display = display});

  ecs.entity("System Events")
    .emplace<SystemEvents>(display);

  s_SystemEvents = ecs.entity()
    .add(flecs::Phase)
    .depends_on(phase::AfterRender());

  ecs.system<SystemEvents>("process_system events")
    .kind(phase::SystemEvents())
    .each([&ecs](SystemEvents& events) {
       events.process(ecs);
    });

  ecs.observer<DisplayHolder>()
    .event(flecs::OnRemove)
    .each([](DisplayHolder& holder) {
      al_destroy_display(holder.display);
    });
}

