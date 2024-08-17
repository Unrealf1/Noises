#include "noise_texture.hpp"


NoiseTexture::NoiseTexture(int width, int height)
  : m_draw_bitmap(width, height)
  , m_memory_bitmap(width, height, ALLEGRO_MEMORY_BITMAP)
  , m_memory_bitmap_mutex(std::make_unique<std::shared_mutex>())
  , m_prepearing_for_draw(std::make_unique<std::atomic<bool>>(true)) {
    auto scopeedTargetOverride = scoped_write_to_memory_bitmap();
    al_clear_to_color(al_map_rgb(128, 128, 128));
   }

void NoiseTexture::mark_modified() {
  m_was_modified_during_update = true;
}

TargetBitmapOverride NoiseTexture::scoped_write_to_memory_bitmap() {
  return TargetBitmapOverride(m_memory_bitmap.get_raw());
}

void NoiseTexture::set(int x, int y, ALLEGRO_COLOR color) {
  al_put_pixel(x, y, color);
}

ALLEGRO_COLOR NoiseTexture::get(int x, int y) {
  return al_get_pixel(m_memory_bitmap.get_raw(), x, y);
}

void NoiseTexture::prepare_for_draw(Bitmap& draw_on) {
  m_prepearing_for_draw->store(true);
  m_memory_bitmap_mutex->lock();
  al_unlock_bitmap(m_memory_bitmap.get_raw());
  if (m_was_modified_during_update) {
    auto override = TargetBitmapOverride(draw_on.get_raw());
    al_draw_bitmap(m_memory_bitmap.get_raw(), 0, 0, 0);
    m_was_modified_during_update = false;
  }
  m_locked_memory_bitmap = al_lock_bitmap(m_memory_bitmap.get_raw(), ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
  m_memory_bitmap_mutex->unlock();
  m_prepearing_for_draw->store(false);
  m_prepearing_for_draw->notify_all();
}

int NoiseTexture::width() {
  return m_memory_bitmap.width();
}

int NoiseTexture::height() {
  return m_memory_bitmap.height();
}
