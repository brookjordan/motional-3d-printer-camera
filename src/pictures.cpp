#include "pictures.h"
#include <FFat.h>
#include <vector>
#include <algorithm>
#include <string.h>

namespace ImageRotator {
static const char *fallbackList[] = {
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
static size_t fallbackCount = sizeof(fallbackList) / sizeof(fallbackList[0]);

static std::vector<String> images;
static size_t idx = 0;
static String currentImage = "/i/8e6a765e87869b07a9dcef1385e37368.jpg";
static unsigned long lastUpdate = 0;
static unsigned long interval = 2000; // ms

static bool hasJpegExt(const char *name) {
  const char *p = name; if (!p) return false;
  size_t n = strlen(p); if (n < 5) return false;
  auto ieq = [](char a, char b){ return (a==b) || (a^b)==32; };
  // .jpg
  if (n >= 4 && p[n-4]=='.' && ieq(p[n-3],'j') && ieq(p[n-2],'p') && ieq(p[n-1],'g')) return true;
  // .jpeg
  if (n >= 5 && p[n-5]=='.' && ieq(p[n-4],'j') && ieq(p[n-3],'p') && ieq(p[n-2],'e') && ieq(p[n-1],'g')) return true;
  return false;
}

static void chooseCurrentSafe() {
  if (!images.empty()) currentImage = images[idx % images.size()];
  else currentImage = fallbackList[idx % fallbackCount];
}

void rescan() {
  images.clear();
  File root = FFat.open("/i");
  if (root && root.isDirectory()) {
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
      if (f.isDirectory()) { f.close(); continue; }
      const char *nm = f.name();
      if (nm && hasJpegExt(nm)) {
        images.emplace_back(String(nm));
      }
      f.close();
    }
  }
  // Sort by path/name for stable order
  std::sort(images.begin(), images.end(), [](const String &a, const String &b){ return strcmp(a.c_str(), b.c_str()) < 0; });
  idx = 0; // restart rotation
  chooseCurrentSafe();
}

void setup() {
  lastUpdate = millis();
  rescan();
}

void tick() {
  unsigned long now = millis();
  if (now - lastUpdate >= interval) {
    lastUpdate = now;
    idx++;
    chooseCurrentSafe();
  }
}

String getCurrentImage() { return currentImage; }

void setIntervalMs(unsigned long ms) {
  if (ms < 250) ms = 250; // safety
  interval = ms;
}
} // namespace ImageRotator
