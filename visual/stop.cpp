#include "stop.hpp"


static bool s_should_stop = false;

void app::stop() {
  s_should_stop = true;
}

bool app::should_stop() {
  return s_should_stop;
}

