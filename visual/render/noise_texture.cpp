#include "noise_texture.hpp"


NoiseTexture::NoiseTexture(int width, int height)
  : m_draw_bitmap(width, height)
  , m_memory_bitmap(width, height, ALLEGRO_MEMORY_BITMAP)
  , m_memory_bitmap_mutex(std::make_unique<std::shared_mutex>())
  , m_prepearing_for_draw(std::make_unique<std::atomic<bool>>(true)) { }

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

void NoiseTexture::prepare_for_update() {
}

void NoiseTexture::prepare_for_draw() {
  m_prepearing_for_draw->store(true);
  m_memory_bitmap_mutex->lock();
  al_unlock_bitmap(m_memory_bitmap.get_raw());
  if (m_was_modified_during_update) {
    auto override = TargetBitmapOverride(m_draw_bitmap.get_raw());
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

void NoiseTexture::draw(ALLEGRO_DISPLAY* display, const InspectionState& inspectionState) {
  auto displayBitmap = al_get_backbuffer(display);
  auto override = TargetBitmapOverride(displayBitmap);

  auto texWidth = float(al_get_bitmap_width(m_draw_bitmap.get_raw()));
  auto texHeight = float(al_get_bitmap_height(m_draw_bitmap.get_raw()));

  auto zoomTexWidth = texWidth * inspectionState.zoom;
  auto zoomTexHeight = texHeight * inspectionState.zoom;

  // no zoomed center - zoomed center
  auto zoomToCenterCompensationX = (texWidth - zoomTexWidth) / 2.0f;
  auto zoomToCenterCompensationY = (texHeight - zoomTexHeight) / 2.0f;

  auto zoomOffsetX = inspectionState.x_offset + zoomToCenterCompensationX;
  auto zoomOffsetY = inspectionState.y_offset + zoomToCenterCompensationY;

  al_draw_scaled_bitmap(m_draw_bitmap.get_raw(),
    0.0f, 0.0f,
    texWidth, texHeight,
    zoomOffsetX, zoomOffsetY,
    zoomTexWidth, zoomTexHeight, 0);
}

