#include "settings.h"
#include <FFat.h>
#include <ctype.h>

static inline String trimStr(const String &s) {
  int a = 0, b = s.length();
  while (a < b && isspace((unsigned char)s[a])) {
    a++;
  }
  while (b > a && isspace((unsigned char)s[b - 1])) {
    b--;
  }
  return s.substring(a, b);
}

void loadSettingsFromFFat(Settings &out) {
  File f = FFat.open("/you-can-change-this/settings.txt", "r");
  if (!f) {
    return;
  }

  struct KeyRule {
    const char *pattern;
    void (*apply)(Settings &, const String &);
  };

  auto apply_wifi_name = [](Settings &s, const String &v) { s.wifiName = v; };
  auto apply_wifi_password = [](Settings &s, const String &v) {
    s.wifiPassword = v;
  };
  auto apply_mdns_name = [](Settings &s, const String &v) { s.mdnsName = v; };
  auto apply_led_brightness = [](Settings &s, const String &v) {
    int n = v.toInt();
    if (n < 0)
      n = 0;
    if (n > 255)
      n = 255;
    s.ledMaxBrightness = n;
  };
  auto apply_picture_speed = [](Settings &s, const String &v) {
    int n = v.toInt();
    if (n <= 0)
      n = 2;
    s.pictureSpeedSeconds = n;
  };
  auto apply_page_refresh = [](Settings &s, const String &v) {
    int n = v.toInt();
    if (n < 1)
      n = 5;
    s.pageRefreshSeconds = n;
  };

  const KeyRule rules[] = {
      {"wi-fi name", apply_wifi_name},
      {"wifi name", apply_wifi_name},
      {"network name", apply_wifi_name},

      {"wi-fi password", apply_wifi_password},
      {"wifi password", apply_wifi_password},
      {"network password", apply_wifi_password},

      {"web address name", apply_mdns_name},
      {"local name", apply_mdns_name},
      {"mdns", apply_mdns_name},

      {"led brightness", apply_led_brightness},
      {"brightness", apply_led_brightness},

      {"picture speed", apply_picture_speed},
      {"photo speed", apply_picture_speed},

      {"page refresh", apply_page_refresh},
      {"reload every", apply_page_refresh},
  };

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) {
      continue;
    }
    if (line[0] == '#') {
      continue;
    }
    int colon = line.indexOf(':');
    if (colon <= 0) {
      continue;
    }
    String key = trimStr(line.substring(0, colon));
    String val = trimStr(line.substring(colon + 1));
    String keyL = key;
    keyL.toLowerCase();

    for (const auto &r : rules) {
      if (keyL.indexOf(r.pattern) >= 0) {
        r.apply(out, val);
        break;
      }
    }
  }
  f.close();
}
