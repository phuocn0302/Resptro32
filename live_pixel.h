#pragma once
#include "common.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>

void live_pixel_init_queue();
void live_pixel_launch_tasks();
void live_pixel_exit();