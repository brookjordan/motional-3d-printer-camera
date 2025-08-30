#include "image_rotator.h"

// Example hash list
static const char *imageList[] = {
    "/i/8e6a765e87869b07a9dcef1385e37368.jpg",
    "/i/136e7dc5856bc72011966cf9f0092376.jpg",
    "/i/446b9a73d396f546cba7e166001a44e7.jpg",
    "/i/40377e956f4efcab4e89540f8c385c47.jpg",
    "/i/ade949e442e25206c5e97f1f3a4ebed9.jpg",
    "/i/b09054584c3caf269b2433bfed2a1414.jpg",
    "/i/c7358ed9156c216fdbf65c0815c71f77.jpg",
    "/i/e68ac640c8c06c576e3ecbb10d9afb4e.jpg",
    "/i/fd81cfa469bb650ecdedd67bf5ac517b.jpg",
};
static const size_t imageCount = sizeof(imageList) / sizeof(imageList[0]);

static size_t idx = 0;
static String currentImage = imageList[0];
static unsigned long lastUpdate = 0;
static const unsigned long interval = 2000; // ms

namespace ImageRotator {
void setup() {
  lastUpdate = millis();
  currentImage = imageList[0];
}

void tick() {
  unsigned long now = millis();
  if (now - lastUpdate >= interval) {
    lastUpdate = now;
    idx = (idx + 1) % imageCount;
    currentImage = imageList[idx];
  }
}

String getCurrentImage() { return currentImage; }
} // namespace ImageRotator