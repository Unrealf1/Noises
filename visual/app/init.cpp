#include "init.hpp"

#include <gui/menu.hpp>
#include <imgui_inc.hpp>
#include <render/noise_texture.hpp>
#include <random>
#include <ecs/render_module.hpp>
#include <ecs/display_module.hpp>
#include <ecs/gui_module.hpp>
#include <ecs/texture_inspection_module.hpp>


static flecs::query<DisplayHolder> s_display_query;
static flecs::query<InspectionState> s_inspection_state_query;
static flecs::query<Menu> s_menu_query;
static flecs::query<NoiseTexture> s_noise_texture_query;

static flecs::entity s_menu_event_receiver;

void init_queries(flecs::world& ecs) {
  s_display_query = ecs.query<DisplayHolder>();
  s_inspection_state_query = ecs.query<InspectionState>();
  s_menu_query = ecs.query<Menu>();
  s_noise_texture_query = ecs.query<NoiseTexture>();
}

void import_modules(flecs::world& ecs) {
  ecs.import<RenderModule>();
  ecs.import<DisplayModule>();
  ecs.import<GuiModule>();
  ecs.import<TextureInspectionModule>();
}

void create_systems(flecs::world& ecs) {
  // TODO: store display pointer somewhere in the NoiseTexture?
  ecs.system<NoiseTexture>("Draw noise texture")
    .kind(phase::Render())
    .each([&](NoiseTexture& texture){
      s_display_query.each([&](const DisplayHolder& holder){
        s_inspection_state_query.each([&](const InspectionState& inspectionState){
          texture.draw(holder.display, inspectionState);
        });
      });
    });

  ecs.system<NoiseTexture>("Prepare noise texture draw")
    .kind(phase::BeforeRender())
    .each([](NoiseTexture& texture){
      texture.prepare_for_draw();
    });

  ecs.system<NoiseTexture>("Prepare noise texture update")
    .kind(flecs::PreUpdate)
    .each([](NoiseTexture& texture) {
      texture.prepare_for_update();
    });
}

void create_entities(flecs::world& ecs) {
  ecs.entity()
    .emplace<NoiseTexture>(500, 500);

  s_menu_event_receiver = ecs.entity();
  s_menu_event_receiver.set<GuiMenu>(GuiMenu{
    .title = "Menu",
    .contents = std::unique_ptr<Menu>(new Menu(ecs, s_menu_event_receiver)),
    .flags = ImGuiWindowFlags_NoTitleBar |  ImGuiWindowFlags_NoResize |  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize
  });
}

void create_observers(flecs::world& ecs) {
  s_menu_event_receiver
    .observe([&ecs](const Menu::EventGenerateWhiteNoiseTexture& event){
      s_noise_texture_query.each([](flecs::entity entity, const NoiseTexture&){
        entity.destruct();
      });
      NoiseTexture texture(event.size[0], event.size[1]);

      std::random_device dev{};
      std::default_random_engine eng(dev());
      std::bernoulli_distribution distr(event.black_prob);

      auto white = al_map_rgb(255, 255, 255);
      auto black = al_map_rgb(0, 0, 0);
      auto width = texture.width();
      auto height = texture.height();
      for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
          auto r = distr(eng) ? 255 : 0;
          auto g = distr(eng) ? 255 : 0;
          auto b = distr(eng) ? 255 : 0;
          texture.set(x, y, al_map_rgb(r, g, b));
        }
      }
      ecs.entity().emplace<NoiseTexture>(std::move(texture));
    });
}

void app::init(flecs::world& ecs) {
  init_queries(ecs);
  import_modules(ecs);
  // TODO: move all systems & observers inside modules?
  create_systems(ecs);
  create_entities(ecs);
  create_observers(ecs);
}

