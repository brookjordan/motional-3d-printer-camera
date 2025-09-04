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

static void applyCommon(Settings &s, const String &keyL, const String &val) {
  auto parse_bool = [](const String &v) {
    String t = v;
    t.trim();
    t.toLowerCase();
    return (t == "1" || t == "true" || t == "yes" || t == "on");
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
    if (n < 0)
      n = 0;
    if (n > 86400)
      n = 86400;
    s.pictureSpeedSeconds = n;
  };
  auto apply_page_refresh = [](Settings &s, const String &v) {
    int n = v.toInt();
    if (n < 1)
      n = 1;
    if (n > 86400)
      n = 86400;
    s.pageRefreshSeconds = n;
  };
  auto apply_use_enterprise = [](Settings &s, const String &v) {
    String t = v;
    t.trim();
    t.toLowerCase();
    bool on = (t == "1" || t == "true" || t == "yes" || t == "on");
    s.useEnterprise = on;
    Serial.printf("Parsed Enterprise toggle -> %s (val='%s')\n",
                  on ? "yes" : "no", v.c_str());
  };
  auto apply_eap_user = [](Settings &s, const String &v) { s.eapUsername = v; };
  auto apply_eap_pass = [](Settings &s, const String &v) { s.eapPassword = v; };
  auto apply_outer_id = [](Settings &s, const String &v) {
    s.eapOuterIdentity = v;
  };
  auto apply_ca_path = [](Settings &s, const String &v) {
    s.eapCaCertPath = v;
  };

  struct KeyRule {
    const char *key;
    void (*apply)(Settings &, const String &);
  };
  // All keys are lowercase; match by exact equality only (no substrings)
  const KeyRule rules[] = {
      // SSID
      {"wi-fi name", apply_wifi_name},
      {"wi‑fi name", apply_wifi_name},
      {"wifi name", apply_wifi_name},
      {"network name", apply_wifi_name},
      {"ssid", apply_wifi_name},
      // PSK
      {"wi-fi password", apply_wifi_password},
      {"wi‑fi password", apply_wifi_password},
      {"wifi password", apply_wifi_password},
      {"network password", apply_wifi_password},
      // mDNS hostname
      {"web address name", apply_mdns_name},
      {"local name", apply_mdns_name},
      {"mdns", apply_mdns_name},
      {"mdns hostname", apply_mdns_name},
      // UI
      {"led brightness", apply_led_brightness},
      {"led brightness (0-255)", apply_led_brightness},
      {"brightness", apply_led_brightness},
      {"picture speed", apply_picture_speed},
      {"picture speed (seconds)", apply_picture_speed},
      {"photo speed", apply_picture_speed},
      {"photo speed (seconds)", apply_picture_speed},
      {"page refresh", apply_page_refresh},
      {"page refresh (seconds)", apply_page_refresh},
      {"reload every", apply_page_refresh},
      // Enterprise
      {"use wpa2 enterprise", apply_use_enterprise},
      {"enable wpa2 enterprise", apply_use_enterprise},
      {"use enterprise", apply_use_enterprise},
      {"enable enterprise", apply_use_enterprise},
      {"enterprise username", apply_eap_user},
      {"eap username", apply_eap_user},
      {"username", apply_eap_user},
      {"enterprise password", apply_eap_pass},
      {"eap password", apply_eap_pass},
      {"password", apply_eap_pass},
      {"outer identity", apply_outer_id},
      {"outer identity (optional)", apply_outer_id},
      {"anonymous identity", apply_outer_id},
      {"ca cert path", apply_ca_path},
      {"ca cert path (optional)", apply_ca_path},
      {"ca certificate", apply_ca_path},
  };
  for (const auto &r : rules) {
    if (keyL == r.key) {
      r.apply(s, val);
      return;
    }
  }
}

static void loadKvFile(const char *path, Settings &out) {
  File f = FFat.open(path, "r");
  if (!f)
    return;
  Serial.printf("Reading settings from %s\n", path);
  String activeProfile;
  bool hasActive = false;
  String curProfile;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length() || line[0] == '#')
      continue;
    // Profile header: [profile: name]
    if (line.length() > 2 && line[0] == '[' && line[line.length() - 1] == ']') {
      String inner = trimStr(line.substring(1, line.length() - 1));
      String innerL = inner;
      innerL.toLowerCase();
      if (innerL.indexOf("profile") == 0) {
        int c = inner.indexOf(':');
        String name = (c > 0) ? trimStr(inner.substring(c + 1)) : String();
        curProfile = name;
        curProfile.toLowerCase();
        Serial.printf("  Entering profile [%s] in %s\n", name.c_str(), path);
        continue;
      }
    }
    int colon = line.indexOf(':');
    if (colon <= 0)
      continue;
    String key = trimStr(line.substring(0, colon));
    String val = trimStr(line.substring(colon + 1));
    String keyL = key;
    keyL.toLowerCase();

    // Recognize Active Profile regardless of section (exact-key matching)
    if (keyL == "active profile") {
      activeProfile = val;
      activeProfile.toLowerCase();
      hasActive = activeProfile.length();
      Serial.printf("  %s: Active Profile -> '%s'\n", path,
                    activeProfile.c_str());
      continue;
    }

    bool shouldApply = true;
    if (curProfile.length()) {
      shouldApply = hasActive && (curProfile == activeProfile);
    }
    if (!shouldApply)
      continue;

    // Targeted logging for Enterprise toggle (exact-key matching)
    bool isEntToggle =
        (keyL == "use wpa2 enterprise" || keyL == "enable wpa2 enterprise" ||
         keyL == "use enterprise" || keyL == "enable enterprise");
    if (isEntToggle) {
      String v = val;
      v.toLowerCase();
      bool on = (v == "1" || v == "true" || v == "yes");
      Serial.printf("  %s: Enterprise toggle -> %s (line: '%s: %s')\n", path,
                    on ? "yes" : "no", key.c_str(), val.c_str());
    }
    applyCommon(out, keyL, val);
  }
  f.close();
}

void loadSettingsFromFFat(Settings &out) {
  loadKvFile("/you-can-change-this/settings.txt", out);
  Serial.printf("After settings.txt: useEnterprise=%s, user='%s'\n",
                out.useEnterprise ? "yes" : "no", out.eapUsername.c_str());
}

// Deprecated: secrets.txt is no longer used. All settings live in settings.txt.
