#include "camera_capture.h"
#include "pictures.h"
#include <FFat.h>
#include <algorithm>
#include <esp_camera.h>
#include <math.h>
#include <vector>

// ===== OV5640 on ESP32-S3 pin map (8-bit DVP) =====
// Orientation: lens facing you, FPC pins numbered 1→24 from left→right.
// This follows a COMMON 24‑pin OV5640 DVP pinout seen on many modules.
// IMPORTANT: Some boards reorder pins — verify against your module’s datasheet.
//
// Typical 24‑pin mapping (for reference):
//  1: GND        2: AVDD(2.8–3.3V)  3: DOVDD(1.8/2.8/3.3V)  4: DVDD(1.5V/1.2V
//  on-module) 5: SIOC(SCL)  6: SIOD(SDA)       7: VSYNC               8: HREF
//  9: PCLK       10: XCLK           11: D7                 12: D6
// 13: D5        14: D4             15: D3                 16: D2
// 17: D1        18: D0             19: RESETB             20: PWDN
// 21: AF+ (NC if no AF) 22: AF- (NC) 23: GND              24: GND
//
// Connect ESP32‑S3 GPIOs below to the corresponding OV5640 FPC pin numbers.
// If your module differs, adjust the FPC pin numbers here and rewire.
// Control/I2C/Clock
#define CAM_PWDN -1  // OV5640 pin 20 (PWDN). Tie to GND if left at -1
#define CAM_RESET -1 // OV5640 pin 19 (RESETB). Pull-up to 3V3 if left at -1
#define CAM_XCLK 15  // OV5640 pin 10 (XCLK/MCLK)
#define CAM_SIOD 41  // OV5640 pin 6  (SIOD/SDA)
#define CAM_SIOC 40  // OV5640 pin 5  (SIOC/SCL)

// Parallel data bus (8‑bit)
#define CAM_D7 16 // OV5640 pin 11 (D7)
#define CAM_D6 17 // OV5640 pin 12 (D6)
#define CAM_D5 18 // OV5640 pin 13 (D5)
#define CAM_D4 12 // OV5640 pin 14 (D4)
#define CAM_D3 10 // OV5640 pin 15 (D3)
#define CAM_D2 8  // OV5640 pin 16 (D2)
#define CAM_D1 9  // OV5640 pin 17 (D1)
#define CAM_D0 11 // OV5640 pin 18 (D0)

#define CAM_VSYNC 46 // OV5640 pin 7  (VSYNC)
#define CAM_HREF 45  // OV5640 pin 8  (HREF)
#define CAM_PCLK 21  // OV5640 pin 9  (PCLK)
// ===================================================

namespace {
static const unsigned long CAPTURE_PERIOD_MS = 60UL * 1000UL; // 1 minute
static const char *IMG_DIR = "/i"; // where UI expects images
static const char *META_PATH = "/i/cam_meta.txt";
static const char *PREFIX = "cam_"; // our files: cam_XXXXXX.jpg

// Keep 10% or at least 128KB free to avoid fragmentation issues
static const float SAFETY_FRACTION = 0.10f;
static const size_t SAFETY_MIN_BYTES = 128 * 1024UL;

static unsigned long lastCapture = 0;
static uint32_t nextIndex = 0;

struct ImgInfo {
  String path;
  uint32_t idx;
  size_t size;
};

bool hasJpegExt(const char *name) {
  if (!name)
    return false;
  size_t n = strlen(name);
  if (n < 4)
    return false;
  auto ieq = [](char a, char b) { return (a == b) || ((a ^ b) == 32); };
  if (n >= 4 && name[n - 4] == '.' && ieq(name[n - 3], 'j') &&
      ieq(name[n - 2], 'p') && ieq(name[n - 1], 'g'))
    return true;
  if (n >= 5 && name[n - 5] == '.' && ieq(name[n - 4], 'j') &&
      ieq(name[n - 3], 'p') && ieq(name[n - 2], 'e') && ieq(name[n - 1], 'g'))
    return true;
  return false;
}

uint64_t fsTotal() { return FFat.totalBytes(); }
uint64_t fsUsed() { return FFat.usedBytes(); }
uint64_t fsFree() { return fsTotal() - fsUsed(); }

uint64_t safetyHeadroom() {
  uint64_t byFrac = (uint64_t)(fsTotal() * SAFETY_FRACTION);
  return (byFrac > SAFETY_MIN_BYTES) ? byFrac : SAFETY_MIN_BYTES;
}

String indexToName(uint32_t idx) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s/%s%06lu.jpg", IMG_DIR, PREFIX,
           (unsigned long)idx);
  return String(buf);
}

int32_t nameToIndex(const String &path) {
  // expects /i/cam_XXXXXX.jpg
  int s = path.lastIndexOf('/');
  if (s < 0)
    return -1;
  String base = path.substring(s + 1); // cam_XXXXXX.jpg
  if (!base.startsWith(PREFIX) || !base.endsWith(".jpg"))
    return -1;
  String num = base.substring(strlen(PREFIX), base.length() - 4);
  for (size_t i = 0; i < num.length(); ++i)
    if (!isDigit(num[i]))
      return -1;
  return num.toInt();
}

void listOurImages(std::vector<ImgInfo> &out) {
  out.clear();
  File dir = FFat.open(IMG_DIR);
  if (!dir || !dir.isDirectory())
    return;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    if (!f.isDirectory()) {
      String p = String(f.name());
      if (hasJpegExt(p.c_str())) {
        int32_t idx = nameToIndex(p);
        if (idx >= 0)
          out.push_back({p, (uint32_t)idx, (size_t)f.size()});
      }
    }
    f.close();
  }
  std::sort(out.begin(), out.end(),
            [](const ImgInfo &a, const ImgInfo &b) { return a.idx < b.idx; });
}

size_t p80Size(const std::vector<ImgInfo> &imgs,
               size_t defaultGuess = 60 * 1024) {
  if (imgs.empty())
    return defaultGuess;
  std::vector<size_t> sizes;
  sizes.reserve(imgs.size());
  for (auto &ii : imgs)
    sizes.push_back(ii.size);
  std::sort(sizes.begin(), sizes.end());
  size_t n = sizes.size();
  size_t pos = (size_t)floor(0.8 * n);
  if (pos >= n)
    pos = n - 1;
  return sizes[pos];
}

void loadNextIndexFromMetaOrScan() {
  if (FFat.exists(META_PATH)) {
    File m = FFat.open(META_PATH, FILE_READ);
    if (m) {
      String s = m.readString();
      m.close();
      uint32_t v = s.toInt();
      if (v > 0 && v < 10000000UL) {
        nextIndex = v;
        return;
      }
    }
  }
  std::vector<ImgInfo> imgs;
  listOurImages(imgs);
  nextIndex = imgs.empty() ? 0 : (imgs.back().idx + 1);
}

void persistNextIndex() {
  File m = FFat.open(META_PATH, FILE_WRITE);
  if (m) {
    m.printf("%lu\n", (unsigned long)nextIndex);
    m.close();
  }
}

void ensureCapacityFor(size_t k, size_t estSize) {
  std::vector<ImgInfo> imgs;
  listOurImages(imgs);
  uint64_t need = (uint64_t)k * (uint64_t)estSize + safetyHeadroom();
  while (fsFree() < need && !imgs.empty()) {
    ImgInfo oldest = imgs.front();
    FFat.remove(oldest.path);
    imgs.erase(imgs.begin());
  }
}

size_t captureAndSave(uint32_t idx) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return 0;
  }
  String path = indexToName(idx);
  File file = FFat.open(path, FILE_WRITE);
  size_t written = 0;
  if (!file) {
    Serial.println("File open failed (pre-space?)");
  } else {
    written = file.write(fb->buf, fb->len);
    file.close();
    if (written != fb->len) {
      Serial.printf("Write incomplete (%u/%u)\n", (unsigned)written,
                    (unsigned)fb->len);
      FFat.remove(path);
      written = 0;
    } else {
      Serial.printf("Saved %s (%u bytes)\n", path.c_str(), (unsigned)written);
    }
  }
  esp_camera_fb_return(fb);
  if (written > 0) {
    // Make the UI see new images without reboot
    ImageRotator::rescan();
  }
  return written;
}

bool initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_D0;
  config.pin_d1 = CAM_D1;
  config.pin_d2 = CAM_D2;
  config.pin_d3 = CAM_D3;
  config.pin_d4 = CAM_D4;
  config.pin_d5 = CAM_D5;
  config.pin_d6 = CAM_D6;
  config.pin_d7 = CAM_D7;
  config.pin_xclk = CAM_XCLK;
  config.pin_pclk = CAM_PCLK;
  config.pin_vsync = CAM_VSYNC;
  config.pin_href = CAM_HREF;
  config.pin_sscb_sda = CAM_SIOD;
  config.pin_sscb_scl = CAM_SIOC;
  config.pin_pwdn = CAM_PWDN;
  config.pin_reset = CAM_RESET;

  config.xclk_freq_hz = 20000000;       // 20 MHz
  config.pixel_format = PIXFORMAT_JPEG; // JPEG output
  config.frame_size = FRAMESIZE_QQVGA;  // 160x120: smallest for smoke test
  config.jpeg_quality = 35;             // higher is lower quality/smaller
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;
#ifdef CAMERA_FB_IN_PSRAM
  config.fb_location = CAMERA_FB_IN_PSRAM;
#endif

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    // A couple of conservative tweaks for stability
    s->set_brightness(s, 0);
    s->set_saturation(s, 0);
    s->set_contrast(s, 0);
  }
  return true;
}
} // namespace

namespace CameraCapture {
bool setup() {
  // Ensure image directory exists
  if (!FFat.exists(IMG_DIR))
    FFat.mkdir(IMG_DIR);
  loadNextIndexFromMetaOrScan();
  if (!initCamera())
    return false;
  lastCapture = millis() - CAPTURE_PERIOD_MS; // shoot one immediately
  Serial.printf("FFat total=%llu used=%llu free=%llu (safety=%llu)\n",
                fsTotal(), fsUsed(), fsFree(), safetyHeadroom());
  return true;
}

void tick() {
  unsigned long now = millis();
  if (now - lastCapture < CAPTURE_PERIOD_MS)
    return;
  lastCapture = now;

  // Estimate size from existing captures
  std::vector<ImgInfo> imgs;
  listOurImages(imgs);
  size_t estP80 = p80Size(imgs, 60 * 1024);

  // Ensure space for 3 future images + safety
  ensureCapacityFor(3, estP80);

  size_t written = captureAndSave(nextIndex);
  if (written == 0) {
    // Free a bit more and retry once
    ensureCapacityFor(2, estP80);
    written = captureAndSave(nextIndex);
  }
  if (written > 0) {
    nextIndex++;
    persistNextIndex();
  }
  Serial.printf("FFat used=%llu free=%llu\n", fsUsed(), fsFree());
}

size_t captureOnce() {
  size_t written = captureAndSave(nextIndex);
  if (written > 0) {
    nextIndex++;
    persistNextIndex();
  }
  return written;
}
} // namespace CameraCapture
