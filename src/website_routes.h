#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "settings.h"

void setupRoutes(AsyncWebServer &srvr, const Settings &settings);

