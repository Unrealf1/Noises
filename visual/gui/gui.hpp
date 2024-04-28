#pragma once

#include <string>
#include <memory>


struct GuiMenuContents {
  virtual void draw();
};

struct TestMenu : GuiMenuContents {
  void draw() override;
};

struct GuiMenu {
  void draw();

  std::string title;
  std::unique_ptr<GuiMenuContents> contents;
};
