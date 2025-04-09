#pragma once
#include <Arduino.h> 
#include <TFT_eSPI.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Common constants and enums
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
#define BORDER_SIZE 4
#define MENU_ITEMS 3 

enum GameState { STATE_MENU, STATE_SNAKE, STATE_PONG, STATE_LIVE_PIXEL };
extern TFT_eSPI tft;
extern volatile GameState current_state;
extern int menu_selection;
extern const char *game_names[MENU_ITEMS];

// Button pins
enum Buttons {
  BTN_UP = 12,
  BTN_LEFT = 26,
  BTN_DOWN = 25,
  BTN_RIGHT = 33,
  BTN_A = 27,
  BTN_B = 32,
  BTN_ALT = 13,
  BTN_SHIFT = 32,
};

// Common structures
struct Position { int x; int y; };

void show_menu();
void draw_centered_text(const char *text, int y, uint16_t color, int size);
