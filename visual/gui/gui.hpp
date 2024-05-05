#pragma once

#include <string>
#include <memory>

#include <flecs.h>


struct GuiMenuContents {
  virtual void draw(flecs::world&);
};

struct GuiMenu {
  void draw(flecs::world&);

  std::string title;
  std::unique_ptr<GuiMenuContents> contents;
  int flags = 0;
};

