#pragma once
#include <Arduino.h>

namespace CameraCapture {
// Initialize camera and internal state. Returns true on success.
bool setup();

// Periodic task that captures a frame every CAPTURE_PERIOD_MS and
// saves it under FFat `/i/cam_XXXXXX.jpg`, evicting old images as needed.
void tick();

// Optional: force a capture now (returns bytes written or 0 on failure)
size_t captureOnce();
}

