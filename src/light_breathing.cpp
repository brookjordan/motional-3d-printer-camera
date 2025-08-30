#include "light_breathing.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>
extern "C" {
#include "esp_timer.h"
}

// ===== LED (onboard WS2812) =====
#define RGB_PIN 48
#define NUMPIXELS 1
static Adafruit_NeoPixel pixel(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// Breathing config
const uint16_t PULSE_PERIOD_MS = 5000;
const uint16_t STEPS_HALF = PULSE_PERIOD_MS;
const uint32_t STEP_INTERVAL_US = (uint32_t)((PULSE_PERIOD_MS * 1000UL) / (2UL * STEPS_HALF));

static int maxBright = 10; // brightness cap 0..255
static uint16_t stepIndex = 0;
static uint32_t lastTickUs = 0;
static int32_t tickRemainderUs = 0;
static uint16_t ditherErrR = 0, ditherErrG = 0, ditherErrB = 0;

// Current base color (mutable)
static uint8_t BASE_R = 0, BASE_G = 255, BASE_B = 180;

// Convert LCH(ab) -> sRGB (8-bit). L in [0..100], C arbitrary, H in degrees [0..360)
static void lch_to_srgb(float L, float C, float Hdeg, uint8_t &R, uint8_t &G, uint8_t &B) {
  // LCH(ab) -> Lab
  float Hr = Hdeg * (float)M_PI / 180.0f;
  float a = C * cosf(Hr);
  float b = C * sinf(Hr);

  // Lab -> XYZ (D65)
  float fy = (L + 16.0f) / 116.0f;
  float fx = a / 500.0f + fy;
  float fz = fy - b / 200.0f;

  auto finv = [](float t) -> float {
    const float eps = 0.008856f;       // (6/29)^3
    const float kappa = 7.787f;        // 29^3/(3*29^2)
    const float offset = 16.0f / 116.0f;
    float t3 = t * t * t;
    return (t3 > eps) ? t3 : (t - offset) / kappa;
  };

  // Reference white D65
  const float Xr = 95.047f;
  const float Yr = 100.000f;
  const float Zr = 108.883f;

  float X = Xr * finv(fx);
  float Y = Yr * finv(fy);
  float Z = Zr * finv(fz);

  // XYZ (0..100) -> linear sRGB (0..1)
  float x = X / 100.0f, y = Y / 100.0f, z = Z / 100.0f;
  float rl =  3.2406f * x + (-1.5372f) * y + (-0.4986f) * z;
  float gl = -0.9689f * x +  1.8758f  * y +  0.0415f  * z;
  float bl =  0.0557f * x + (-0.2040f) * y +  1.0570f  * z;

  auto compand = [](float c) -> float {
    if (c <= 0.0031308f) return 12.92f * c;
    return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
  };

  rl = compand(fmaxf(0.0f, fminf(1.0f, rl)));
  gl = compand(fmaxf(0.0f, fminf(1.0f, gl)));
  bl = compand(fmaxf(0.0f, fminf(1.0f, bl)));

  R = (uint8_t)lroundf(fmaxf(0.0f, fminf(1.0f, rl)) * 255.0f);
  G = (uint8_t)lroundf(fmaxf(0.0f, fminf(1.0f, gl)) * 255.0f);
  B = (uint8_t)lroundf(fmaxf(0.0f, fminf(1.0f, bl)) * 255.0f);
}

static void pickNewColor() {
  // Fixed L and C; random H in [0, 359]
  const float L = 85.0f;
  const float C = 60.0f;
  float H = (float)random(0, 360);
  uint8_t r, g, b;
  lch_to_srgb(L, C, H, r, g, b);
  BASE_R = r; BASE_G = g; BASE_B = b;
}

namespace LedBreath {

void setup() {
  pixel.begin();
  pixel.clear();
  pixel.show();
  lastTickUs = micros();
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
        pickNewColor();
      }

      uint16_t idx = (stepIndex <= STEPS_HALF) ? stepIndex : (2 * STEPS_HALF - stepIndex);
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

void setMaxBrightness(int v) {
  if (v < 0) v = 0; if (v > 255) v = 255;
  maxBright = v;
}

} // namespace LedBreath
