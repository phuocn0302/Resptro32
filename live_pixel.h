#pragma once
#include "common.h"
#include <WiFi.h>
#include <WebSocketsServer.h>

struct PixelData {
    int x, y;
    uint16_t color;
};

void live_pixel_init_mutex();
void live_pixel_launch_tasks();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void live_pixel_loop();