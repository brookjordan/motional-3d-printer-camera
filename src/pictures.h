#pragma once
#include <Arduino.h>

namespace ImageRotator {
void setup();
void tick();
String getCurrentImage();
void setIntervalMs(unsigned long ms);
void rescan();
// Returns the latest camera-captured image path (cam_*.jpg) from /i,
// or empty string if none found.
String getLatestImage();
}
