#include "led_breathe.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>
extern "C" {
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

// LED hardware
static Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// Breathing state
static uint32_t breathStartTime = 0;
static bool breathingUp = true;
static uint8_t breathR = 0, breathG = 255, breathB = 180;

static void pickNewColor() {
  breathR = random(100, 255);
  breathG = random(100, 255); 
  breathB = random(100, 255);
}

static void updateLED(float intensity) {
  float brightness = (float)LED_MAX_BRIGHTNESS / 255.0f * intensity;
  uint8_t r = (uint8_t)(breathR * brightness);
  uint8_t g = (uint8_t)(breathG * brightness);
  uint8_t b = (uint8_t)(breathB * brightness);
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

namespace LEDBreathe {

void setup() {
  pixel.begin();
  pixel.clear();
  pixel.show();
  randomSeed(esp_timer_get_time());
  pickNewColor();
  breathStartTime = millis();
}

void loop() {
  uint32_t elapsed = millis() - breathStartTime;
  
  if (breathingUp) {
    // Breathe up over 1.5 seconds
    if (elapsed < 1500) {
      float intensity = (float)elapsed / 1500.0f;
      updateLED(intensity);
    } else {
      updateLED(1.0f);
      breathingUp = false;
      breathStartTime = millis();
    }
  } else {
    // Breathe down over 1.5 seconds
    if (elapsed < 1500) {
      float intensity = 1.0f - ((float)elapsed / 1500.0f);
      updateLED(intensity);
    } else {
      updateLED(0.0f);
      breathingUp = true;
      breathStartTime = millis();
      pickNewColor(); // New color for next cycle
    }
  }
}

void breatheOnce() {
  pickNewColor();
  
  // Breathe up over 1.5 seconds
  for (int i = 0; i <= 150; i++) {
    float intensity = (float)i / 150.0f;
    updateLED(intensity);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  
  // Breathe down over 1.5 seconds
  for (int i = 150; i >= 0; i--) {
    float intensity = (float)i / 150.0f;
    updateLED(intensity);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

} // namespace LEDBreathe