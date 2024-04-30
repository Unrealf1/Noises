#include "texture_inspection_module.hpp"
#include <ecs/display_module.hpp>
#include <algorithm>

#include <spdlog/spdlog.h>

TextureInspectionModule::TextureInspectionModule(flecs::world& ecs) {
  ecs.module<TextureInspectionModule>();

  ecs.entity()
    .emplace<InspectionState>();

  ecs.observer<InspectionState>()
    .event<EventMouseButtonDown>()
    .iter([](flecs::iter& it, InspectionState* inspection_state) {
      auto& eventData = *it.event().get<EventMouseButtonDown>();
      spdlog::info("got mouse button down (pre). sz");
      for (auto _ : it) {
        if (eventData.button == 1) {
          spdlog::info("got mouse button down");
          inspection_state->is_dragging = true;
        }
      }
    });

  ecs.observer<InspectionState>()
    .event<EventMouseButtonUp>()
    .iter([](flecs::iter& it, InspectionState* inspection_state){
      auto& eventData = *it.event().get<EventMouseButtonUp>();
      for (auto _ : it) {
        if (eventData.button == 1) {
          spdlog::info("got mouse button up");
          inspection_state->is_dragging = false;
        }
      }
    });

  ecs.observer<InspectionState>()
    .event<EventMouseAxes>()  
    .iter([](flecs::iter& it, InspectionState* inspection_state){
      auto& eventData = *it.event().get<EventMouseAxes>();
      for (auto _ : it) {
        spdlog::info("got mouse axis change. dz is {}", eventData.dz);
        if (eventData.dz > 0) {
          inspection_state->zoom *= inspection_state->zoom_step;
          inspection_state->zoom = std::clamp(inspection_state->zoom, inspection_state->min_zoom, inspection_state->max_zoom);
        } else if (eventData.dx < 0) {
          inspection_state->zoom /= inspection_state->zoom_step;
          inspection_state->zoom = std::clamp(inspection_state->zoom, inspection_state->min_zoom, inspection_state->max_zoom);
        }

        if (inspection_state->is_dragging) {
          inspection_state->x_offset += eventData.dx;
          inspection_state->y_offset += eventData.dy;
        }
      }
    });
}