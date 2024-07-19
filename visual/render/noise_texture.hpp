#pragma once

#include <allegro_util.hpp>
#include <ecs/texture_inspection_module.hpp>
#include <shared_mutex>
#include <memory>
#include <atomic>


struct NoiseTexture {
  NoiseTexture(int width, int height);

  void set(int x, int y, ALLEGRO_COLOR color);

  TargetBitmapOverride scoped_write_to_memory_bitmap();
  void mark_modified();

  ALLEGRO_COLOR get(int x, int y); // not const due to allegro interface

  int width();
  int height();

  void draw(ALLEGRO_DISPLAY*, const InspectionState&);

  void prepare_for_update();
  void prepare_for_draw();

  Bitmap m_draw_bitmap;
  Bitmap m_memory_bitmap;

  ALLEGRO_LOCKED_REGION* m_locked_memory_bitmap;
  bool m_was_modified_during_update = true; // to update gpu state on creation
  std::unique_ptr<std::shared_mutex> m_memory_bitmap_mutex;
  std::unique_ptr<std::atomic<bool>> m_prepearing_for_draw;
};
