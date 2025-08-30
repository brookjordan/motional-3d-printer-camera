#pragma once
#include <Arduino.h>

namespace ImageRotator {
void setup();
void tick();              // call regularly in loop()
String getCurrentImage(); // ask for current file name
} // namespace ImageRotator