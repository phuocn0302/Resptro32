#include "wifi_config.h"

TaskHandle_t wifi_task_handle = NULL;
TaskHandle_t input_task_handle = NULL;
WiFiManager wifiManager;
String wifi_ip = "Not Connected";
bool wifi_initialized = false;


void wifi_config_task(void *pvParameters) {
    tft.fillScreen(TFT_BLACK);
    draw_centered_text("WiFi Config Mode", 10, TFT_WHITE, 1);
    draw_centered_text("Connect to WiFi AP:", 30, TFT_WHITE, 1);
    draw_centered_text("Resptro32-Config", 45, TFT_CYAN, 1);
    draw_centered_text("Then open:", 65, TFT_WHITE, 1);
    draw_centered_text("192.168.4.1", 80, TFT_GREEN, 1);
    draw_centered_text("in browser", 95, TFT_WHITE, 1);
    draw_centered_text("Press A to exit", 115, TFT_YELLOW, 1);
    
    WiFi.mode(WIFI_AP_STA);    
    bool connected = wifiManager.startConfigPortal("Resptro32-Config");
    
    if (connected) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Connected to WiFi!", 60, TFT_GREEN, 1);
        wifi_ip = WiFi.localIP().toString();
        String ipText = "IP: " + wifi_ip;
        draw_centered_text(ipText.c_str(), 80, TFT_WHITE, 1);
        draw_centered_text("Press A", 100, TFT_WHITE, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
    } else {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("WiFi setup cancelled", 60, TFT_YELLOW, 1);
        draw_centered_text("Press A", 80, TFT_WHITE, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
    vTaskDelete(NULL);
}

void wifi_config_launch() {
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    wifiManager.setHostname("Resptro32");
    wifiManager.setConfigPortalTimeout(0); // No timeout
    
    wifiManager.setSaveConfigCallback([]() {
        draw_centered_text("WiFi credentials saved", 135, TFT_GREEN, 1);
    });
    
    xTaskCreatePinnedToCore(wifi_config_task, "wifi_config_task", 8192, NULL, 1, &wifi_task_handle, 0);        
}

void wifi_config_exit() {
    wifiManager.stopConfigPortal();
    

    vTaskDelay(pdMS_TO_TICKS(300));
    
    if (input_task_handle != NULL) {
        vTaskDelete(input_task_handle);
        input_task_handle = NULL;
    }
    
    if (wifi_task_handle != NULL) {
        vTaskDelete(wifi_task_handle);
        wifi_task_handle = NULL;
    }
    
    WiFi.mode(WIFI_STA);

    menu_requested = true;
}

bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

String get_wifi_ip() {
    if (wifi_is_connected()) {
        wifi_ip = WiFi.localIP().toString();
    } else {
        wifi_ip = "Not Connected";
    }
    return wifi_ip;
}