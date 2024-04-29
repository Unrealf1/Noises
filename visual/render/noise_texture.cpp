#include "noise_texture.hpp"

#include <spdlog/spdlog.h>

NoiseTexture::NoiseTexture(int width, int height)
  : m_draw_bitmap(width, height)
  , m_memory_bitmap(width, height, ALLEGRO_MEMORY_BITMAP) { 
}

struct TargetBitmapOverride{
  TargetBitmapOverride(ALLEGRO_BITMAP* new_target) {
    m_old_target = al_get_target_bitmap();
    if (m_old_target != new_target) {
      al_set_target_bitmap(new_target);
    }
  }

  ~TargetBitmapOverride() {
    al_set_target_bitmap(m_old_target);
  }
  ALLEGRO_BITMAP* m_old_target;
};

void NoiseTexture::set(int x, int y, ALLEGRO_COLOR color) {
  m_was_modified_during_update = true;

  auto override = TargetBitmapOverride(m_memory_bitmap.get_raw());
  al_put_pixel(x, y, color);
}

ALLEGRO_COLOR NoiseTexture::get(int x, int y) {
  return al_get_pixel(m_memory_bitmap.get_raw(), x, y);
}

void NoiseTexture::prepare_for_update() {
  m_locked_memory_bitmap = al_lock_bitmap(m_memory_bitmap.get_raw(), ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
  m_was_modified_during_update = false;
}

void NoiseTexture::prepare_for_draw() {
  al_unlock_bitmap(m_memory_bitmap.get_raw());
  if (m_was_modified_during_update) {
    auto override = TargetBitmapOverride(m_draw_bitmap.get_raw());
    al_draw_bitmap(m_memory_bitmap.get_raw(), 0, 0, 0);
    spdlog::info("updating draw bitmap");
  }
}

int NoiseTexture::width() {
  return m_memory_bitmap.width();
}

int NoiseTexture::height() {
  return m_memory_bitmap.height();
}

void NoiseTexture::draw(ALLEGRO_DISPLAY* display, const InspectionState& inspectionState) {
  auto displayBitmap = al_get_backbuffer(display);
  auto override = TargetBitmapOverride(displayBitmap);
  //al_draw_bitmap(m_draw_bitmap.get_raw(), 0, 0, 0); // TODO: draw scaled
  auto texWidth = al_get_bitmap_width(m_draw_bitmap.get_raw());
  auto texHeight = al_get_bitmap_height(m_draw_bitmap.get_raw());
  al_draw_scaled_bitmap(m_draw_bitmap.get_raw(),
    inspectionState.x_offset, inspectionState.y_offset, texWidth * inspectionState.zoom, texHeight * inspectionState.zoom,
    0.0f, 0.0f, al_get_bitmap_width(displayBitmap), al_get_bitmap_height(displayBitmap), 0);
}
