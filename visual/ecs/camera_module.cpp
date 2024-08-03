#include "camera_module.hpp"
#include <ecs/display_module.hpp>
#include <ecs/render_module.hpp>
#include <algorithm>

#include <log.hpp>

static flecs::query<CameraState> s_camera_state_query;

CameraModule::CameraModule(flecs::world& ecs) {
  ecs.module<CameraModule>();

  s_camera_state_query = ecs.query<CameraState>();

  CameraState cameraState{};
  ecs.each([&cameraState](const DisplayHolder& holder) {
    auto w = al_get_display_width(holder.display);
    auto h = al_get_display_height(holder.display);
    cameraState.display_dimentions = {w, h};
  });

  ecs.entity()
    .emplace<CameraState>(cameraState);

  get_input_event_receiver()
    .observe([](EventDisplayResize event){
      s_camera_state_query.each([&event](CameraState& camera_state){
        camera_state.display_dimentions = vec2{event.width, event.height};
      });
    });

  get_input_event_receiver()
    .observe([](EventMouseButtonDown event){
      if (event.button != 1) {
        return;
      }
      s_camera_state_query.each([](CameraState& camera_state){
        camera_state.is_dragging = true; 
      });
    });

  get_input_event_receiver()
    .observe([](EventMouseButtonUp event){
      if (event.button != 1) {
        return;
      }
      s_camera_state_query.each([](CameraState& camera_state){
         camera_state.is_dragging = false; 
      });
    });

  get_input_event_receiver()
    .observe([](EventMouseAxes event){
      s_camera_state_query.each([&event](CameraState& camera_state){
        if (event.dz > 0) {
          camera_state.zoom *= camera_state.zoom_step;
          camera_state.zoom = std::clamp(camera_state.zoom, camera_state.min_zoom, camera_state.max_zoom);
        } else if (event.dz < 0) {
          camera_state.zoom /= camera_state.zoom_step;
          camera_state.zoom = std::clamp(camera_state.zoom, camera_state.min_zoom, camera_state.max_zoom);
        }

        if (camera_state.is_dragging) {
          camera_state.center -= vec2{event.dx, event.dy} / camera_state.zoom;
        }
      });
    });

  ecs.system<ALLEGRO_KEYBOARD_STATE>("update keyboard panning state")
    .kind(phase::SystemEvents())
    .each([](const ALLEGRO_KEYBOARD_STATE& state){
      s_camera_state_query.each([&state](CameraState& camera_state){
        bool left = al_key_down(&state, ALLEGRO_KEY_LEFT);
        bool right = al_key_down(&state, ALLEGRO_KEY_RIGHT);
        bool up = al_key_down(&state, ALLEGRO_KEY_UP);
        bool down = al_key_down(&state, ALLEGRO_KEY_DOWN);
        bool speedup = al_key_down(&state, ALLEGRO_KEY_LSHIFT) || al_key_down(&state, ALLEGRO_KEY_RSHIFT);

        camera_state.keyboard_pan_speed = speedup
          ? camera_state.keyboard_pan_speed_fast
          : camera_state.keyboard_pan_speed_normal;

        camera_state.keyboard_pan = {
          .x = (left ? -1.0f : 0.0f) + (right ? 1.0f : 0.0f),
          .y = (up ? -1.0f : 0.0f) + (down ? 1.0f : 0.0f)
        };
      });
    });

  ecs.system<CameraState>("pan via keyboard")
    .kind(flecs::OnUpdate)
    .each([](const flecs::iter& it, size_t, CameraState& state){
      if (state.is_dragging) {
        return; // Mouse override keyboard panning
      }

      float len = state.keyboard_pan.length();
      if (len < 1e-6f) {
        return;
      }

      auto offsetMult = state.keyboard_pan_speed / len * it.delta_time();
      state.center += state.keyboard_pan * offsetMult;
    });

  ecs.system<CameraState>("Calculate view box")
    .kind(phase::BeforeRender())
    .each([](CameraState& state){
      auto zoomedDims = state.display_dimentions / state.zoom;
      state.view = {
        state.center - zoomedDims / 2.0f,
        state.center + zoomedDims / 2.0f
      };
    });
}
