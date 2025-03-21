#include "common.h"
#include "snake_game.h"

struct SnakeGame {
    Position segments[100];
    Position food;
    int length;
    int dx;
    int dy;
    int speed;
    uint8_t running;
};

// Snake game constants
const int SNAKE_SEGMENT_SIZE = 4;
const int INITIAL_SNAKE_SPEED = 100;
const Position START_POSITION = {60, 80};

SnakeGame snake;
SemaphoreHandle_t snake_mutex;

// Direction vectors: [Up, Left, Down, Right]
const int DIRECTION_VECTORS[4][2] = {
    {0, -SNAKE_SEGMENT_SIZE},  // Up
    {-SNAKE_SEGMENT_SIZE, 0},  // Left
    {0, SNAKE_SEGMENT_SIZE},   // Down
    {SNAKE_SEGMENT_SIZE, 0}    // Right
};

volatile int new_dx = 0;
volatile int new_dy = 0;
volatile bool input_received = false;
volatile bool snake_position_updated = false;
volatile bool self_ate = false;

void IRAM_ATTR handle_button_press(int buttonIndex) {
    int temp_dx = DIRECTION_VECTORS[buttonIndex][0];
    int temp_dy = DIRECTION_VECTORS[buttonIndex][1];
    
    if (snake.dx == temp_dx && snake.dy == temp_dy) {
        return;
    }

    if (snake_position_updated &&(snake.dx + temp_dx != 0 || snake.dy + temp_dy != 0)) {
        new_dx = temp_dx;
        new_dy = temp_dy;
        input_received = true;
    }
}

void IRAM_ATTR btnUpISR() { handle_button_press(0); }
void IRAM_ATTR btnLeftISR() { handle_button_press(1); }
void IRAM_ATTR btnDownISR() { handle_button_press(2); }
void IRAM_ATTR btnRightISR() { handle_button_press(3); }

void snake_input_task(void *pv) {
    while (current_state == STATE_SNAKE) {
        if (input_received && snake_position_updated) {
            xSemaphoreTake(snake_mutex, portMAX_DELAY);
            snake.dx = new_dx;
            snake.dy = new_dy;
            input_received = false;
            snake_position_updated = false;
            xSemaphoreGive(snake_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

void initialize_snake_game() {
    xSemaphoreTake(snake_mutex, portMAX_DELAY);

    // Initialize snake state
    snake = (SnakeGame){.segments = {START_POSITION},
                        .length = 1,
                        .dx = SNAKE_SEGMENT_SIZE,
                        .dy = 0,
                        .speed = INITIAL_SNAKE_SPEED,
                        .running = 1};

    // Calculate play area dimensions
    const uint16_t play_width = SCREEN_WIDTH - 2 * BORDER_SIZE - SNAKE_SEGMENT_SIZE;
    const uint16_t play_height = SCREEN_HEIGHT - 2 * BORDER_SIZE - SNAKE_SEGMENT_SIZE;

    // Generate initial food position
    snake.food.x = BORDER_SIZE + (random(0, play_width / SNAKE_SEGMENT_SIZE) * SNAKE_SEGMENT_SIZE);
    snake.food.y = BORDER_SIZE + (random(0, play_height / SNAKE_SEGMENT_SIZE) * SNAKE_SEGMENT_SIZE);

    // Draw initial game state
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, SCREEN_WIDTH, BORDER_SIZE, TFT_WHITE);   // Top border
    tft.fillRect(0, 0, BORDER_SIZE, SCREEN_HEIGHT, TFT_WHITE);  // Left border
    tft.fillRect(SCREEN_WIDTH - BORDER_SIZE, 0, BORDER_SIZE, SCREEN_HEIGHT,
                 TFT_WHITE);  // Right border
    tft.fillRect(0, SCREEN_HEIGHT - BORDER_SIZE, SCREEN_WIDTH, BORDER_SIZE,
                 TFT_WHITE);  // Bottom border
    tft.fillRect(snake.food.x, snake.food.y, SNAKE_SEGMENT_SIZE, SNAKE_SEGMENT_SIZE, TFT_GREEN);

    xSemaphoreGive(snake_mutex);
}

void update_snake_position() {
    // Move body segments
    for (int i = snake.length - 1; i > 0; i--) {
        snake.segments[i] = snake.segments[i - 1];
    }

    // Update head position
    snake.segments[0].x += snake.dx;
    snake.segments[0].y += snake.dy;

    snake_position_updated = true;
}

bool check_snake_collisions() {
    const Position head = snake.segments[0];

    // Wall collision check
    if (head.x < BORDER_SIZE || head.x >= SCREEN_WIDTH - BORDER_SIZE || head.y < BORDER_SIZE ||
        head.y >= SCREEN_HEIGHT - BORDER_SIZE) {
        return true;
    }

    // Self-collision check
    for (int i = 1; i < snake.length; i++) {
        if (head.x == snake.segments[i].x && head.y == snake.segments[i].y) {
            self_ate = true;
            return true;
        }
    }

    return false;
}

void handle_food_consumption() {
    if (snake.segments[0].x == snake.food.x && snake.segments[0].y == snake.food.y) {
        if (snake.length < 100) {
            snake.segments[snake.length] = snake.segments[snake.length - 1];
            snake.length++;
        }

        snake.speed = (snake.speed > 30) ? snake.speed - 2 : 30;

        // Generate new food position
        const uint16_t play_width = SCREEN_WIDTH - 2 * BORDER_SIZE - SNAKE_SEGMENT_SIZE;
        const uint16_t play_height = SCREEN_HEIGHT - 2 * BORDER_SIZE - SNAKE_SEGMENT_SIZE;

        snake.food.x =
            BORDER_SIZE + (random(0, play_width / SNAKE_SEGMENT_SIZE) * SNAKE_SEGMENT_SIZE);
        snake.food.y =
            BORDER_SIZE + (random(0, play_height / SNAKE_SEGMENT_SIZE) * SNAKE_SEGMENT_SIZE);

        tft.fillRect(snake.food.x, snake.food.y, SNAKE_SEGMENT_SIZE, SNAKE_SEGMENT_SIZE, TFT_GREEN);
    }
}

void snake_gameover() {
    detachInterrupt(digitalPinToInterrupt(BTN_UP));
    detachInterrupt(digitalPinToInterrupt(BTN_LEFT));
    detachInterrupt(digitalPinToInterrupt(BTN_DOWN));
    detachInterrupt(digitalPinToInterrupt(BTN_RIGHT));

    tft.fillScreen(TFT_BLACK);
    if (self_ate) {
        draw_centered_text("Fake Over!", 60, TFT_WHITE, 1);
    } else {
        draw_centered_text("Game Over!", 60, TFT_WHITE, 1);
    }

    char score[20];
    snprintf(score, sizeof(score), "Score: %d", snake.length - 1);
    draw_centered_text(score, 80, TFT_WHITE, 1);

    xSemaphoreGive(snake_mutex);
    vTaskDelay(pdMS_TO_TICKS(2000));

    current_state = STATE_MENU;

    show_menu();
}

void snake_task(void *pv) {
    initialize_snake_game();
    static int prev_length = 1;

    while (current_state == STATE_SNAKE) {
        xSemaphoreTake(snake_mutex, portMAX_DELAY);

        tft.setTextSize(1);
        tft.setCursor(0, 0);
        tft.print(snake_position_updated);

        if (!snake.running) {
            snake_gameover();
            break;
        }

        // Clear previous tail if not growing
        if (snake.length == prev_length) {
            const Position tail = snake.segments[snake.length - 1];
            tft.fillRect(tail.x, tail.y, SNAKE_SEGMENT_SIZE, SNAKE_SEGMENT_SIZE, TFT_BLACK);
        }
        prev_length = snake.length;

        update_snake_position();
        snake.running = !check_snake_collisions();
        handle_food_consumption();

        // Draw new head position
        const Position head = snake.segments[0];
        tft.fillRect(head.x, head.y, SNAKE_SEGMENT_SIZE, SNAKE_SEGMENT_SIZE, TFT_WHITE);

        xSemaphoreGive(snake_mutex);
        vTaskDelay(pdMS_TO_TICKS(snake.speed));
    }

    vTaskDelete(NULL);
}

void snake_init_mutex() { snake_mutex = xSemaphoreCreateMutex(); }

void snake_launch_tasks() {
    self_ate = false;

    attachInterrupt(digitalPinToInterrupt(BTN_UP), btnUpISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_LEFT), btnLeftISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_DOWN), btnDownISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_RIGHT), btnRightISR, FALLING);

    xTaskCreatePinnedToCore(snake_task, "Snake", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(snake_input_task, "SnakeInput", 2048, NULL, 3, NULL, 0);
}