#pragma once

#include <gui/gui.hpp>
#include <array>
#include <ecs/texture_inspection_module.hpp>


static constexpr std::array s_noises {"white", "perlin"};

class Menu : public GuiMenuContents {
public:
  struct EventGenerateWhiteNoiseTexture {
    int size[2];
    float black_prob;
  };

public:
  Menu(flecs::world&, flecs::entity menu_eid);
  void draw(flecs::world&) override;

  int m_new_texture_size[2] = {500, 500};
  int m_noise_idx = 0;
  flecs::entity m_event_receiver;

private:
  flecs::query<InspectionState> m_inspection_state_query;
  int m_current_texture_size[2] = {0, 0};
};

