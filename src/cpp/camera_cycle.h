#pragma once
#include <Arduino.h>

namespace CameraCycle {
  void setup();
  void loop();
}

// Compatibility namespace for web routes
namespace Camera {
  String getCurrentImage();
  bool getCachedImage(uint8_t **buffer, size_t *size);
}

namespace ImageRotator = Camera;