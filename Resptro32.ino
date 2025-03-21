#include <SPI.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "pong_game.h"
#include "snake_game.h"
#include "live_pixel.h"

TFT_eSPI tft;
volatile GameState current_state = STATE_MENU;
int menu_selection = 0;
const char *game_names[MENU_ITEMS] = {"Snake", "Pong", "Live Pixel"};

void draw_centered_text(const char *text, int y, uint16_t color, int size) {
    tft.setTextSize(size);
    tft.setCursor((SCREEN_WIDTH - strlen(text) * 6 * size) / 2, y);
    tft.setTextColor(color, TFT_BLACK);
    tft.print(text);
}

void show_menu() {
    tft.fillScreen(TFT_BLACK);
    draw_centered_text("Select Game", 10, TFT_WHITE, 1);
    for (int i = 0; i < MENU_ITEMS; i++)
        draw_centered_text(game_names[i], 40 + i * 30,
                           (i == menu_selection) ? TFT_GREEN : TFT_WHITE, 1);
}

void IRAM_ATTR menu_button_ISR() {
    current_state = STATE_MENU;
    show_menu();
}

void handle_input(void *pv) {
    while (1) {
        if (current_state == STATE_MENU) {
            if (!digitalRead(BTN_UP)) {
                menu_selection = (menu_selection - 1 + MENU_ITEMS) % MENU_ITEMS;
                show_menu();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            if (!digitalRead(BTN_DOWN)) {
                menu_selection = (menu_selection + 1) % MENU_ITEMS;
                show_menu();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            if (!digitalRead(BTN_RIGHT)) {
                switch (menu_selection) {
                    case 0:
                        current_state = STATE_SNAKE;
                        snake_launch_tasks();
                        break;
                    case 1:
                        current_state = STATE_PONG;
                        pong_launch_tasks();
                        break;
                    case 2:
                        current_state = STATE_LIVE_PIXEL;
                        live_pixel_launch_tasks();
                        break;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    tft.init();
    tft.fillScreen(TFT_BLACK);
    
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_MENU, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BTN_MENU), menu_button_ISR, FALLING);

    snake_init_mutex();
    pong_init_mutex();
    live_pixel_init_mutex();

    show_menu();
    xTaskCreate(handle_input, "MainInput", 4096, NULL, 1, NULL);
}

void loop() {}