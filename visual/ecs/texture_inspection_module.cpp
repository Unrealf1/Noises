#include "texture_inspection_module.hpp"
#include <ecs/display_module.hpp>
#include <algorithm>
#include <gui/menu.hpp>

#include <spdlog/spdlog.h>

static flecs::query<InspectionState> s_inspection_state_query;

TextureInspectionModule::TextureInspectionModule(flecs::world& ecs) {
  ecs.module<TextureInspectionModule>();

  s_inspection_state_query = ecs.query<InspectionState>();

  InspectionState initialState{};
  ecs.each([&initialState](const DisplayHolder& holder) {
    auto w = al_get_display_width(holder.display);
    auto h = al_get_display_height(holder.display);
    initialState.x_offset = float(w - s_default_texture_size) / 2.0f;
    initialState.y_offset = float(h - s_default_texture_size) / 2.0f;
  });

  ecs.entity()
    .emplace<InspectionState>(initialState);

  get_input_event_receiver()
    .observe([](EventMouseButtonDown event){
      if (event.button != 1) {
        return;
      }
      s_inspection_state_query.each([](InspectionState& inspection_state){
         inspection_state.is_dragging = true; 
      });
    });

  get_input_event_receiver()
    .observe([](EventMouseButtonUp event){
      if (event.button != 1) {
        return;
      }
      s_inspection_state_query.each([](InspectionState& inspection_state){
         inspection_state.is_dragging = false; 
      });
    });

  get_input_event_receiver()
    .observe([](EventMouseAxes event){
      s_inspection_state_query.each([&event](InspectionState& inspection_state){
        if (event.dz > 0) {
          inspection_state.zoom *= inspection_state.zoom_step;
          inspection_state.zoom = std::clamp(inspection_state.zoom, inspection_state.min_zoom, inspection_state.max_zoom);
        } else if (event.dz < 0) {
          inspection_state.zoom /= inspection_state.zoom_step;
          inspection_state.zoom = std::clamp(inspection_state.zoom, inspection_state.min_zoom, inspection_state.max_zoom);
        }

        if (inspection_state.is_dragging) {
          inspection_state.x_offset += float(event.dx);
          inspection_state.y_offset += float(event.dy);
        }
      });
    });

  ecs.system<ALLEGRO_KEYBOARD_STATE>("update keyboard panning state")
    .kind(phase::SystemEvents())
    .each([](const ALLEGRO_KEYBOARD_STATE& state){
      s_inspection_state_query.each([&state](InspectionState& inspection_state){
        bool left = al_key_down(&state, ALLEGRO_KEY_LEFT);
        bool right = al_key_down(&state, ALLEGRO_KEY_RIGHT);
        bool up = al_key_down(&state, ALLEGRO_KEY_UP);
        bool down = al_key_down(&state, ALLEGRO_KEY_DOWN);
        bool speedup = al_key_down(&state, ALLEGRO_KEY_LSHIFT) || al_key_down(&state, ALLEGRO_KEY_RSHIFT);

        inspection_state.keyboard_pan_speed = speedup
          ? inspection_state.keyboard_pan_speed_fast
          : inspection_state.keyboard_pan_speed_normal;

        inspection_state.keyboard_pan_dx = (left ? -1.0f : 0.0f) + (right ? 1.0f : 0.0f);
        inspection_state.keyboard_pan_dy = (up ? -1.0f : 0.0f) + (down ? 1.0f : 0.0f);
      });
    });

  ecs.system<InspectionState>("pan via keyboard")
    .kind(flecs::OnUpdate)
    .each([](const flecs::iter& it, size_t, InspectionState& state){
      if (state.is_dragging) {
        return; // Mouse override keyboard panning
      }

      float dx = state.keyboard_pan_dx;
      float dy = state.keyboard_pan_dy;
      float len = std::sqrt(dx * dx + dy * dy);
      if (len < 1e-6f) {
        return;
      }

      auto offsetMult = state.keyboard_pan_speed / len * it.delta_time();
      state.x_offset += dx * offsetMult;
      state.y_offset += dy * offsetMult;
    });
}
