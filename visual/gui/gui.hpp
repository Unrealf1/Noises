#pragma once

#include <string>
#include <memory>

#include <flecs_incl.hpp>


struct GuiMenuContents {
  virtual void draw(flecs::world&);
  virtual ~GuiMenuContents() {}
};

struct GuiMenu {
  void draw(flecs::world&);

  std::string title;
  std::unique_ptr<GuiMenuContents> contents;
  int flags = 0;
};

