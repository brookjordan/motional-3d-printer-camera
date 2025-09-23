#include "wifi_and_name.h"
#include "config.h"
#include <FFat.h>
#include <WiFi.h>
#ifdef ESP32
#include <ESPmDNS.h>
#include <esp_eap_client.h>
#include <esp_log.h>
#include <esp_wifi.h>
#endif

static String readFileToString(const char *path) {
  File f = FFat.open(path, "r");
  if (!f)
    return String();
  String s;
  s.reserve((size_t)f.size() + 1);
  while (f.available())
    s += (char)f.read();
  f.close();
  return s;
}

#ifdef ESP32
static bool connectWPA2Enterprise() {
  // Prepare STA cleanly; avoid noisy log if STA hasn't begun yet.
  WiFi.mode(WIFI_STA);
  wl_status_t pre = WiFi.status();
  if (pre != WL_IDLE_STATUS) {
    WiFi.disconnect(true, true);
    delay(200);
  }
  // Verbose logs for WPA/EAP negotiation to aid troubleshooting
  // esp_log_level_set("wifi", ESP_LOG_INFO);
  // esp_log_level_set("eap", ESP_LOG_DEBUG);

  const char *ssid = WIFI_SSID;
  const char *user = EAP_USERNAME;
  const char *pass = EAP_PASSWORD;
  const char *identCandidate = EAP_OUTER_IDENTITY;
  const char *ident = (identCandidate && *identCandidate) ? identCandidate : user;

  // Configure EAP identity/credentials (IDF 5 / Arduino-ESP32 3.x)
  esp_eap_client_set_identity((const uint8_t *)ident, strlen(ident));
  esp_eap_client_set_username((const uint8_t *)user, strlen(user));
  esp_eap_client_set_password((const uint8_t *)pass, strlen(pass));

  // CA certificate support removed for simplicity

  if (esp_wifi_sta_enterprise_enable() != ESP_OK) {
    Serial.println("Enterprise enable failed");
    return false;
  }

  WiFi.begin(ssid);
  Serial.print("Connecting (WPA2-Enterprise)");

  auto wlStatusName = [](wl_status_t st) -> const char * {
    switch (st) {
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID";
    case WL_SCAN_COMPLETED:
      return "SCAN_DONE";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
    }
  };

  uint32_t t0 = millis();
  wl_status_t last = WL_IDLE_STATUS;
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 30000) {
    wl_status_t cur = WiFi.status();
    if (cur != last) {
      Serial.printf(" [%s]", wlStatusName(cur));
      last = cur;
    }
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  bool ok = WiFi.status() == WL_CONNECTED;
  if (!ok) {
    Serial.printf(
        "WPA2-Enterprise connect failed. SSID='%s' user='%s' outer='%s'\n",
        ssid, user, ident ? ident : "");
    Serial.println("Tips: verify username/password, try setting an Outer "
                   "Identity (e.g. 'anonymous'). "
                   "WPA/EAP debug logs above can help.");
  }
  return ok;
}
#endif

void connectWiFi() {
  bool ok = false;
  const int kMaxAttempts = 3;
  for (int attempt = 1; attempt <= kMaxAttempts && !ok; ++attempt) {
    const char *ssid = WIFI_SSID ? WIFI_SSID : "";
    const char *user = EAP_USERNAME ? EAP_USERNAME : "";
    Serial.printf(
        "Wi-Fi attempt %d/%d: SSID='%s', useEnterprise=%s, user='%s'\n",
        attempt, kMaxAttempts, ssid, USE_ENTERPRISE_WIFI ? "yes" : "no",
        user);
#ifdef ESP32
    bool enterpriseEligible = USE_ENTERPRISE_WIFI && strlen(EAP_USERNAME) > 0 &&
                              strlen(WIFI_SSID) > 0;
    if (!enterpriseEligible && USE_ENTERPRISE_WIFI) {
      Serial.print("Enterprise requested but not eligible: missing ");
      bool first = true;
      if (strlen(EAP_USERNAME) == 0) {
        Serial.print("username");
        first = false;
      }
      if (strlen(WIFI_SSID) == 0) {
        Serial.print(first ? "ssid" : "+ ssid");
        first = false;
      }
      Serial.println();
    }
    if (enterpriseEligible) {
      Serial.println("Trying WPA2-Enterprise...");
      ok = connectWPA2Enterprise();
    }
#endif
    if (!ok) {
      WiFi.mode(WIFI_STA);
      // esp_log_level_set("wpa", ESP_LOG_DEBUG);
      // esp_log_level_set("eap", ESP_LOG_DEBUG);

      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.println("Falling back to WPA2-PSK/open");
      Serial.print("Connecting");
      uint32_t t0 = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
        delay(300);
        Serial.print(".");
      }
      Serial.println();
      ok = WiFi.status() == WL_CONNECTED;
      if (!ok) {
        wl_status_t st = WiFi.status();
        Serial.printf("WPA2-PSK/open connect failed (status=%d). ", (int)st);
        if (st == WL_NO_SSID_AVAIL) {
          Serial.println("SSID not visible.");
        } else if (st == WL_CONNECT_FAILED) {
          Serial.println("Authentication failed (password or AP reject).");
        } else {
          Serial.println("See logs above for details.");
        }
        if (st == WL_NO_SSID_AVAIL) {
          Serial.println("Scan (top 10 SSIDs):");
          int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
          for (int i = 0; i < n && i < 10; ++i) {
            Serial.printf("  %2d) %s (RSSI %d, ch %d)\n", i + 1,
                          WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
          }
          if (n <= 0)
            Serial.println("  <no networks found>");
        }
      }
    }

    if (!ok && attempt < kMaxAttempts) {
      Serial.printf("Retrying Wi-Fi in 2s (%d/%d)\n", attempt, kMaxAttempts);
      WiFi.disconnect(true, true);
      delay(2000);
    }
  }

  if (ok) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    // Reduce verbose WPA/EAP logs after a successful join
    esp_log_level_set("wpa", ESP_LOG_WARN);
    esp_log_level_set("eap", ESP_LOG_WARN);
#ifdef ESP32
    if (MDNS.begin(MDNS_HOSTNAME)) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("mDNS: http://%s.local\n", MDNS_HOSTNAME);
    }
#endif
  } else {
    Serial.println("Wi-Fi not connected. Starting SoftAP for local access...");
    // Create a small unique SSID suffix from the chip MAC
    uint64_t mac = ESP.getEfuseMac();
    char suf[5];
    snprintf(suf, sizeof(suf), "%04X", (unsigned int)(mac & 0xFFFF));
    String apSsid = String("PrinterCam-") + suf; // e.g., PrinterCam-1A2B
    const char *apPass = "printercam";           // >=8 chars; change if desired

    WiFi.mode(WIFI_AP_STA);
    // Channel 1, visible SSID, up to 8 clients
    bool apok = WiFi.softAP(apSsid.c_str(), apPass, 1, 0, 8);
    if (apok) {
      IPAddress apip = WiFi.softAPIP();
      Serial.printf("SoftAP started. SSID='%s' Pass='%s' AP IP: %s\n",
                    apSsid.c_str(), apPass, apip.toString().c_str());
      Serial.println("Open this URL on your phone/laptop: http://192.168.4.1/");
    } else {
      Serial.println("SoftAP start failed.");
    }
  }
}
