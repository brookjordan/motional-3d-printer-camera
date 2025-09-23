#pragma once

// EXAMPLE local credential overrides
// 1. Copy this file to config.local.h
// 2. Replace CHANGE_ME with your real credentials
// 3. config.local.h is git-ignored for security

// Example 1: Standard WPA2/WPA3 WiFi (home networks, most common)
#undef WPA_WIFI_PROFILE
#define WPA_WIFI_PROFILE                                                       \
  Config {                                                                     \
    .network = {.wifiSsid = "CHANGE_ME",                                       \
                .wifiPassword = "CHANGE_ME",                                   \
                .mdnsHostname = "my-camera"}                                   \
  }

// Example 2: WPA2-Enterprise WiFi (corporate networks with username/password)
#undef ENTERPRISE_WIFI_PROFILE
#define ENTERPRISE_WIFI_PROFILE                                                \
  Config {                                                                     \
    .network = {.wifiSsid = "CHANGE_ME",                                       \
                .mdnsHostname = "my-camera",                                   \
                .useEnterprise = true,                                         \
                .eapUsername = "CHANGE_ME",                                    \
                .eapPassword = "CHANGE_ME"},                                   \
    .camera = {.breathCycleDurationMs = 2000},                                 \
    .system = {.ledMaxBrightness = 20}                                         \
  }

// Profile selection happens in config.h with ACTIVE_PROFILE
// Just define both profiles here with your real credentials
