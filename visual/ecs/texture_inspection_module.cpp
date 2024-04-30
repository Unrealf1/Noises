#include "texture_inspection_module.hpp"
#include <ecs/display_module.hpp>
#include <algorithm>

#include <spdlog/spdlog.h>

static flecs::query<InspectionState> s_inspection_state_query;

TextureInspectionModule::TextureInspectionModule(flecs::world& ecs) {
  ecs.module<TextureInspectionModule>();

  s_inspection_state_query = ecs.query<InspectionState>();

  ecs.entity()
    .emplace<InspectionState>();

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
        auto oldZoom = inspection_state.zoom;
        if (event.dz > 0) {
          inspection_state.zoom *= inspection_state.zoom_step;
          inspection_state.zoom = std::clamp(inspection_state.zoom, inspection_state.min_zoom, inspection_state.max_zoom);
        } else if (event.dz < 0) {
          inspection_state.zoom /= inspection_state.zoom_step;
          inspection_state.zoom = std::clamp(inspection_state.zoom, inspection_state.min_zoom, inspection_state.max_zoom);
        }

        if (inspection_state.is_dragging) {
          inspection_state.x_offset += event.dx;
          inspection_state.y_offset += event.dy;
        }
      });
    });
}
