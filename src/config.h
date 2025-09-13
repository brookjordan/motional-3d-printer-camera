#pragma once
#include "esp_camera.h"

// ===== ACTIVE PROFILE SELECTION =====
// Optional local overrides: create `src/config.local.h` (git-ignored) to define
// credentials or switch profiles without committing secrets.
#if __has_include("config.local.h")
#include "config.local.h"
#endif
// Change this to switch between profiles at compile time (can be overridden in
// config.local.h)
#ifndef ACTIVE_PROFILE
#define ACTIVE_PROFILE HOME_PROFILE
#endif
// Available profiles: HOME_PROFILE, OFFICE_PROFILE

// ===== PROFILE DEFINITIONS =====
struct NetworkConfig {
  const char *wifiSsid;
  const char *wifiPassword;
  const char *mdnsHostname;
  bool useEnterprise;
  const char *eapUsername;
  const char *eapPassword;
  const char *eapOuterIdentity;
};

struct CameraConfig {
  // GPIO pins for ESP32-S3 N16R8 CAM board
  int pinPwdn = -1;
  int pinReset = -1;
  int pinXclk = 15;
  int pinSiod = 4; // I2C SDA
  int pinSioc = 5; // I2C SCL
  int pinY9 = 16;  // D7
  int pinY8 = 17;  // D6
  int pinY7 = 18;  // D5
  int pinY6 = 12;  // D4
  int pinY5 = 10;  // D3
  int pinY4 = 8;   // D2
  int pinY3 = 9;   // D1
  int pinY2 = 11;  // D0
  int pinVsync = 6;
  int pinHref = 7;
  int pinPclk = 13;

  // Camera settings
  uint32_t xclkFreqHz = 6000000; // 6MHz to minimize heat

  // Resolution options:
  // FRAMESIZE_QVGA  - 320x240   (76K pixels)
  // FRAMESIZE_CIF   - 400x296   (118K pixels)
  // FRAMESIZE_VGA   - 640x480   (307K pixels)
  // FRAMESIZE_SVGA  - 800x600   (480K pixels) ← Current (with 8MB PSRAM)
  // FRAMESIZE_VGA   - 640x480   (307K pixels)
  // FRAMESIZE_XGA   - 1024x768  (786K pixels)
  // FRAMESIZE_SXGA  - 1280x1024 (1.3M pixels)
  framesize_t frameSize = FRAMESIZE_SVGA;

  // JPEG Quality: 0-63 (lower = better quality, larger files)
  // 10 = high quality, 20 = good balance, 40 = smaller files
  int jpegQuality = 20;
  int fbCount = 1;
  uint32_t breathCycleDurationMs =
      3000; // 3 seconds total breath cycle (1.5s up + 1.5s down)
};

struct SystemConfig {
  // LED
  int ledPin = 48;
  int ledMaxBrightness = 10; // 0-255

  // Web server
  int webServerPort = 80;
  int pageRefreshSeconds = 5;

  // Image storage
  int maxStoredImages = 5;
  const char *imagePathPrefix = "/i/img_";
  const char *imagePathSuffix = ".jpg";
  const char *latestImagePath = "/i/latest.jpg";

  // System
  uint32_t serialBaudRate = 115200;
  uint32_t cameraTaskStackSize = 16384; // Increased for camera operations
  int cameraTaskPriority = 0;
  int cameraTaskCore = 0;
  uint32_t debugStartupDelayMs = 200;
};

struct Config {
  NetworkConfig network;
  CameraConfig camera;
  SystemConfig system;
};

// ===== DEFAULT CONFIGURATION =====
// All struct members have defaults defined above, so profiles only need to
// override what's different

// ===== PROFILE CONFIGURATIONS =====
// Profiles must be defined in config.local.h
// See config.local.example.h for template

// ===== ACTIVE CONFIGURATION =====
static constexpr Config CONFIG = ACTIVE_PROFILE;

// ===== COMPILE-TIME SAFETY CHECK =====
// Force use of config.local.h for credentials
#if !__has_include("config.local.h")
#error "ERROR: Copy src/config.local.example.h to src/config.local.h and add your real WiFi credentials!"
#endif

// ===== CONVENIENCE MACROS FOR BACKWARD COMPATIBILITY =====
#define WIFI_SSID CONFIG.network.wifiSsid
#define WIFI_PASSWORD CONFIG.network.wifiPassword
#define MDNS_HOSTNAME CONFIG.network.mdnsHostname
#define USE_ENTERPRISE_WIFI CONFIG.network.useEnterprise
#define EAP_USERNAME CONFIG.network.eapUsername
#define EAP_PASSWORD CONFIG.network.eapPassword
#define EAP_OUTER_IDENTITY CONFIG.network.eapOuterIdentity

#define CAMERA_PIN_PWDN CONFIG.camera.pinPwdn
#define CAMERA_PIN_RESET CONFIG.camera.pinReset
#define CAMERA_PIN_XCLK CONFIG.camera.pinXclk
#define CAMERA_PIN_SIOD CONFIG.camera.pinSiod
#define CAMERA_PIN_SIOC CONFIG.camera.pinSioc
#define CAMERA_PIN_Y9 CONFIG.camera.pinY9
#define CAMERA_PIN_Y8 CONFIG.camera.pinY8
#define CAMERA_PIN_Y7 CONFIG.camera.pinY7
#define CAMERA_PIN_Y6 CONFIG.camera.pinY6
#define CAMERA_PIN_Y5 CONFIG.camera.pinY5
#define CAMERA_PIN_Y4 CONFIG.camera.pinY4
#define CAMERA_PIN_Y3 CONFIG.camera.pinY3
#define CAMERA_PIN_Y2 CONFIG.camera.pinY2
#define CAMERA_PIN_VSYNC CONFIG.camera.pinVsync
#define CAMERA_PIN_HREF CONFIG.camera.pinHref
#define CAMERA_PIN_PCLK CONFIG.camera.pinPclk
#define CAMERA_XCLK_FREQ_HZ CONFIG.camera.xclkFreqHz
#define CAMERA_FRAME_SIZE CONFIG.camera.frameSize
#define CAMERA_JPEG_QUALITY CONFIG.camera.jpegQuality
#define CAMERA_FB_COUNT CONFIG.camera.fbCount
#define CAMERA_BREATH_CYCLE_MS CONFIG.camera.breathCycleDurationMs

#define LED_PIN CONFIG.system.ledPin
#define LED_MAX_BRIGHTNESS CONFIG.system.ledMaxBrightness
#define WEB_SERVER_PORT CONFIG.system.webServerPort
#define PAGE_REFRESH_SECONDS CONFIG.system.pageRefreshSeconds
#define MAX_STORED_IMAGES CONFIG.system.maxStoredImages
#define IMAGE_PATH_PREFIX CONFIG.system.imagePathPrefix
#define IMAGE_PATH_SUFFIX CONFIG.system.imagePathSuffix
#define LATEST_IMAGE_PATH CONFIG.system.latestImagePath
#define SERIAL_BAUD_RATE CONFIG.system.serialBaudRate
#define CAMERA_TASK_STACK_SIZE CONFIG.system.cameraTaskStackSize
#define CAMERA_TASK_PRIORITY CONFIG.system.cameraTaskPriority
#define CAMERA_TASK_CORE CONFIG.system.cameraTaskCore
#define DEBUG_STARTUP_DELAY_MS CONFIG.system.debugStartupDelayMs
