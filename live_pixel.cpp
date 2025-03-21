#include "live_pixel.h"
#include "common.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>

const char *WIFI_SSID = "";         // Change to your WiFi SSID
const char *WIFI_PASSWORD = "";    // Change to your WiFi password
// Auto-connect to the server running on the computer
const char *SERVER_HOST = "ws://ip:port/ws"; // Replace with your computer's IP

using namespace websockets;
WebsocketsClient client;
QueueHandle_t pixelQueue;
String esp32_ip = "Connecting...";
bool websocket_connected = false;
unsigned long last_reconnect_attempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds between reconnection attempts

void connect_wifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempt = 0;
    
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        tft.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        attempt++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        esp32_ip = WiFi.localIP().toString();
        
        tft.fillScreen(TFT_BLACK);
        String ipText = "IP: " + esp32_ip;
        draw_centered_text(ipText.c_str(), 135, TFT_WHITE, 1);
        draw_centered_text("Connecting to server...", 145, TFT_WHITE, 1);
    } else {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("WiFi Failed", 135, TFT_RED, 1);
        draw_centered_text("Retrying in 5s...", 145, TFT_WHITE, 1);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void reset_screen()
{
    tft.fillRect(0, 0, 128, 128, TFT_WHITE); 
}

void on_msg_callback(WebsocketsMessage message) {
    int x, y;
    uint16_t colorRGB565;
    sscanf(message.data().c_str(), "%d,%d,%hx", &x, &y, &colorRGB565);

    if (x == -1 && y == -1) {
        reset_screen();
    }

    if (x >= 0 && x < 64 && y >= 0 && y < 64) {
        PixelData pixel = {x, y, colorRGB565};
        
        if (uxQueueSpacesAvailable(pixelQueue) > 0) {
            xQueueSend(pixelQueue, &pixel, portMAX_DELAY);
        }
    }
}

void on_events_callback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        websocket_connected = true;
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Connected!", 135, TFT_GREEN, 1);
        String ipText = "IP: " + esp32_ip;
        draw_centered_text(ipText.c_str(), 145, TFT_WHITE, 1);
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        websocket_connected = false;
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Disconnected", 135, TFT_RED, 1);
        draw_centered_text("Reconnecting...", 145, TFT_WHITE, 1);
    }
}

void display_task(void *pvParameters) {
    PixelData pixel;
    while (true) {
        if (xQueueReceive(pixelQueue, &pixel, portMAX_DELAY)) {
            int px = pixel.x * 4;
            int py = pixel.y * 4;
            tft.fillRect(px, py, 4, 4, pixel.color);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void connect_server() {
    if (websocket_connected) {
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        connect_wifi();
        return;
    }
    
    unsigned long current_time = millis();
    if (current_time - last_reconnect_attempt < RECONNECT_INTERVAL) {
        return;
    }
    last_reconnect_attempt = current_time;
    
    tft.fillScreen(TFT_BLACK);
    String ipText = "IP: " + esp32_ip;
    draw_centered_text(ipText.c_str(), 135, TFT_WHITE, 1);
    draw_centered_text("Connecting to server...", 145, TFT_WHITE, 1);
    reset_screen();
    
    bool connected = client.connect(SERVER_HOST);
    if (!connected) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Connect failed", 135, TFT_RED, 1);
        draw_centered_text("Retrying in 5s...", 145, TFT_WHITE, 1);
    }
}

void server_task(void *pvParameters) {
    while (true) {
        if (client.available()) {
            client.poll();
        } else if (!websocket_connected) {
            connect_server();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void live_pixel_init_mutex() {
    pixelQueue = xQueueCreate(500, sizeof(PixelData));
}

void live_pixel_launch_tasks() {
    tft.fillScreen(TFT_BLACK);
    draw_centered_text("Starting...", 135, TFT_WHITE, 1);
    

    connect_wifi();

    client.onMessage(on_msg_callback);
    client.onEvent(on_events_callback);
    
    connect_server();
    tft.fillRect(0, 0, 128, 128, TFT_WHITE); 

    xTaskCreatePinnedToCore(server_task, "server_task", 12288, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(display_task, "display_task", 8192, NULL, 1, NULL, 1);
}

void live_pixel_loop() {}