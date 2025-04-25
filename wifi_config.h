
#pragma once
#include "common.h"
#include <WiFiManager.h>
#include <Preferences.h>

void wifi_config_launch();
void wifi_config_exit();
bool wifi_is_connected();
String get_wifi_ip();
String get_ws_url();

extern bool wifi_config_active;
extern String wifi_ip;
