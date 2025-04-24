#include "live_pixel.h"
#include "wifi_config.h"

const char *SERVER_HOST = "ws://192.168.1.167:5173/ws";

using namespace websockets;
WebsocketsClient client;

QueueHandle_t pixelQueue;
TaskHandle_t server_task_handle = NULL;
TaskHandle_t display_task_handle = NULL;

struct PixelData {
    int x, y;
    uint16_t color;
};

String esp32_ip = "Connecting...";
bool websocket_connected = false;
unsigned long last_reconnect_attempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;  // 5 seconds between reconnection attempts

volatile bool initialization_complete = false;
volatile bool exit_in_progress = false;

void reset_screen() { tft.fillRect(0, 0, 128, 128, TFT_WHITE); }

void draw_image(uint16_t *colorArray, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            int px = x * 4;
            int py = y * 4;
            tft.fillRect(px, py, 4, 4, colorArray[index]);
        }
    }
}

void on_msg_callback(WebsocketsMessage message) {
    String msg = message.data();

    if (msg.startsWith("full,")) {
        String pixelData = msg.substring(5);
        uint16_t imageArray[1024];
        int index = 0;

        while (pixelData.length() > 0 && index < 1024) {
            int commaPos = pixelData.indexOf(',');
            String colorHex;

            if (commaPos > 0) {
                colorHex = pixelData.substring(0, commaPos);
                pixelData = pixelData.substring(commaPos + 1);
            } else {
                colorHex = pixelData;
                pixelData = "";
            }

            imageArray[index] = (uint16_t)strtol(colorHex.c_str(), NULL, 16);
            index++;
        }

        draw_image(imageArray, 32, 32);
        return;
    }

    // Static buffer for pixel data to avoid repeated memory allocation
    static PixelData pixelBuffer[64];  // Fixed-size buffer for pixel data

    // Handle chunked batch updates (new optimized format)
    if (msg.startsWith("chunk;")) {
        // Format: "chunk;chunk_index;total_chunks;count;x1,y1,color1;x2,y2,color2;..."
        int firstSemi = msg.indexOf(';');
        int secondSemi = msg.indexOf(';', firstSemi + 1);
        int thirdSemi = msg.indexOf(';', secondSemi + 1);
        int fourthSemi = msg.indexOf(';', thirdSemi + 1);

        if (firstSemi < 0 || secondSemi < 0 || thirdSemi < 0 || fourthSemi < 0) {
            return;  // Invalid format
        }

        // Parse chunk metadata
        int chunkIndex = msg.substring(firstSemi + 1, secondSemi).toInt();
        int totalChunks = msg.substring(secondSemi + 1, thirdSemi).toInt();
        int chunkSize = msg.substring(thirdSemi + 1, fourthSemi).toInt();
        String pixelData = msg.substring(fourthSemi + 1);

        // Process pixels in this chunk
        int pixelCount = 0;

        // Parse all pixels in this chunk
        while (pixelData.length() > 0 && pixelCount < chunkSize && pixelCount < 64) {
            int semicolonPos = pixelData.indexOf(';');
            String pixelInfo;

            if (semicolonPos > 0) {
                pixelInfo = pixelData.substring(0, semicolonPos);
                pixelData = pixelData.substring(semicolonPos + 1);
            } else {
                pixelInfo = pixelData;
                pixelData = "";
            }

            // Process individual pixel
            int x, y;
            uint16_t colorRGB565;
            sscanf(pixelInfo.c_str(), "%d,%d,%hx", &x, &y, &colorRGB565);

            if (x >= 0 && x < 32 && y >= 0 && y < 32) {
                // Store in our buffer
                pixelBuffer[pixelCount].x = x;
                pixelBuffer[pixelCount].y = y;
                pixelBuffer[pixelCount].color = colorRGB565;
                pixelCount++;
            }
        }

        // Process all pixels directly to screen for better performance
        for (int i = 0; i < pixelCount; i++) {
            int px = pixelBuffer[i].x * 4;
            int py = pixelBuffer[i].y * 4;
            tft.fillRect(px, py, 4, 4, pixelBuffer[i].color);
        }

        return;
    }

    // Handle compressed batch pixel updates (legacy support)
    if (msg.startsWith("compressed;")) {
        // Format: "compressed;count;x1,y1,color1;x2,y2,color2;..."
        String countStr = msg.substring(11, msg.indexOf(';', 11));
        int expectedCount = countStr.toInt();
        String batchData = msg.substring(msg.indexOf(';', 11) + 1);

        // Limit the number of pixels we process to avoid memory issues
        const int maxPixels = 64;
        int pixelCount = 0;

        // Parse pixels up to our buffer size
        while (batchData.length() > 0 && pixelCount < maxPixels && pixelCount < expectedCount) {
            int semicolonPos = batchData.indexOf(';');
            String pixelInfo;

            if (semicolonPos > 0) {
                pixelInfo = batchData.substring(0, semicolonPos);
                batchData = batchData.substring(semicolonPos + 1);
            } else {
                pixelInfo = batchData;
                batchData = "";
            }

            // Process individual pixel
            int x, y;
            uint16_t colorRGB565;
            sscanf(pixelInfo.c_str(), "%d,%d,%hx", &x, &y, &colorRGB565);

            if (x >= 0 && x < 32 && y >= 0 && y < 32) {
                // Store in our buffer
                pixelBuffer[pixelCount].x = x;
                pixelBuffer[pixelCount].y = y;
                pixelBuffer[pixelCount].color = colorRGB565;
                pixelCount++;
            }
        }

        // Process all pixels directly to screen
        for (int i = 0; i < pixelCount; i++) {
            int px = pixelBuffer[i].x * 4;
            int py = pixelBuffer[i].y * 4;
            tft.fillRect(px, py, 4, 4, pixelBuffer[i].color);
        }

        return;
    }

    // Legacy batch format support
    if (msg.startsWith("batch;")) {
        // Just ignore old format batches, as we now use compressed format
        return;
    }

    // Handle single pixel updates and clear command
    int x, y;
    uint16_t colorRGB565;
    sscanf(message.data().c_str(), "%d,%d,%hx", &x, &y, &colorRGB565);

    if (x == -1 && y == -1) {
        reset_screen();
    }

    if (x >= 0 && x < 32 && y >= 0 && y < 32) {
        PixelData pixel = {x, y, colorRGB565};

        if (uxQueueSpacesAvailable(pixelQueue) > 0) {
            xQueueSend(pixelQueue, &pixel, portMAX_DELAY);
        }
    }
}

void on_events_callback(WebsocketsEvent event, String data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
        websocket_connected = true;
        tft.fillScreen(TFT_BLACK);
        reset_screen();
        draw_centered_text("Connected!", 135, TFT_GREEN, 1);
        String ipText = "IP: " + esp32_ip;
        draw_centered_text(ipText.c_str(), 145, TFT_WHITE, 1);
    } else if (event == WebsocketsEvent::ConnectionClosed) {
        websocket_connected = false;
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Disconnected", 135, TFT_RED, 1);
        draw_centered_text("Reconnecting...", 145, TFT_WHITE, 1);
    }
}

void display_task(void *pvParameters) {
    PixelData pixel;
    PixelData pixelBatch[32];
    int batchCount = 0;
    TickType_t lastYield = xTaskGetTickCount();

    while (true) {
        batchCount = 0;
        while (batchCount < 32 && xQueueReceive(pixelQueue, &pixelBatch[batchCount], 0) == pdTRUE) {
            batchCount++;
        }

        if (batchCount > 0) {
            int startX = pixelBatch[0].x;
            int startY = pixelBatch[0].y;
            uint16_t currentColor = pixelBatch[0].color;
            int width = 1;

            for (int i = 1; i < batchCount; i++) {
                if (pixelBatch[i].y == startY && pixelBatch[i].x == startX + width && pixelBatch[i].color == currentColor) {
                    width++;
                } else {
                    int px = startX * 4;
                    int py = startY * 4;
                    tft.fillRect(px, py, width * 4, 4, currentColor);

                    startX = pixelBatch[i].x;
                    startY = pixelBatch[i].y;
                    currentColor = pixelBatch[i].color;
                    width = 1;
                }
            }

            int px = startX * 4;
            int py = startY * 4;
            tft.fillRect(px, py, width * 4, 4, currentColor);

            if (xTaskGetTickCount() - lastYield > pdMS_TO_TICKS(20)) {
                vTaskDelay(1);
                lastYield = xTaskGetTickCount();
            }
        } else {
            if (xQueueReceive(pixelQueue, &pixel, pdMS_TO_TICKS(20))) {
                int px = pixel.x * 4;
                int py = pixel.y * 4;
                tft.fillRect(px, py, 4, 4, pixel.color);
            } else {
                vTaskDelay(1);
            }
        }
    }
}

void connect_server() {
    if (current_state == STATE_MENU) return;

    if (websocket_connected) {
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("WiFi Disconnected", 60, TFT_RED, 1);
        draw_centered_text("Press A to exit", 80, TFT_WHITE, 1);
        exit_requested = true;
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
    draw_centered_text("Connect server...", 145, TFT_WHITE, 1);

    bool connected = client.connect(SERVER_HOST);
    if (!connected) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Connect failed", 135, TFT_RED, 1);
        draw_centered_text("Retrying in 5s...", 145, TFT_WHITE, 1);
    } else {
        reset_screen();
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

void handle_exit_button(void *pvParameters) {
    while (!exit_in_progress) {
        if (!digitalRead(BTN_A)) {
            exit_requested = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelete(NULL);
}

void live_pixel_init_queue() {
    // Create a smaller queue size - we're now processing pixels in chunks
    // so we don't need such a large queue
    pixelQueue = xQueueCreate(256, sizeof(PixelData));
}

void live_pixel_launch_tasks() {
    initialization_complete = false;
    exit_in_progress = false;
    websocket_connected = false;
    last_reconnect_attempt = 0;
    esp32_ip = "Connecting...";
    server_task_handle = NULL;
    display_task_handle = NULL;

    tft.fillScreen(TFT_BLACK);
    draw_centered_text("Starting Live Pixel...", 40, TFT_WHITE, 1);
    
    // Check WiFi connection first
    if (WiFi.status() != WL_CONNECTED) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("No WiFi Connection", 40, TFT_YELLOW, 1);
        draw_centered_text("Live Pixel requires WiFi", 60, TFT_WHITE, 1);
        draw_centered_text("Go to WiFi Config", 80, TFT_WHITE, 1);
        draw_centered_text("Press A to exit", 110, TFT_CYAN, 1);
        
        // Create task to handle exit button
        TaskHandle_t exitTaskHandle;
        xTaskCreatePinnedToCore(handle_exit_button, "exit_button_task", 2048, NULL, 1, &exitTaskHandle, 1);
        
        // We won't continue initialization
        return;
    }

    if (exit_in_progress) {
        live_pixel_exit();
        return;
    }

    if (pixelQueue != NULL) {
        vQueueDelete(pixelQueue);
    }
    pixelQueue = xQueueCreate(256, sizeof(PixelData));
    if (pixelQueue == NULL) {
        draw_centered_text("Queue Init Failed!", 135, TFT_RED, 1);
        return;
    }

    if (exit_in_progress) {
        live_pixel_exit();
        return;
    }

    // Get WiFi IP address
    esp32_ip = WiFi.localIP().toString();
    
    tft.fillScreen(TFT_BLACK);
    String ipText = "IP: " + esp32_ip;
    draw_centered_text(ipText.c_str(), 135, TFT_WHITE, 1);
    draw_centered_text("Connect server...", 145, TFT_WHITE, 1);

    if (exit_in_progress) {
        live_pixel_exit();
        return;
    }

    client.onMessage(on_msg_callback);
    client.onEvent(on_events_callback);

    connect_server();

    if (exit_in_progress) {
        live_pixel_exit();
        return;
    }

    BaseType_t serverTaskCreated = xTaskCreatePinnedToCore(server_task, "server_task", 12288, NULL, 1, &server_task_handle, 0);

    if (serverTaskCreated != pdPASS) {
        draw_centered_text("Server Task Failed!", 135, TFT_RED, 1);
        live_pixel_exit();
        return;
    }

    // Check if exit was requested during startup
    if (exit_in_progress) {
        live_pixel_exit();
        return;
    }

    BaseType_t displayTaskCreated = xTaskCreatePinnedToCore(display_task, "display_task", 8192, NULL, 1, &display_task_handle, 1);

    if (displayTaskCreated != pdPASS) {
        if (server_task_handle != NULL) {
            vTaskDelete(server_task_handle);
            server_task_handle = NULL;
        }
        draw_centered_text("Display Task Failed!", 135, TFT_RED, 1);
        live_pixel_exit();
        return;
    }
    
    // Create task to handle exit button
    TaskHandle_t exitTaskHandle;
    xTaskCreatePinnedToCore(handle_exit_button, "exit_button_task", 2048, NULL, 1, &exitTaskHandle, 1);

    initialization_complete = true;
}

void live_pixel_exit() {
    if (exit_in_progress) return;
    exit_in_progress = true;

    if (!initialization_complete) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    tft.fillScreen(TFT_BLACK);

    if (server_task_handle != NULL) {
        vTaskDelete(server_task_handle);
        server_task_handle = NULL;
    }

    if (display_task_handle != NULL) {
        vTaskDelete(display_task_handle);
        display_task_handle = NULL;
    }

    if (pixelQueue != NULL) {
        vQueueDelete(pixelQueue);
        pixelQueue = NULL;
    }

    if (websocket_connected) {
        client.close();
        websocket_connected = false;
    }

    websocket_connected = false;
    last_reconnect_attempt = 0;
    esp32_ip = "Connecting...";
    initialization_complete = false;
    exit_in_progress = false;

    tft.fillScreen(TFT_BLACK);
    menu_requested = true;
    vTaskDelay(pdMS_TO_TICKS(50));
}