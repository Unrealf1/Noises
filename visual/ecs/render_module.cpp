#include "render_module.hpp"

#include <render/render.hpp>

static flecs::entity s_BeforeRender;
static flecs::entity s_Render;
static flecs::entity s_AfterRender;

namespace phase {
  flecs::entity BeforeRender() {
    return s_BeforeRender;
  }

  flecs::entity Render() {
    return s_Render;
  }

  flecs::entity AfterRender() {
    return s_AfterRender;
  }
}

RenderModule::RenderModule(flecs::world& ecs) {
  render::init();

  ecs.module<RenderModule>();

  // 1. create before/ in / after render phases
  s_BeforeRender = ecs.entity()
    .add(flecs::Phase)
    .depends_on(flecs::PostUpdate);

  s_Render = ecs.entity()
    .add(flecs::Phase)
    .depends_on(phase::BeforeRender());

  s_AfterRender = ecs.entity()
    .add(flecs::Phase)
    .depends_on(phase::Render());


  // 2. create systems for frame start and end
  ecs.system("start_render")
    .kind(phase::BeforeRender())
    .iter([](flecs::iter&) {
      render::start_frame();
    });

  ecs.system("finish_render")
    .kind(phase::AfterRender())
    .iter([](flecs::iter&) {
      render::finish_frame();
    });
}

RenderModule::~RenderModule() {
  render::uninit();
}

