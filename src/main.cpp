// ----- Wi-Fi / HTTP -----
// #include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#ifdef ESP32
#include <ESPmDNS.h>
#endif
#include "image_rotator.h"
#include "led_breath.h"
#include <FFat.h>
#include <math.h>

// ===== ESP-IDF headers (safe to include under Arduino core) =====
extern "C" {
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_flash_encrypt.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "spi_flash_mmap.h"
}

// image hashes
// 8e6a765e87869b07a9dcef1385e37368
// 136e7dc5856bc72011966cf9f0092376
// 446b9a73d396f546cba7e166001a44e7
// 40377e956f4efcab4e89540f8c385c47
// ade949e442e25206c5e97f1f3a4ebed9
// b09054584c3caf269b2433bfed2a1414
// c7358ed9156c216fdbf65c0815c71f77
// e68ac640c8c06c576e3ecbb10d9afb4e
// fd81cfa469bb650ecdedd67bf5ac517b

// ===== Optional unlocks (compile guards) =====
#define ENABLE_EFUSE_EXTENDED 1
#define ESP_EFUSE_FLASH_CRYPT_CNT 1
#define ENABLE_LWIP_MIB2 1
#define ENABLE_FREERTOS_STATS 1
#define ENABLE_SNTP_DETAILS 1

#if ENABLE_LWIP_MIB2
extern "C" {
#include "lwip/opt.h"
#include "lwip/stats.h"
}
#endif

// ===== Wi-Fi creds / mDNS name =====
const char *WIFI_SSID = "Brook";
const char *WIFI_PASS = "TryMyWiFi";
const char *MDNS_NAME = "esp32s3"; // http://esp32s3.local

// ===== HTTP server =====
AsyncWebServer srvr(80);

// ===== Helpers =====
static inline String uptimeStr() {
  uint64_t ms = millis();
  uint32_t s = ms / 1000, m = s / 60, h = m / 60, d = h / 24;
  char buf[64];
  snprintf(buf, sizeof(buf), "%ud %02uh:%02um:%02us", d, h % 24, m % 60,
           s % 60);
  return String(buf);
}

// ===== Optional emitters (guarded) =====
static void emit_lwip_stats(AsyncResponseStream *res) {
#if ENABLE_LWIP_MIB2
#if defined(LWIP_STATS) && LWIP_STATS
  res->print("\"lwip\":{");
  res->printf("\"ip.recv\":%u,", (unsigned)lwip_stats.ip.recv);
  res->printf("\"ip.xmit\":%u,", (unsigned)lwip_stats.ip.xmit);
  res->printf("\"ip.drop\":%u", (unsigned)lwip_stats.ip.drop);
  res->print("},");
#else
  res->print("\"lwip\":\"unavailable (LWIP_STATS=0)\",");
#endif
#else
  res->print("\"lwip\":\"unavailable (ENABLE_LWIP_MIB2=0)\",");
#endif
}

static void emit_freertos_stats(AsyncResponseStream *res) {
#if ENABLE_FREERTOS_STATS
  res->print(
      "\"freertosStats\":\"enabled (populate uxTaskGetSystemState here)\",");
#else
  res->print("\"freertosStats\":\"unavailable (ENABLE_FREERTOS_STATS=0)\",");
#endif
}

static void emit_sntp_details(AsyncResponseStream *res) {
#if ENABLE_SNTP_DETAILS
  res->print("\"sntpDetails\":\"enabled (populate SNTP servers here)\",");
#else
  res->print("\"sntpDetails\":\"unavailable (ENABLE_SNTP_DETAILS=0)\",");
#endif
}

// ===== /json: stream a big JSON object safely =====
void handleJson(AsyncWebServerRequest *request) {
  AsyncResponseStream *res = request->beginResponseStream("application/json");
  res->addHeader("Cache-Control", "no-cache");

  auto kv = [&](const char *k, uint64_t v, bool last = false) {
    res->print("\"");
    res->print(k);
    res->print("\":");
    res->print(v);
    if (!last)
      res->print(",");
  };
  auto kvs = [&](const char *k, const char *v, bool last = false) {
    res->print("\"");
    res->print(k);
    res->print("\":\"");
    res->print(v ? v : "");
    res->print("\"");
    if (!last)
      res->print(",");
  };
  auto kvsS = [&](const char *k, const String &v, bool last = false) {
    res->print("\"");
    res->print(k);
    res->print("\":\"");
    res->print(v);
    res->print("\"");
    if (!last)
      res->print(",");
  };

  res->print("{");

  // ---- System / SDK ----
  esp_chip_info_t chip;
  esp_chip_info(&chip);
  kvs("chipModel", "ESP32-S3");
  kv("chipRevision", chip.revision);
  kv("chipCores", chip.cores);
  kvs("idfVersion", esp_get_idf_version());
  kvs("arduinoSdk", ESP.getSdkVersion());
  kv("cpuFreqMHz", ESP.getCpuFreqMHz());

  // ---- App image ----
  const esp_app_desc_t *app = esp_app_get_description();
  kvs("appProjectName", app ? app->project_name : "");
  kvs("appVersion", app ? app->version : "");
  kvs("appBuildDate", app ? app->date : "");
  kvs("appBuildTime", app ? app->time : "");
  kvsS("sketchMD5", ESP.getSketchMD5());

  // ---- Reset & timers ----
  kv("resetReason", (int)esp_reset_reason());
  kv("espTimer_us", (uint64_t)esp_timer_get_time());
  kvs("uptime", uptimeStr().c_str());

  // ---- Heaps / PSRAM ----
  kv("heapFree", ESP.getFreeHeap());
  kv("heapMinFreeEver", ESP.getMinFreeHeap());
  kv("heapMaxAlloc", ESP.getMaxAllocHeap());
  kv("heapIntFree", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  kv("heapSPIRAM_Free", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  kv("psramSize", ESP.getPsramSize());
  kv("psramFree", ESP.getFreePsram());
  kv("psramMinFreeEver", ESP.getMinFreePsram());

  // ---- Flash ----
  uint32_t fsize = 0, fid = 0;
  esp_flash_get_size(nullptr, &fsize);
  kv("flashSize", (uint64_t)fsize);
  kv("flashSpeedHz", ESP.getFlashChipSpeed());
  kv("flashMode", ESP.getFlashChipMode());
  esp_flash_read_id(nullptr, &fid);
  kv("flashJedecID", fid);

  // ---- Partitions ----
  res->print("\"partitions\":[");
  const esp_partition_t *p = nullptr;
  esp_partition_iterator_t it = esp_partition_find(
      ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
  bool first = true;
  int pcount = 0;
  while (it && (p = esp_partition_get(it)) && pcount < 64) {
    if (!first)
      res->print(",");
    first = false;
    pcount++;
    res->print("{");
    kvs("label", p->label);
    kv("type", p->type);
    kv("subtype", p->subtype);
    kv("address", (uint32_t)p->address);
    kv("size", (uint32_t)p->size, /*last=*/true);
    res->print("}");
    it = esp_partition_next(it);
  }
  if (it)
    esp_partition_iterator_release(it);
  res->print("],");

  // ---- OTA block ----
  const esp_partition_t *runp = esp_ota_get_running_partition();
  const esp_partition_t *bootp = esp_ota_get_boot_partition();
  const esp_partition_t *nextp = esp_ota_get_next_update_partition(nullptr);
  res->print("\"ota\":{");
  bool needComma = false;
  if (runp) {
    res->print("\"running\":\"");
    res->print(runp->label);
    res->print("\"");
    needComma = true;
  }
  if (bootp) {
    if (needComma)
      res->print(",");
    res->print("\"boot\":\"");
    res->print(bootp->label);
    res->print("\"");
    needComma = true;
  }
  if (nextp) {
    if (needComma)
      res->print(",");
    res->print("\"next\":\"");
    res->print(nextp->label);
    res->print("\"");
  }
  res->print("},");

  // ---- Wi-Fi / netif ----
  kvsS("ssid", WiFi.SSID());
  kv("rssi", WiFi.RSSI());
  kv("channel", WiFi.channel());
  kvsS("mac", WiFi.macAddress());
  kvsS("bssid", WiFi.BSSIDstr());
  kvs("hostname", WiFi.getHostname() ? WiFi.getHostname() : "");
  kvsS("ip", WiFi.localIP().toString());
  kvsS("gateway", WiFi.gatewayIP().toString());
  kvsS("subnet", WiFi.subnetMask().toString());
  kvsS("dns", WiFi.dnsIP().toString());

  // ---- mDNS ----
#ifdef ESP32
  kvs("mdnsHostname", MDNS_NAME);
#else
  kvs("mdnsHostname", "unavailable");
#endif

  emit_lwip_stats(res);
  emit_freertos_stats(res);
  emit_sntp_details(res);

#if ESP_EFUSE_FLASH_CRYPT_CNT
  kv("flashEncryptionEnabled", esp_flash_encryption_enabled() ? 1 : 0);
#else
  kvs("flashEncryptionEnabled", "unavailable (ENABLE_EFUSE_EXTENDED=0)");
#endif

#if ENABLE_EFUSE_EXTENDED
  kvs("efuseExtendedNote", "enabled but not implemented here");
#else
  kvs("efuseExtendedNote", "unavailable (ENABLE_EFUSE_EXTENDED=0)");
#endif

  // ---- eFuse / security ----
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macf[18];
  snprintf(macf, sizeof(macf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
  kvs("efuseMAC_FACTORY", macf, /*last=*/true);

  res->print("}");

  request->send(res);
}

// ===== HTML UI with nested tables + robust retry/backoff =====
void handleRoot(AsyncWebServerRequest *request) {
  String html = "<!doctype html>"
                "<meta charset=utf-8>"
                "<title>ESP32-S3 Secrets</title>"
                "<link rel='stylesheet' href='/app.css'>"
                ""
                "<img id='img' src='/i/latest.jpg'>"
                ""
                "<h1 class='secrets-header'>ESP32-S3 Secrets</h1>"
                "<p class=muted>Live, streaming diagnostics. Complex values "
                "render as nested tables.</p>"
                "<table id=t>"
                "  <thead>"
                "    <tr>"
                "      <th>Key</th>"
                "      <th>Value</th>"
                "    </tr>"
                "  </thead>"
                "  <tbody></tbody>"
                "</table>"
                ""
                "<script type='module' src='/js/entry.js'></script>";
  request->send(200, "text/html; charset=utf-8", html);
}
void handleFavicon(AsyncWebServerRequest *request) { request->send(204); }
void handleFsList(AsyncWebServerRequest *request) {
  auto *res = request->beginResponseStream("text/plain; charset=utf-8");
  res->addHeader("Cache-Control", "no-cache");

  if (!FFat.begin()) {                // ideally mount FFat once in setup()
    res->print("FFat NOT mounted\n"); // and just report status here
    request->send(res);
    return;
  }
  res->print("FFat mounted\n");

#if defined(ARDUINO_ARCH_ESP32)
  res->printf("totalBytes: %llu\n", (unsigned long long)FFat.totalBytes());
  res->printf("usedBytes : %llu\n", (unsigned long long)FFat.usedBytes());
#endif

  File root = FFat.open("/");
  if (!root) {
    res->print("open('/') failed\n");
    request->send(res);
    return;
  }
  if (!root.isDirectory()) {
    res->print("'/' is not a dir\n");
    request->send(res);
    return;
  }

  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    res->print(f.isDirectory() ? "[DIR] " : "      ");
    res->print(f.name());
    res->print("  ");
    res->printf("%u bytes\n", (unsigned)f.size());
    f.close();
  }

  request->send(res);
}

// ===== Wi-Fi connect =====
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
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
    if (MDNS.begin(MDNS_NAME)) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("mDNS: http://%s.local\n", MDNS_NAME);
    }
  } else {
    Serial.println("Wi-Fi not connected.");
  }
}

// ===== Setup / Loop =====
void setup() {
  Serial.begin(115200);
  delay(200);

  LedBreath::setup();
  ImageRotator::setup();

  // --- MOUNT FS + REGISTER STATIC FILES BEFORE server.begin() ---
  if (!FFat.begin()) {
    Serial.println("FFat mount failed");
  } else {
    srvr.serveStatic("/app.css", FFat, "/app.css");
    srvr.serveStatic("/js", FFat, "/js");
    srvr.serveStatic("/i", FFat, "/i")
        .setCacheControl("public, max-age=31536000, immutable");
    srvr.on("/i/latest.jpg", HTTP_GET, [](AsyncWebServerRequest *req) {
      req->redirect(ImageRotator::getCurrentImage());
    });
  }

  srvr.on("/", HTTP_GET, handleRoot);
  srvr.on("/json", HTTP_GET, handleJson);
  srvr.on("/favicon.ico", HTTP_GET, handleFavicon);
  srvr.on("/fs", HTTP_GET, handleFsList);

  connectWiFi();
  srvr.begin();

  Serial.println("HTTP server started");
  Serial.println(
      "Type a number 0..255 in Serial Monitor to set max brightness.");
}

void loop() {
  LedBreath::loop();
  ImageRotator::tick();
}
