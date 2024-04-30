#include "display_module.hpp"
#include "render_module.hpp"

#include <allegro_util.hpp>
#include <app/stop.hpp>
#include <imgui_inc.hpp>

#include <spdlog/spdlog.h>
#include <ecs/texture_inspection_module.hpp>


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

  void process(flecs::world& ecs);
};

static flecs::query<InspectionState> s_inspection_state_query;

void SystemEvents::process(flecs::world& ecs) {
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
      
      flecs::entity eventReceiver{};
      s_inspection_state_query.each([&eventReceiver](flecs::entity e, const InspectionState&){
        spdlog::info("found inspection state entity");
        eventReceiver = e;
      });

      if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
        spdlog::info("sending axis change to <{}>. dz is {}", eventReceiver, event.mouse.dz);
        ecs.event<EventMouseAxes>()
          .ctx(EventMouseAxes{event.mouse})
          .id<InspectionState>()
          .entity(eventReceiver)
          .emit();
      } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
        ecs.event<EventMouseButtonDown>()
          .ctx(EventMouseButtonDown{event.mouse})
          .entity(eventReceiver)
          .emit();
      } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
        ecs.event<EventMouseButtonUp>()
          .ctx(EventMouseButtonUp{event.mouse})
          .entity(eventReceiver)
          .emit();
      } else if (event.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY) {
        ecs.event<EventMouseEnterDisplay>()
          .ctx(EventMouseEnterDisplay{event.mouse})
          .entity(eventReceiver)
          .emit();
      } else if (event.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY) {
        ecs.event<EventMouseLeaveDisplay>()
          .ctx(EventMouseLeaveDisplay{event.mouse})
          .entity(eventReceiver)
          .emit();
      } else if (event.type == ALLEGRO_EVENT_MOUSE_WARPED) {
        ecs.event<EventMouseWarped>()
          .ctx(EventMouseWarped{event.mouse})
          .entity(eventReceiver)
          .emit();
      }
    }

    if (event.any.source == al_get_keyboard_event_source()) {
      if (ImGui::GetIO().WantCaptureKeyboard) {
        continue;
      }
    }
  }
}


DisplayModule::DisplayModule(flecs::world& ecs) {
  al_set_new_display_flags(ALLEGRO_RESIZABLE);
  auto display = al_create_display(500, 500);
  ImGui_ImplAllegro5_Init(display);

  s_inspection_state_query = ecs.query<InspectionState>();

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

