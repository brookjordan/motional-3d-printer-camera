#pragma once

// Local credential overrides - this file is git-ignored
// Copy your real WiFi credentials here

#undef HOME_PROFILE
#define HOME_PROFILE                                                           \
  Config {                                                                     \
    .network = {.wifiSsid = "Brook",                                           \
                .wifiPassword = "TryMyWiFi",                                   \
                .mdnsHostname = "brook-and-lily-cam"}                          \
  }

#undef OFFICE_PROFILE
#define OFFICE_PROFILE                                                         \
  Config {                                                                     \
    .network = {.wifiSsid = "mojo",                                            \
                .mdnsHostname = "motional-3d-printer",                         \
                .useEnterprise = true,                                         \
                .eapUsername = "brook.jordan",                                 \
                .eapPassword = "TICKET9lease.winner"},                         \
    .camera = {.breathCycleDurationMs = 2000},                                 \
    .system = {.ledMaxBrightness = 20}                                         \
  }
