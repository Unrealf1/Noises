#pragma once

#include <allegro_util.hpp>
#include <ecs/texture_inspection_module.hpp>


struct NoiseTexture {
  NoiseTexture(int width, int height);

  void set(int x, int y, ALLEGRO_COLOR color);
  ALLEGRO_COLOR get(int x, int y); // not const due to allegro interface

  int width();
  int height();

  void draw(ALLEGRO_DISPLAY*, const InspectionState&);

  void prepare_for_update();
  void prepare_for_draw();

  Bitmap m_draw_bitmap;
  Bitmap m_memory_bitmap;

  ALLEGRO_LOCKED_REGION* m_locked_memory_bitmap;
  bool m_was_modified_during_update = false;
};
