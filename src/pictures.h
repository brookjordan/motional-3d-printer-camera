#pragma once
#include <Arduino.h>

namespace ImageRotator {
void setup();
void tick();
String getCurrentImage();
void setIntervalMs(unsigned long ms);
void rescan();
}
