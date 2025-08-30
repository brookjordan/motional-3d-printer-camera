#include "led_breath.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

// ===== LED (onboard WS2812) =====
#define RGB_PIN 48
#define NUMPIXELS 1
static Adafruit_NeoPixel pixel(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// Breathing config
const uint16_t PULSE_PERIOD_MS = 5000;
const uint16_t STEPS_HALF = PULSE_PERIOD_MS;
const uint32_t STEP_INTERVAL_US =
    (uint32_t)((PULSE_PERIOD_MS * 1000UL) / (2UL * STEPS_HALF));

static int maxBright = 10; // << hard-coded brightness cap
static uint16_t stepIndex = 0;
static uint32_t lastTickUs = 0;
static int32_t tickRemainderUs = 0;
static uint16_t ditherErrR = 0, ditherErrG = 0, ditherErrB = 0;

// Current base color (mutable now)
static uint8_t BASE_R = 0;
static uint8_t BASE_G = 255;
static uint8_t BASE_B = 180;

namespace LedBreath {

// Current base color (mutable)
static uint8_t BASE_R = 0, BASE_G = 255, BASE_B = 180;

static void pickNewColor() {
  // Pick dominant channel x > 180
  uint8_t x = random(181, 256);          // [181..255]
  uint16_t S = 380 - x;                  // remainder [125..199]
  uint8_t y = (uint8_t)random(0, S + 1); // [0..S]
  uint8_t z = (uint8_t)(S - y);          // completes sum to 380

  // Randomly permute (x,y,z) over (R,G,B)
  uint8_t perm = random(0, 6);
  switch (perm) {
  case 0:
    BASE_R = x;
    BASE_G = y;
    BASE_B = z;
    break;
  case 1:
    BASE_R = x;
    BASE_G = z;
    BASE_B = y;
    break;
  case 2:
    BASE_R = y;
    BASE_G = x;
    BASE_B = z;
    break;
  case 3:
    BASE_R = y;
    BASE_G = z;
    BASE_B = x;
    break;
  case 4:
    BASE_R = z;
    BASE_G = x;
    BASE_B = y;
    break;
  default:
    BASE_R = z;
    BASE_G = y;
    BASE_B = x;
    break;
  }
}

void setup() {
  pixel.begin();
  pixel.clear();
  pixel.show();
  lastTickUs = micros();

  // seed random once
  randomSeed(esp_timer_get_time());
  pickNewColor();
}

void loop() {
  uint32_t nowUs = micros();
  int32_t elapsed = (int32_t)(nowUs - lastTickUs) + tickRemainderUs;
  if (elapsed >= (int32_t)STEP_INTERVAL_US) {
    int32_t stepsToDo = elapsed / (int32_t)STEP_INTERVAL_US;
    int32_t spent = stepsToDo * (int32_t)STEP_INTERVAL_US;
    tickRemainderUs = elapsed - spent;
    lastTickUs = nowUs;

    while (stepsToDo-- > 0) {
      stepIndex++;
      if (stepIndex > 2 * STEPS_HALF) {
        stepIndex = 0;
        pickNewColor(); // new color at black
      }

      uint16_t idx =
          (stepIndex <= STEPS_HALF) ? stepIndex : (2 * STEPS_HALF - stepIndex);

      float alpha = (float)idx / (float)STEPS_HALF; // 0..1..0
      float beta = (float)maxBright / 255.0f;
      float f = powf(alpha, 2.2f) * beta;

      uint32_t r_fp = (uint32_t)(BASE_R * f * 256.0f);
      uint32_t g_fp = (uint32_t)(BASE_G * f * 256.0f);
      uint32_t b_fp = (uint32_t)(BASE_B * f * 256.0f);

      uint32_t sr = r_fp + ditherErrR;
      uint32_t sg = g_fp + ditherErrG;
      uint32_t sb = b_fp + ditherErrB;

      uint8_t r = (uint8_t)(sr >> 8);
      uint8_t g = (uint8_t)(sg >> 8);
      uint8_t b = (uint8_t)(sb >> 8);

      ditherErrR = (uint16_t)(sr & 0xFF);
      ditherErrG = (uint16_t)(sg & 0xFF);
      ditherErrB = (uint16_t)(sb & 0xFF);

      pixel.setPixelColor(0, pixel.Color(r, g, b));
    }
    pixel.show();
  }
}
} // namespace LedBreath