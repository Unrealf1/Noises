#include "display_module.hpp"
#include "render_module.hpp"

#include <allegro_util.hpp>
#include <app/stop.hpp>
#include <imgui_inc.hpp>

#include <log.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

static bool s_emscripten_display_size_changed = false;

static EM_BOOL on_web_display_size_changed(int /*event_type*/,const EmscriptenUiEvent */*event*/, void */* user_data */)
{
  s_emscripten_display_size_changed = true;
  warn("Display size change is detected, but not yet supported");
  return 0;
}

#endif

static flecs::entity s_SystemEvents;
static flecs::entity s_InputEventReceiver;

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


static void process_mouse_event(flecs::world& ecs, ALLEGRO_EVENT event, const flecs::entity& event_receiver) {
  if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
    ecs.event<EventMouseAxes>()
      .ctx(EventMouseAxes{event.mouse})
      .entity(event_receiver)
      .emit();
  } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
    ecs.event<EventMouseButtonDown>()
      .ctx(EventMouseButtonDown{event.mouse})
      .entity(event_receiver)
      .emit();
  } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
    ecs.event<EventMouseButtonUp>()
      .ctx(EventMouseButtonUp{event.mouse})
      .entity(event_receiver)
      .emit();
  } else if (event.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY) {
    ecs.event<EventMouseEnterDisplay>()
      .ctx(EventMouseEnterDisplay{event.mouse})
      .entity(event_receiver)
      .emit();
  } else if (event.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY) {
    ecs.event<EventMouseLeaveDisplay>()
      .ctx(EventMouseLeaveDisplay{event.mouse})
      .entity(event_receiver)
      .emit();
  } else if (event.type == ALLEGRO_EVENT_MOUSE_WARPED) {
    ecs.event<EventMouseWarped>()
      .ctx(EventMouseWarped{event.mouse})
      .entity(event_receiver)
      .emit();
  }
}

static void process_keyboard_event(flecs::world& ecs, ALLEGRO_EVENT event, const flecs::entity& event_receiver) {
  if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
    ecs.event<EventKeyDown>()
      .ctx(EventKeyDown{event.keyboard})
      .entity(event_receiver)
      .emit();
  } else if (event.type == ALLEGRO_EVENT_KEY_UP) {
    ecs.event<EventKeyUp>()
      .ctx(EventKeyUp{event.keyboard})
      .entity(event_receiver)
      .emit();
  }
}

void SystemEvents::process(flecs::world& ecs) {
  while (!empty()) {
    ALLEGRO_EVENT event;
    get(event);

    flecs::entity eventReceiver = get_input_event_receiver();
    if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
      app::stop();
      break;
    } else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
      ImGui_ImplAllegro5_InvalidateDeviceObjects();
      al_acknowledge_resize(event.display.source);
      ImGui_ImplAllegro5_CreateDeviceObjects();
      ecs.event<EventDisplayResize>()
        .ctx(EventDisplayResize{event.display})
        .entity(eventReceiver)
        .emit();
    }

    ImGui_ImplAllegro5_ProcessEvent(&event);

    if (event.any.source == al_get_mouse_event_source()) {
      if (ImGui::GetIO().WantCaptureMouse) {
        continue;
      }
      process_mouse_event(ecs, event, eventReceiver);
    } else if (event.any.source == al_get_keyboard_event_source()) {
      if (ImGui::GetIO().WantCaptureKeyboard) {
        continue;
      }
      process_keyboard_event(ecs, event, eventReceiver);
    }
  }
}

flecs::entity get_input_event_receiver() {
  return s_InputEventReceiver;
}

DisplayModule::DisplayModule(flecs::world& ecs) {
  al_set_new_display_flags(ALLEGRO_RESIZABLE);

#ifdef __EMSCRIPTEN__
  double w;
  double h;
  emscripten_get_element_css_size("#canvas", &w, &h );
  auto display = al_create_display(int(w), int(h));
#else
  ALLEGRO_MONITOR_INFO monitorInfo;
  int w = 500;
  int h = 500;
  if (al_get_monitor_info(0, &monitorInfo)) {
    w = int(float(monitorInfo.x2 - monitorInfo.x1) * 0.7f);
    h = int(float(monitorInfo.y2 - monitorInfo.x1) * 0.7f);
  }
  auto display = al_create_display(w, h);
#endif

  ImGui_ImplAllegro5_Init(display);

#ifdef __EMSCRIPTEN__
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 0, on_web_display_size_changed);
#endif

  s_InputEventReceiver = ecs.entity()
    .add<ALLEGRO_KEYBOARD_STATE>();

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

  ecs.system<ALLEGRO_KEYBOARD_STATE>("update keyboard state")
    .kind(phase::SystemEvents())
    .each([](ALLEGRO_KEYBOARD_STATE& state){
      al_get_keyboard_state(&state);
    });

  ecs.observer<DisplayHolder>()
    .event(flecs::OnRemove)
    .each([](DisplayHolder& holder) {
      al_destroy_display(holder.display);
    });
}

