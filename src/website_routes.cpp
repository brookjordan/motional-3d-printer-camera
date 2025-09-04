#include "website_routes.h"
#include "settings.h"
#include "page_words.h"
#include "pictures.h"
#include <FFat.h>
#include <WiFi.h>
#include <string.h>

// ===== ESP-IDF headers needed for /json =====
extern "C" {
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
// FreeRTOS primitives for the status cache
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
}

// Try to include lwIP stats headers when available
#if defined(ESP32)
extern "C" {
#include "lwip/opt.h"
#include "lwip/stats.h"
}
#endif

static inline String uptimeStr() {
  uint64_t ms = millis();
  uint32_t s = ms / 1000, m = s / 60, h = m / 60, d = h / 24;
  char buf[64];
  snprintf(buf, sizeof(buf), "%ud %02uh:%02um:%02us", d, h % 24, m % 60,
           s % 60);
  return String(buf);
}

// Provide prototype for htmlEscape used by the status rows builder
static String htmlEscape(const String &in);

// Build safe HTML rows for the status table (placed early so cache task can use it)
static void buildStatusRowsHtml(String &out, const Settings &settings) {
  auto row = [&](const char *k, const String &vHtml) {
    const bool hasSubTable = vHtml.indexOf(F("<table")) >= 0;
    out += F("<tr><td>");
    out += k;
    out += F("</td><td");
    if (hasSubTable)
      out += F(" class=\"has-table\"");
    out += F(">");
    out += vHtml;
    out += F("</td></tr>");
  };

  esp_chip_info_t chip;
  esp_chip_info(&chip);
  row("chipModel", F("ESP32-S3"));
  row("chipRevision", String((unsigned)chip.revision));
  row("chipCores", String((unsigned)chip.cores));
  row("idfVersion", htmlEscape(esp_get_idf_version()));
  row("arduinoSdk", htmlEscape(ESP.getSdkVersion()));
  row("cpuFreqMHz", String((unsigned)ESP.getCpuFreqMHz()));

  const esp_app_desc_t *app = esp_app_get_description();
  row("appProjectName", htmlEscape(app ? app->project_name : ""));
  row("appVersion", htmlEscape(app ? app->version : ""));
  row("appBuildDate", htmlEscape(app ? app->date : ""));
  row("appBuildTime", htmlEscape(app ? app->time : ""));
  row("sketchMD5", htmlEscape(ESP.getSketchMD5()));

  row("resetReason", String((int)esp_reset_reason()));
  row("espTimer_us", String((unsigned long long)esp_timer_get_time()));
  row("uptime", htmlEscape(String(uptimeStr())));

  row("heapFree", String(ESP.getFreeHeap()));
  row("heapMinFreeEver", String(ESP.getMinFreeHeap()));
  row("heapMaxAlloc", String(ESP.getMaxAllocHeap()));
  row("heapIntFree", String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
  row("heapSPIRAM_Free", String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
  row("psramSize", String(ESP.getPsramSize()));
  row("psramFree", String(ESP.getFreePsram()));
  row("psramMinFreeEver", String(ESP.getMinFreePsram()));

  uint32_t fsize = 0, fid = 0;
  esp_flash_get_size(nullptr, &fsize);
  row("flashSize", String((unsigned long)fsize));
  row("flashSpeedHz", String(ESP.getFlashChipSpeed()));
  row("flashMode", String(ESP.getFlashChipMode()));
  esp_flash_read_id(nullptr, &fid);
  row("flashJedecID", String(fid));

  // Partitions subtable
  {
    String th =
        F("<table "
          "class='sub'><thead><tr><th>label</th><th>type</th><th>subtype</"
          "th><th>address</th><th>size</th></tr></thead><tbody>");
    const esp_partition_t *p = nullptr;
    int pcount = 0;
    esp_partition_iterator_t it = esp_partition_find(
        ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    while (it && (p = esp_partition_get(it)) && pcount < 64) {
      pcount++;
      th += F("<tr><td>");
      th += htmlEscape(p->label);
      th += F("</td><td>");
      th += String((unsigned)p->type);
      th += F("</td><td>");
      th += String((unsigned)p->subtype);
      th += F("</td><td>");
      th += String((uint32_t)p->address);
      th += F("</td><td>");
      th += String((uint32_t)p->size);
      th += F("</td></tr>");
      it = esp_partition_next(it);
    }
    if (it)
      esp_partition_iterator_release(it);
    th += F("</tbody></table>");
    row("partitions", th);
  }

  // OTA subtable
  {
    const esp_partition_t *runp = esp_ota_get_running_partition();
    const esp_partition_t *bootp = esp_ota_get_boot_partition();
    const esp_partition_t *nextp = esp_ota_get_next_update_partition(nullptr);
    String th = F("<table class='sub'><tbody>");
    if (runp) {
      th += F("<tr><th>running</th><td>");
      th += htmlEscape(runp->label);
      th += F("</td></tr>");
    }
    if (bootp) {
      th += F("<tr><th>boot</th><td>");
      th += htmlEscape(bootp->label);
      th += F("</td></tr>");
    }
    if (nextp) {
      th += F("<tr><th>next</th><td>");
      th += htmlEscape(nextp->label);
      th += F("</td></tr>");
    }
    th += F("</tbody></table>");
    row("ota", th);
  }

  // Wi-Fi
  row("ssid", htmlEscape(WiFi.SSID()));
  row("rssi", String(WiFi.RSSI()));
  row("channel", String(WiFi.channel()));
  row("mac", htmlEscape(WiFi.macAddress()));
  row("bssid", htmlEscape(WiFi.BSSIDstr()));
  row("hostname", htmlEscape(WiFi.getHostname() ? WiFi.getHostname() : ""));
  row("ip", htmlEscape(WiFi.localIP().toString()));
  row("gateway", htmlEscape(WiFi.gatewayIP().toString()));
  row("subnet", htmlEscape(WiFi.subnetMask().toString()));
  row("dns", htmlEscape(WiFi.dnsIP().toString()));
  row("mdnsHostname", htmlEscape(settings.mdnsName));
}

// ===== Cached status HTML (dual-core prebuild to keep request path light) =====
static String g_tbody_cached;               // full <tbody>...</tbody>
static SemaphoreHandle_t g_tbody_mutex = 0; // protects g_tbody_cached
static bool g_cache_task_started = false;
static Settings g_settings_snapshot;        // snapshot of settings used for status

static String getCachedTbody() {
  String out;
  if (g_tbody_mutex && xSemaphoreTake(g_tbody_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    out = g_tbody_cached; // copy
    xSemaphoreGive(g_tbody_mutex);
  }
  return out;
}

static void statusCacheTask(void *arg) {
  (void)arg;
  for (;;) {
    String rows;
    rows.reserve(4096);
    buildStatusRowsHtml(rows, g_settings_snapshot);
    String tb;
    tb.reserve(rows.length() + 16);
    tb += "<tbody>";
    tb += rows;
    tb += "</tbody>";

    if (g_tbody_mutex && xSemaphoreTake(g_tbody_mutex, portMAX_DELAY) == pdTRUE) {
      g_tbody_cached = tb; // swap
      xSemaphoreGive(g_tbody_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // update about once per second
  }
}

static void startStatusCacheIfNeeded(const Settings &settings) {
  if (!g_cache_task_started) {
    g_settings_snapshot = settings; // copy by value
    if (!g_tbody_mutex) g_tbody_mutex = xSemaphoreCreateMutex();
    TaskHandle_t h = nullptr;
    xTaskCreatePinnedToCore(statusCacheTask, "status_cache", 6144, nullptr, 1, &h, 1);
    g_cache_task_started = (h != nullptr);
  }
}

static void emit_lwip_stats(AsyncResponseStream *res) {
#if defined(LWIP_STATS) && LWIP_STATS
  res->print("\"lwip\":{");
  res->printf("\"ip.recv\":%u,", (unsigned)lwip_stats.ip.recv);
  res->printf("\"ip.xmit\":%u,", (unsigned)lwip_stats.ip.xmit);
  res->printf("\"ip.drop\":%u", (unsigned)lwip_stats.ip.drop);
  res->print("},");
#else
  res->print("\"lwip\":\"unavailable (LWIP_STATS=0)\",");
#endif
}

static void emit_freertos_stats(AsyncResponseStream *res) {
  res->print(
      "\"freertosStats\":\"enabled (populate uxTaskGetSystemState here)\",");
}

static void emit_sntp_details(AsyncResponseStream *res) {
  res->print("\"sntpDetails\":\"enabled (populate SNTP servers here)\",");
}

// Forward declaration for helpers used below (definition appears below)

static void handleJson(AsyncWebServerRequest *request,
                       const Settings &settings) {
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

  esp_chip_info_t chip;
  esp_chip_info(&chip);
  kvs("chipModel", "ESP32-S3");
  kv("chipRevision", chip.revision);
  kv("chipCores", chip.cores);
  kvs("idfVersion", esp_get_idf_version());
  kvs("arduinoSdk", ESP.getSdkVersion());
  kv("cpuFreqMHz", ESP.getCpuFreqMHz());

  const esp_app_desc_t *app = esp_app_get_description();
  kvs("appProjectName", app ? app->project_name : "");
  kvs("appVersion", app ? app->version : "");
  kvs("appBuildDate", app ? app->date : "");
  kvs("appBuildTime", app ? app->time : "");
  kvsS("sketchMD5", ESP.getSketchMD5());

  kv("resetReason", (int)esp_reset_reason());
  kv("espTimer_us", (uint64_t)esp_timer_get_time());
  kvs("uptime", uptimeStr().c_str());

  kv("heapFree", ESP.getFreeHeap());
  kv("heapMinFreeEver", ESP.getMinFreeHeap());
  kv("heapMaxAlloc", ESP.getMaxAllocHeap());
  kv("heapIntFree", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  kv("heapSPIRAM_Free", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  kv("psramSize", ESP.getPsramSize());
  kv("psramFree", ESP.getFreePsram());
  kv("psramMinFreeEver", ESP.getMinFreePsram());

  uint32_t fsize = 0, fid = 0;
  esp_flash_get_size(nullptr, &fsize);
  kv("flashSize", (uint64_t)fsize);
  kv("flashSpeedHz", ESP.getFlashChipSpeed());
  kv("flashMode", ESP.getFlashChipMode());
  esp_flash_read_id(nullptr, &fid);
  kv("flashJedecID", fid);

  res->print("\"partitions\":[");
  const esp_partition_t *p = nullptr;
  bool first = true;
  int pcount = 0;
  esp_partition_iterator_t it = esp_partition_find(
      ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
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

  kvs("mdnsHostname", settings.mdnsName.c_str());

  emit_lwip_stats(res);
  emit_freertos_stats(res);
  emit_sntp_details(res);

  kv("flashEncryptionEnabled", 0);

  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macf[18];
  snprintf(macf, sizeof(macf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
  kvs("efuseMAC_FACTORY", macf, /*last=*/true);

  res->print("}");
  request->send(res);
}

static void handleFavicon(AsyncWebServerRequest *request) {
  request->send(204);
}

static void handleFsList(AsyncWebServerRequest *request) {
  auto *res = request->beginResponseStream("text/plain; charset=utf-8");
  res->addHeader("Cache-Control", "no-cache");
  if (!FFat.begin()) {
    res->print("FFat NOT mounted\n");
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

// (definition moved earlier)

// Server-side rendered page with a prefilled table snapshot
static String htmlEscape(const String &in) {
  String out;
  out.reserve(in.length() + 16);
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c == '&')
      out += F("&amp;");
    else if (c == '<')
      out += F("&lt;");
    else if (c == '>')
      out += F("&gt;");
    else if (c == '"')
      out += F("&quot;");
    else
      out += c;
  }
  return out;
}

static void handlePrefilled(AsyncWebServerRequest *request,
                            const Settings &settings) {
  PageWords words; loadPageWords(words);
  auto *res = request->beginResponseStream("text/html; charset=utf-8");
  res->addHeader("Cache-Control", "no-cache");

  // Load template HTML from FFat
  String html;
  File f = FFat.open("/index.html", "r");
  if (f) {
    html.reserve((size_t)f.size() + 4096);
    while (f.available()) html += (char)f.read();
    f.close();
  } else {
    // Minimal fallback if template is missing
    html = F("<!doctype html><meta charset=\\\"utf-8\\\"><meta name=\\\"viewport\\\" content=\\\"width=device-width, initial-scale=1\\\">"
             "<noscript><meta http-equiv=\\\"refresh\\\" content=\\\"{{REFRESH_SECONDS}}\\\"></noscript>"
             "<link rel=\\\"stylesheet\\\" href=\\\"/app.css\\\">"
             "<img id=\\\"img\\\" src=\\\"/i/latest.jpg\\\" alt=\\\"latest frame\\\">"
             "<h1 id=\\\"heading\\\">{{HEADING}}</h1><p id=\\\"help\\\" class=\\\"muted\\\">{{HELP}}</p>"
             "<table id=\\\"t\\\"><thead><tr><th>Key</th><th>Value</th></tr></thead><tbody>{{STATUS_ROWS}}</tbody></table>"
             "<script type=\\\"module\\\" src=\\\"/js/entry.js\\\"></script>");
  }

  // Build status rows
  String rows;
  {
    String tb = getCachedTbody();
    if (tb.length() >= 16) {
      // Strip <tbody>...</tbody>
      int a = tb.indexOf('<');
      int b = tb.lastIndexOf("</tbody>");
      if (a >= 0 && b > a) {
        // find end of opening tag
        int endOpen = tb.indexOf('>');
        if (endOpen >= 0 && endOpen + 1 <= b) rows = tb.substring(endOpen + 1, b);
      }
    }
    if (!rows.length()) {
      rows.reserve(4096);
      buildStatusRowsHtml(rows, settings);
    }
  }

  // Replace tokens (simple substring rebuild, since Arduino String lacks insert)
  auto replaceToken = [](String &s, const char *from, const String &to) {
    int pos = 0; size_t flen = strlen(from);
    while ((pos = s.indexOf(from, pos)) != -1) {
      s = s.substring(0, pos) + to + s.substring(pos + (int)flen);
      pos += to.length();
    }
  };
  replaceToken(html, "{{TITLE}}", htmlEscape(words.pageTitle));
  replaceToken(html, "{{HEADING}}", htmlEscape(words.heading));
  replaceToken(html, "{{HELP}}", htmlEscape(words.helpLine));
  replaceToken(html, "{{STATUS_ROWS}}", rows);
  int refresh = settings.pageRefreshSeconds;
  if (refresh < 1) refresh = 1; // never emit 0; minimum 1s
  replaceToken(html, "{{REFRESH_SECONDS}}", String(refresh));

  res->print(html);
  request->send(res);
}

void setupRoutes(AsyncWebServer &srvr, const Settings &settings) {
  startStatusCacheIfNeeded(settings);
  // Dynamic overrides first (more specific), then static handlers.
  srvr.on("/i/latest.jpg", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->redirect(ImageRotator::getCurrentImage());
  });
  srvr.on("/photos/latest.jpg", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->redirect(ImageRotator::getCurrentImage());
  });

  // Static files
  srvr.serveStatic("/app.css", FFat, "/app.css");
  srvr.serveStatic("/js", FFat, "/js");
  srvr.serveStatic("/i", FFat, "/i")
      .setCacheControl("public, max-age=31536000, immutable");
  srvr.serveStatic("/photos", FFat, "/i")
      .setCacheControl("public, max-age=31536000, immutable");
  // Serve the root dynamically with a prefilled, safe HTML snapshot
  srvr.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    handlePrefilled(req, g_settings_snapshot);
  });
  srvr.on("/photos/rescan", HTTP_GET, [](AsyncWebServerRequest *req) {
    ImageRotator::rescan();
    req->send(200, "text/plain; charset=utf-8", "rescan ok");
  });

  srvr.on("/json", HTTP_GET, [](AsyncWebServerRequest *req) {
    handleJson(req, g_settings_snapshot);
  });
  // Optional: HTML status fragment (safe to inject into <tbody>)
  srvr.on("/status.html", HTTP_GET, [](AsyncWebServerRequest *req) {
    auto *res = req->beginResponseStream("text/html; charset=utf-8");
    String tb = getCachedTbody();
    if (!tb.length()) {
      // Fallback: compute once if cache not ready yet
      String rows; rows.reserve(4096);
      buildStatusRowsHtml(rows, g_settings_snapshot);
      tb = String("<tbody>") + rows + "</tbody>";
    }
    res->print(tb);
    req->send(res);
  });
  srvr.on("/prefilled", HTTP_GET, [](AsyncWebServerRequest *req) {
    handlePrefilled(req, g_settings_snapshot);
  });
  srvr.on("/favicon.ico", HTTP_GET, handleFavicon);
  srvr.on("/fs", HTTP_GET, handleFsList);
}
