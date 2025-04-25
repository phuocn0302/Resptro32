#include "wifi_config.h"

TaskHandle_t wifi_task_handle = NULL;
TaskHandle_t input_task_handle = NULL;
WiFiManager* wifiManager = NULL;
String wifi_ip = "Not Connected";
bool wifi_initialized = false;
bool wifi_config_active = false;

WiFiManagerParameter* wsServerParam;
WiFiManagerParameter* wsPortParam;
char wsServer[40] = "192.168.1.167";  // Default value
char wsPort[6] = "5173";              // Default value

void saveWsConfigCallback() {
    // Copy values to the global variables
    strncpy(wsServer, wsServerParam->getValue(), sizeof(wsServer));
    strncpy(wsPort, wsPortParam->getValue(), sizeof(wsPort));
    
    // Save to preferences
    Preferences preferences;
    preferences.begin("livepixel", false);
    preferences.putString("wsServer", wsServer);
    preferences.putString("wsPort", wsPort);
    preferences.end();
    
    draw_centered_text("Settings saved!", 135, TFT_GREEN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void loadWsConfig() {
    Preferences preferences;
    preferences.begin("livepixel", true);
    String savedServer = preferences.getString("wsServer", "");
    String savedPort = preferences.getString("wsPort", "");
    preferences.end();

    if (savedServer.length() > 0) {
        strncpy(wsServer, savedServer.c_str(), sizeof(wsServer));
    }
    if (savedPort.length() > 0) {
        strncpy(wsPort, savedPort.c_str(), sizeof(wsPort));
    }
}

void wifi_config_task(void *pvParameters) {
    if (!wifiManager) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("WiFiManager Init Failed", 60, TFT_RED, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
        wifi_config_active = false;
        vTaskDelete(NULL);
        return;
    }

    tft.fillScreen(TFT_BLACK);
    draw_centered_text("WiFi Config Mode", 10, TFT_WHITE, 1);
    draw_centered_text("Connect to WiFi AP:", 30, TFT_WHITE, 1);
    draw_centered_text("Resptro32-Config", 45, TFT_CYAN, 1);
    draw_centered_text("Then open:", 65, TFT_WHITE, 1);
    draw_centered_text("192.168.4.1", 80, TFT_GREEN, 1);
    draw_centered_text("in browser", 95, TFT_WHITE, 1);
    draw_centered_text("Press A to exit", 115, TFT_YELLOW, 1);

    // Ensure proper delay between mode changes
    WiFi.disconnect(true);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    WiFi.mode(WIFI_OFF);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    WiFi.mode(WIFI_AP_STA);
    vTaskDelay(pdMS_TO_TICKS(500));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    vTaskDelay(pdMS_TO_TICKS(500));
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    vTaskDelay(pdMS_TO_TICKS(1000));

    bool connected = false;
    try {
        connected = wifiManager->startConfigPortal("Resptro32-Config");
    } catch (...) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Config Portal Failed", 60, TFT_RED, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
        wifi_config_active = false;
        vTaskDelete(NULL);
        return;
    }

    if (connected) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Settings Saved!", 60, TFT_GREEN, 1);
        wifi_ip = WiFi.localIP().toString();
        String ipText = "IP: " + wifi_ip;
        draw_centered_text(ipText.c_str(), 80, TFT_WHITE, 1);
        String wsText = String("WS: ") + wsServer + ":" + wsPort;
        draw_centered_text(wsText.c_str(), 100, TFT_WHITE, 1);
        draw_centered_text("Press A", 120, TFT_WHITE, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
    } else {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Setup cancelled", 60, TFT_YELLOW, 1);
        draw_centered_text("Press A", 80, TFT_WHITE, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    wifi_config_active = false;
    vTaskDelete(NULL);
}

void wifi_config_launch() {
    if (wifi_config_active) {
        return;
    }

    // Load saved WebSocket settings
    loadWsConfig();

    if (wifiManager != NULL) {
        delete wifiManager;
        wifiManager = NULL;
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    vTaskDelay(pdMS_TO_TICKS(1000));

    try {
        wifiManager = new WiFiManager();
        if (!wifiManager) {
            tft.fillScreen(TFT_BLACK);
            draw_centered_text("Memory allocation failed", 60, TFT_RED, 1);
            return;
        }
    } catch (...) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("WiFiManager Init Failed", 60, TFT_RED, 1);
        return;
    }

    // Create custom parameters
    wsServerParam = new WiFiManagerParameter("server", "Live Pixel Server IP", wsServer, 40);
    wsPortParam = new WiFiManagerParameter("port", "Live Pixel Server Port", wsPort, 6);

    // Add parameters to WiFiManager
    wifiManager->addParameter(wsServerParam);
    wifiManager->addParameter(wsPortParam);

    wifiManager->setSaveConfigCallback(saveWsConfigCallback);
    wifiManager->setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    wifiManager->setHostname("Resptro32");
    wifiManager->setConfigPortalTimeout(0);
    wifiManager->setCleanConnect(true);
    wifiManager->setBreakAfterConfig(true);
    wifiManager->setDebugOutput(false);

    wifi_config_active = true;

    BaseType_t taskCreated = xTaskCreatePinnedToCore(
        wifi_config_task,
        "wifi_config_task",
        32768,
        NULL,
        2,
        &wifi_task_handle,
        0
    );

    if (taskCreated != pdPASS) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Task creation failed", 60, TFT_RED, 1);
        wifi_config_active = false;
        delete wifiManager;
        wifiManager = NULL;
    }
}

void wifi_config_exit() {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Exiting...");

    static bool exit_in_progress = false;
    if (exit_in_progress || !wifi_config_active) {
        return;
    }
    exit_in_progress = true;
    
    if (wifiManager != NULL) {
        wifiManager->stopConfigPortal();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (wifi_task_handle != NULL) {
        vTaskDelete(wifi_task_handle);
        wifi_task_handle = NULL;
    }

    if (input_task_handle != NULL) {
        vTaskDelete(input_task_handle);
        input_task_handle = NULL;
    }

    if (wifiManager != NULL) {
        delete wifiManager;
        wifiManager = NULL;
    }

    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    WiFi.begin();
    
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifi_ip = WiFi.localIP().toString();
    } else {
        wifi_ip = "Not Connected";
    }

    wifi_config_active = false;
    menu_requested = true;
    exit_in_progress = false;
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

String get_ws_url() {
    return String("ws://") + String(wsServer) + ":" + String(wsPort) + "/ws";
}
