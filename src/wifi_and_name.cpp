#include "wifi_and_name.h"
#include <WiFi.h>
#ifdef ESP32
#include <ESPmDNS.h>
#endif

void connectWiFi(const Settings &settings) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings.wifiName.c_str(), settings.wifiPassword.c_str());
  Serial.print("Connecting");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
#ifdef ESP32
    if (MDNS.begin(settings.mdnsName.c_str())) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("mDNS: http://%s.local\n", settings.mdnsName.c_str());
    }
#endif
  } else {
    Serial.println("Wi-Fi not connected.");
  }
}

