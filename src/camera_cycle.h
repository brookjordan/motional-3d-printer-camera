#pragma once
#include <Arduino.h>

namespace CameraCycle {
  void setup();
  void loop();
}

// Compatibility namespace for web routes
namespace Camera {
  String getCurrentImage();
}

namespace ImageRotator = Camera;