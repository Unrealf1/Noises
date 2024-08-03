#include "render_module.hpp"

#include <render/render.hpp>
#include <render/drawable_bitmap.hpp>
#include <ecs/camera_module.hpp>
#include <ecs/display_module.hpp>

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

static flecs::query<DrawableBitmap> s_drawable_bitmap_query;
static flecs::query<DisplayHolder> s_display_query;

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

  s_drawable_bitmap_query = ecs.query<DrawableBitmap>();
  s_display_query = ecs.query<DisplayHolder>();

  // 2. create systems for frame start and end
  ecs.system("start_render")
    .kind(phase::BeforeRender())
    .iter([](flecs::iter&) {
      render::start_frame();
    });

  ecs.system<CameraState>("render drawable bitmaps")
    .kind(phase::Render())
    .each([](const CameraState& camera){
      s_display_query.each([&camera](const DisplayHolder& display){
        auto displayBitmap = al_get_backbuffer(display.display);
        auto targetOverride = TargetBitmapOverride(displayBitmap);

        s_drawable_bitmap_query.each([&camera](DrawableBitmap& drawable){
          // 1. calculate bitmap position on the screen
          auto bitmapDims = vec2{float(drawable.bitmap.width()), float(drawable.bitmap.height())};
          auto zoomedDims = bitmapDims * camera.zoom;

          auto screenTopLeft = drawable.top_left - camera.view.top_left;
          auto zoomedTopLeft = screenTopLeft * camera.zoom;

          // 2. check that it is in the screen
          const auto drawableBounds = Box2{ drawable.top_left, drawable.top_left + bitmapDims };
          if (!camera.view.intersects(drawableBounds)) {
            return;
          }

          // 3. draw
          al_draw_scaled_bitmap(drawable.bitmap.get_raw(),
            0.0f, 0.0f,
            bitmapDims.x, bitmapDims.y,
            zoomedTopLeft.x, zoomedTopLeft.y,
            zoomedDims.x, zoomedDims.y, 0);
        });
      });
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

