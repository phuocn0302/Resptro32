#include "live_pixel.h"

const char *WIFI_SSID = "502B";         // Change to your WiFi SSID
const char *WIFI_PASSWORD = "110818032003";    // Change to your WiFi password
const char *SERVER_HOST = "ws://192.168.1.208:8080/ws";

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
        draw_centered_text("Connect server...", 145, TFT_WHITE, 1);
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

void draw_image(uint16_t* colorArray, int width, int height) {
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

    // Handle compressed batch pixel updates
    if (msg.startsWith("compressed;")) {
        // Format: "compressed;count;x1,y1,color1;x2,y2,color2;..."
        String countStr = msg.substring(11, msg.indexOf(';', 11));
        int expectedCount = countStr.toInt();
        String batchData = msg.substring(msg.indexOf(';', 11) + 1);

        // Create a temporary array to hold all pixels before sending to queue
        // This prevents queue overflow by processing all pixels at once
        PixelData* pixelBatch = NULL;
        int batchSize = 0;

        // Only allocate memory if we have a reasonable number of pixels
        if (expectedCount > 0 && expectedCount < 1024) {
            pixelBatch = new PixelData[expectedCount];

            // Parse all pixels first
            while (batchData.length() > 0 && batchSize < expectedCount) {
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
                    // Store in our batch array
                    pixelBatch[batchSize].x = x;
                    pixelBatch[batchSize].y = y;
                    pixelBatch[batchSize].color = colorRGB565;
                    batchSize++;
                }
            }

            // Now process the batch more efficiently
            // For large eraser operations, we can optimize by directly drawing to the screen
            // instead of using the queue which might overflow
            if (batchSize > 50) {
                // Direct screen update for large batches
                for (int i = 0; i < batchSize; i++) {
                    int px = pixelBatch[i].x * 4;
                    int py = pixelBatch[i].y * 4;
                    tft.fillRect(px, py, 4, 4, pixelBatch[i].color);
                }
            } else {
                // Use queue for smaller batches
                for (int i = 0; i < batchSize; i++) {
                    if (uxQueueSpacesAvailable(pixelQueue) > 0) {
                        xQueueSend(pixelQueue, &pixelBatch[i], 0);
                    }
                }
            }

            // Free the allocated memory
            delete[] pixelBatch;
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
    if(event == WebsocketsEvent::ConnectionOpened) {
        websocket_connected = true;
        tft.fillScreen(TFT_BLACK);
        reset_screen();
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
    PixelData pixelBatch[32]; // Process up to 32 pixels at once
    int batchCount = 0;
    TickType_t lastYield = xTaskGetTickCount();

    while (true) {
        // Process multiple pixels at once if available
        batchCount = 0;
        while (batchCount < 32 && xQueueReceive(pixelQueue, &pixelBatch[batchCount], 0) == pdTRUE) {
            batchCount++;
        }

        // If we got any pixels, process them
        if (batchCount > 0) {
            for (int i = 0; i < batchCount; i++) {
                int px = pixelBatch[i].x * 4;
                int py = pixelBatch[i].y * 4;
                tft.fillRect(px, py, 4, 4, pixelBatch[i].color);
            }

            // Yield periodically to prevent watchdog timer issues
            if (xTaskGetTickCount() - lastYield > pdMS_TO_TICKS(50)) {
                vTaskDelay(1); // Short delay to allow other tasks to run
                lastYield = xTaskGetTickCount();
            }
        } else {
            // If no pixels were available, wait for some to arrive
            if (xQueueReceive(pixelQueue, &pixel, pdMS_TO_TICKS(50))) {
                int px = pixel.x * 4;
                int py = pixel.y * 4;
                tft.fillRect(px, py, 4, 4, pixel.color);
            }
        }
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
    draw_centered_text("Connect server...", 145, TFT_WHITE, 1);

    bool connected = client.connect(SERVER_HOST);
    if (!connected) {
        tft.fillScreen(TFT_BLACK);
        draw_centered_text("Connect failed", 135, TFT_RED, 1);
        draw_centered_text("Retrying in 5s...", 145, TFT_WHITE, 1);
    }
    else {
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

void live_pixel_init_queue() {
    // Increase queue size to handle more pixels
    pixelQueue = xQueueCreate(4096, sizeof(PixelData));
}

void live_pixel_launch_tasks() {
    tft.fillScreen(TFT_BLACK);
    draw_centered_text("Starting...", 135, TFT_WHITE, 1);

    connect_wifi();

    client.onMessage(on_msg_callback);
    client.onEvent(on_events_callback);

    connect_server();

    xTaskCreatePinnedToCore(server_task, "server_task", 12288, NULL, 1, &server_task_handle, 0);
    xTaskCreatePinnedToCore(display_task, "display_task", 8192, NULL, 1, &display_task_handle, 1);
}

void live_pixel_exit() {
    if (websocket_connected) {
        client.close();
        websocket_connected = false;
    }

    if (server_task_handle != NULL) {
        vTaskDelete(server_task_handle);
        server_task_handle = NULL;
    }

    if (display_task_handle != NULL) {
        vTaskDelete(display_task_handle);
        display_task_handle = NULL;
    }

    if (pixelQueue != NULL) {
        xQueueReset(pixelQueue);
    }
}