#include "camera_cycle.h"
#include "config.h"
#include "esp_camera.h"
#include "led_breathe.h"
#include <Arduino.h>
#include <FFat.h>

// ===== CAMERA SETUP =====
static bool cameraInitialized = false;
static String latestImagePath = LATEST_IMAGE_PATH;
static uint32_t lastCaptureTime = 0;
static String imageHistory[MAX_STORED_IMAGES];
static int imageIndex = 0;

// ===== FILESYSTEM FUNCTIONS =====
static bool initFilesystem() {
  if (!FFat.begin()) {
    Serial.println("FFat mount failed, trying format...");
    if (!FFat.format()) {
      Serial.println("FFat format failed!");
      return false;
    }
    if (!FFat.begin()) {
      Serial.println("FFat mount failed after format!");
      return false;
    }
  }
  Serial.println("FFat mounted successfully");

  // Create image directory if it doesn't exist
  if (!FFat.exists("/i")) {
    FFat.mkdir("/i");
    Serial.println("Created /i directory");
  }

  return true;
}

// ===== CAMERA FUNCTIONS =====
static bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAMERA_PIN_Y2;
  config.pin_d1 = CAMERA_PIN_Y3;
  config.pin_d2 = CAMERA_PIN_Y4;
  config.pin_d3 = CAMERA_PIN_Y5;
  config.pin_d4 = CAMERA_PIN_Y6;
  config.pin_d5 = CAMERA_PIN_Y7;
  config.pin_d6 = CAMERA_PIN_Y8;
  config.pin_d7 = CAMERA_PIN_Y9;
  config.pin_xclk = CAMERA_PIN_XCLK;
  config.pin_pclk = CAMERA_PIN_PCLK;
  config.pin_vsync = CAMERA_PIN_VSYNC;
  config.pin_href = CAMERA_PIN_HREF;
  config.pin_sccb_sda = CAMERA_PIN_SIOD;
  config.pin_sccb_scl = CAMERA_PIN_SIOC;
  config.pin_pwdn = CAMERA_PIN_PWDN;
  config.pin_reset = CAMERA_PIN_RESET;
  config.xclk_freq_hz = CAMERA_XCLK_FREQ_HZ;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = CAMERA_FRAME_SIZE;
  config.jpeg_quality = CAMERA_JPEG_QUALITY;
  config.fb_count = CAMERA_FB_COUNT;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  Serial.println("Camera initialized successfully");
  return true;
}

static void sequentialCaptureAndProcess() {
  if (!cameraInitialized) return;

  // Memory check before capture
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < 50000) {
    Serial.printf("Core 0: Low memory %d bytes, skipping capture\n", freeHeap);
    return;
  }

  // Step 1: Capture image (synchronous)
  Serial.println("Core 0: Capturing image...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Core 0: Camera capture failed");
    // Try to reinitialize camera on failure
    esp_camera_deinit();
    vTaskDelay(pdMS_TO_TICKS(1000));
    cameraInitialized = initCamera();
    return;
  }

  // Step 2: Copy to PSRAM buffer (fast, non-blocking)
  uint8_t *psramBuffer = (uint8_t*)ps_malloc(fb->len);
  if (!psramBuffer) {
    Serial.println("Core 0: PSRAM allocation failed");
    esp_camera_fb_return(fb);
    return;
  }
  
  memcpy(psramBuffer, fb->buf, fb->len);
  size_t imageSize = fb->len;
  esp_camera_fb_return(fb);  // Release camera buffer immediately
  
  // Step 3: Generate filename
  uint32_t timestamp = millis();
  String imagePath = String(IMAGE_PATH_PREFIX) + String(timestamp) + String(IMAGE_PATH_SUFFIX);
  
  // Step 4: Quick atomic write to FFat (minimal blocking)
  Serial.printf("Core 0: Writing %s...\n", imagePath.c_str());
  File file = FFat.open(imagePath, "w");
  bool writeSuccess = false;
  
  if (file) {
    size_t bytesWritten = file.write(psramBuffer, imageSize);
    file.close();
    writeSuccess = (bytesWritten == imageSize);
    
    if (writeSuccess) {
      latestImagePath = imagePath;
      Serial.printf("Core 0: Photo saved %s (%d bytes)\n", imagePath.c_str(), imageSize);
    } else {
      Serial.printf("Core 0: Write failed %d/%d bytes\n", bytesWritten, imageSize);
      FFat.remove(imagePath);
    }
  } else {
    Serial.println("Core 0: Failed to open file - checking filesystem");
    // Try to remount filesystem on failure
    FFat.end();
    vTaskDelay(pdMS_TO_TICKS(100));
    if (!FFat.begin()) {
      Serial.println("Core 0: Filesystem remount failed!");
    }
  }
  
  // Step 5: Free PSRAM buffer
  free(psramBuffer);
  
  if (!writeSuccess) return;
  
  // Step 4: Delete old image if needed (synchronous)
  if (imageHistory[imageIndex].length() > 0) {
    Serial.printf("Core 0: Deleting %s...\n", imageHistory[imageIndex].c_str());
    if (!FFat.remove(imageHistory[imageIndex])) {
      Serial.println("Core 0: Failed to delete old image");
    }
    imageHistory[imageIndex] = ""; // Clear to free memory
  }
  
  // Step 5: Update image history
  imageHistory[imageIndex] = imagePath;
  imageIndex = (imageIndex + 1) % MAX_STORED_IMAGES;
  
  // Memory cleanup
  Serial.printf("Core 0: Free heap: %d bytes\n", ESP.getFreeHeap());
  
  // Step 6: LED breathe once (synchronous)
  Serial.println("Core 0: LED breathe...");
  LEDBreathe::breatheOnce();
  
  Serial.println("Core 0: Cycle complete");
}

namespace CameraCycle {

void setup() {
  // Initialize filesystem with retry
  if (!initFilesystem()) {
    Serial.println("Filesystem initialization failed!");
    return;
  }

  // Clean up all existing images on boot
  File dir = FFat.open("/i");
  if (dir) {
    File file = dir.openNextFile();
    while (file) {
      String fileName = file.name();
      file.close();
      if (fileName.endsWith(".jpg")) {
        String fullPath = "/i/" + fileName;
        FFat.remove(fullPath);
        Serial.printf("Deleted old image: %s\n", fullPath.c_str());
      }
      file = dir.openNextFile();
    }
    dir.close();
  }

  // Initialize camera
  cameraInitialized = initCamera();
  if (!cameraInitialized) {
    Serial.println("Camera initialization failed");
  }

  lastCaptureTime = millis();
}

void loop() {
  uint32_t now = millis();

  // Sequential capture cycle every 3 seconds
  if (now - lastCaptureTime >= 3000) {
    sequentialCaptureAndProcess();
    lastCaptureTime = now;
  }
  
  // Small yield to prevent watchdog
  vTaskDelay(10);
}

} // namespace CameraCycle

// Compatibility functions for existing code
namespace Camera {
String getCurrentImage() { return latestImagePath; }
} // namespace Camera

namespace ImageRotator = Camera;
