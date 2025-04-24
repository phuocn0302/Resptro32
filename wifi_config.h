
#pragma once
#include "common.h"
#include <WiFiManager.h>

void wifi_config_launch();
void wifi_config_exit();
bool wifi_is_connected();
String get_wifi_ip();