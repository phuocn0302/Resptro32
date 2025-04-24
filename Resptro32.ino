#include <User_Setup.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "pong_game.h"
#include "snake_game.h"
#include "live_pixel.h"

TFT_eSPI tft;
TFT_eSprite menuSprite = TFT_eSprite(&tft);
volatile GameState current_state = STATE_MENU;
int menu_selection = 0;
const char *game_names[MENU_ITEMS] = {"Snake", "Pong", "Live Pixel"};
volatile bool menu_requested = false;
volatile bool exit_requested = false;

#define ANIM_STEPS 24
#define ANIM_DELAY 1
bool animating = false;

const unsigned char snake_icon[32] = {
	0xc0, 0x03, 0x80, 0x01, 0x00, 0xfc, 0x01, 0xfe, 0x01, 0xb6, 0x01, 0xb6, 0x01, 0xfc, 0x0c, 0xe2, 
	0x1e, 0xf0, 0x1f, 0x78, 0x4f, 0xbc, 0x7f, 0xfc, 0x7d, 0xfc, 0x38, 0xf8, 0x80, 0x01, 0xc0, 0x03
};

const unsigned char pong_icon[32] = {
	0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 
	0x10, 0x48, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x01, 0xc0, 0x03
};

const unsigned char pixel_icon[32] = {
	0xc0, 0x03, 0x9f, 0xf9, 0x20, 0x04, 0x40, 0x02, 0x40, 0x02, 0x44, 0x22, 0x44, 0x22, 0x44, 0x22, 
	0x44, 0x22, 0x40, 0x02, 0x40, 0x02, 0x60, 0x06, 0x7f, 0xfe, 0x3f, 0xfc, 0x9f, 0xf9, 0xc0, 0x03
};

const unsigned char* game_icons[MENU_ITEMS] = {
    snake_icon, pong_icon, pixel_icon
};

void draw_centered_text(const char *text, int y, uint16_t color, int size) {
    tft.setTextSize(size);
    tft.setCursor((SCREEN_WIDTH - strlen(text) * 6 * size) / 2, y);
    tft.setTextColor(color, TFT_BLACK);
    tft.print(text);
}

void draw_icon_to_sprite(int x, int y, const unsigned char* icon, TFT_eSprite &sprite) {
    // upscale 4x from 16x16
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            uint8_t bit = icon[(i * 2) + (j / 8)] & (0x80 >> (j % 8));
            if (bit) {
                for (int sy = 0; sy < 4; sy++) {
                    for (int sx = 0; sx < 4; sx++) {
                         sprite.drawPixel(x + j * 4 + sx, y + i * 4 + sy, TFT_WHITE);
                    }
                }
            }
        }
    }
}

void draw_menu_item_to_sprite(int item_index, int x_offset, TFT_eSprite &sprite) {
    int center_x = SCREEN_WIDTH / 2 + x_offset;
    
    draw_icon_to_sprite(center_x - 32, 10, game_icons[item_index], sprite);

    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);
    int text_x = center_x - (strlen(game_names[item_index]) * 6) / 2;
    sprite.setCursor(text_x, 80);
    sprite.print(game_names[item_index]);
}

void show_menu() {
    tft.fillScreen(TFT_BLACK);
    draw_centered_text("Game Selection", 10, TFT_WHITE, 1);
    
    menuSprite.createSprite(SCREEN_WIDTH, 100);
    menuSprite.fillSprite(TFT_BLACK);
    
    draw_menu_item_to_sprite(menu_selection, 0, menuSprite);
    
    menuSprite.pushSprite(0, 30);
    menuSprite.deleteSprite();
}

void animate_menu_transition(int old_selection, int new_selection) {
    animating = true;
    
    int direction = (new_selection > old_selection) ? -1 : 1;
    if ((old_selection == 0 && new_selection == MENU_ITEMS - 1) || 
        (old_selection == MENU_ITEMS - 1 && new_selection == 0)) {
        direction = -direction;
    }

    menuSprite.createSprite(SCREEN_WIDTH * 2, 100);

    for (int step = 1; step <= ANIM_STEPS; step++) {
        menuSprite.fillSprite(TFT_BLACK);

        int offset_old = SCREEN_WIDTH/2 + direction * (SCREEN_WIDTH * step / ANIM_STEPS);
        int offset_new = offset_old - (direction * SCREEN_WIDTH);

        draw_menu_item_to_sprite(old_selection, offset_old, menuSprite);
        draw_menu_item_to_sprite(new_selection, offset_new, menuSprite);

        menuSprite.fillSprite(TFT_BLACK);
        draw_menu_item_to_sprite(old_selection, offset_old, menuSprite);
        draw_menu_item_to_sprite(new_selection, offset_new, menuSprite);
        
        
        menuSprite.pushSprite(0, 30, SCREEN_WIDTH/2, 0, SCREEN_WIDTH, 100);
        
        delay(ANIM_DELAY);
    }
    
    menuSprite.deleteSprite();
    animating = false;
}

void IRAM_ATTR menu_button_ISR() {
    if (current_state != STATE_MENU) {
        menu_requested = true;
        exit_requested = true;
    }
}

void handle_input(void *pv) {
    while (1) {
        if (current_state == STATE_MENU) {
            if (!digitalRead(BTN_LEFT) && !animating) {
                int old_selection = menu_selection;
                menu_selection = (menu_selection - 1 + MENU_ITEMS) % MENU_ITEMS;
                animate_menu_transition(old_selection, menu_selection);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            if (!digitalRead(BTN_RIGHT) && !animating) {
                int old_selection = menu_selection;
                menu_selection = (menu_selection + 1) % MENU_ITEMS;
                animate_menu_transition(old_selection, menu_selection);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            if (!digitalRead(BTN_B) && !animating) {
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
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BTN_A), menu_button_ISR, FALLING);

    snake_init_mutex();
    pong_init_mutex();
    live_pixel_init_queue();

    show_menu();
    xTaskCreate(handle_input, "MainInput", 4096, NULL, 1, NULL);
}

void loop() {
    if (exit_requested) {
        if (current_state == STATE_SNAKE) {
            snake_exit();
        } else if (current_state == STATE_PONG) {
            pong_exit();
        } else if (current_state == STATE_LIVE_PIXEL) {
            live_pixel_exit();
        }
        current_state = STATE_MENU;
        exit_requested = false;
    }
    
    if (menu_requested) {
        show_menu();
        menu_requested = false;
    }
    delay(10);
}
