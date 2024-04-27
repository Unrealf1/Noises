#include <flecs.h>

namespace phase {
  flecs::entity BeforeRender();
  flecs::entity Render();
  flecs::entity AfterRender();
}

struct RenderModule {
  RenderModule(flecs::world&);
  ~RenderModule();
};

