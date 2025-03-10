#pragma once
#include <TFT_eSPI.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <TFT_eSPI.h>

// Common constants and enums
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
#define BORDER_SIZE 4
#define MENU_ITEMS 2

enum GameState { STATE_MENU, STATE_SNAKE, STATE_PONG};
extern TFT_eSPI tft;  // Declaration for external access
extern volatile GameState current_state;  // Declaration
extern int menu_selection;
extern const char *game_names[MENU_ITEMS];

// Button pins
enum Buttons {
  BTN_UP = 12,
  BTN_LEFT = 26,
  BTN_DOWN = 25,
  BTN_RIGHT = 33,
  BTN_MENU = 27
};

// Common structures
struct Position { int x; int y; };

void show_menu();
void draw_centered_text(const char *text, int y, uint16_t color, int size);