#include "pong_game.h"

struct PongGame {
    Position player;
    Position ai;
    Position ball;
    int ball_dx;
    int ball_dy;
    int player_score;
    int ai_score;
    uint8_t running;
};

// Pong game constants
const int PADDLE_WIDTH = 4;
const int PADDLE_HEIGHT = 20;
const int BALL_SIZE = 4;
const int AI_PREDICTION_FRAMES = 6;
const int MAX_SCORE = 5;

PongGame pong;
SemaphoreHandle_t pong_mutex;
TaskHandle_t pong_task_handle = NULL;
TaskHandle_t pong_input_task_handle = NULL;

void initialize_pong_game() {
    xSemaphoreTake(pong_mutex, portMAX_DELAY);

    pong =
        (PongGame){.player = {BORDER_SIZE, (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2},
                   .ai = {SCREEN_WIDTH - BORDER_SIZE - PADDLE_WIDTH,
                          (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2},
                   .ball = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2},
                   .ball_dx = (rand() % 2) ? 2 : -2,
                   .ball_dy = (rand() % 3) - 1,
                   .player_score = 0,
                   .ai_score = 0,
                   .running = 1};

    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, SCREEN_WIDTH, BORDER_SIZE, TFT_WHITE);
    tft.fillRect(0, SCREEN_HEIGHT - BORDER_SIZE, SCREEN_WIDTH, BORDER_SIZE, TFT_WHITE);

    xSemaphoreGive(pong_mutex);
}

void pong_input_task(void *pv) {
    const int MOVE_SPEED = 3;

    while (current_state == STATE_PONG) {
        xSemaphoreTake(pong_mutex, portMAX_DELAY);

        if (!digitalRead(BTN_UP)) {
            pong.player.y =
                constrain(pong.player.y - MOVE_SPEED, BORDER_SIZE,
                          SCREEN_HEIGHT - BORDER_SIZE - PADDLE_HEIGHT);
        }
        if (!digitalRead(BTN_DOWN)) {
            pong.player.y =
                constrain(pong.player.y + MOVE_SPEED, BORDER_SIZE,
                          SCREEN_HEIGHT - BORDER_SIZE - PADDLE_HEIGHT);
        }

        xSemaphoreGive(pong_mutex);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void erase_previous_positions(int prev_player_y, int prev_ai_y, Position prev_ball) {
    tft.fillRect(pong.player.x, prev_player_y, PADDLE_WIDTH, PADDLE_HEIGHT,
                 TFT_BLACK);
    tft.fillRect(pong.ai.x, prev_ai_y, PADDLE_WIDTH, PADDLE_HEIGHT, TFT_BLACK);
    tft.fillRect(prev_ball.x, prev_ball.y, BALL_SIZE, BALL_SIZE, TFT_BLACK);
}

void update_ball_position() {
    pong.ball.x += pong.ball_dx;
    pong.ball.y += pong.ball_dy;
}

void handle_wall_collisions() {
    if (pong.ball.y <= BORDER_SIZE ||
        pong.ball.y >= SCREEN_HEIGHT - BORDER_SIZE - BALL_SIZE) {
        pong.ball_dy = -pong.ball_dy;
    }
}

void handle_paddle_collisions() {
    // Player paddle collision
    if (pong.ball.x <= BORDER_SIZE + PADDLE_WIDTH &&
        pong.ball.y + BALL_SIZE > pong.player.y &&
        pong.ball.y < pong.player.y + PADDLE_HEIGHT) {
        pong.ball_dx = abs(pong.ball_dx);
    }

    // AI paddle collision
    if (pong.ball.x + BALL_SIZE >= SCREEN_WIDTH - BORDER_SIZE - PADDLE_WIDTH &&
        pong.ball.y + BALL_SIZE > pong.ai.y &&
        pong.ball.y < pong.ai.y + PADDLE_HEIGHT) {
        pong.ball_dx = -abs(pong.ball_dx);
    }
}

void update_ai_paddle() {
    const int prediction_y =
        pong.ball.y + (pong.ball_dy * AI_PREDICTION_FRAMES);
    const int ai_center = pong.ai.y + PADDLE_HEIGHT / 2;
    const int target_y = prediction_y - PADDLE_HEIGHT / 2 + (rand() % 7 - 3);

    const int constrained_target = constrain(
        target_y, BORDER_SIZE, SCREEN_HEIGHT - BORDER_SIZE - PADDLE_HEIGHT);

    pong.ai.y += constrain((constrained_target - ai_center) / 2, -4, 4);
    pong.ai.y = constrain(pong.ai.y, BORDER_SIZE, SCREEN_HEIGHT - BORDER_SIZE - PADDLE_HEIGHT);
}

void reset_ball(bool player_scored) {
    pong.ball.x = SCREEN_WIDTH / 2;
    pong.ball.y =
        BORDER_SIZE + rand() % (SCREEN_HEIGHT - 2 * BORDER_SIZE - BALL_SIZE);

    if (player_scored) {
        pong.ball_dx = -abs(pong.ball_dx);
    } else {
        pong.ball_dx = abs(pong.ball_dx);
    }
}

void update_scores() {
    if (pong.ball.x < 0) {
        pong.ai_score++;
        reset_ball(false);
    }
    if (pong.ball.x > SCREEN_WIDTH) {
        pong.player_score++;
        reset_ball(true);
    }
}

void render_game_state() {
    tft.fillRect(pong.player.x, pong.player.y, PADDLE_WIDTH, PADDLE_HEIGHT, TFT_WHITE);
    tft.fillRect(pong.ai.x, pong.ai.y, PADDLE_WIDTH, PADDLE_HEIGHT, TFT_WHITE);
    tft.fillRect(pong.ball.x, pong.ball.y, BALL_SIZE, BALL_SIZE, TFT_WHITE);

    tft.setCursor(SCREEN_WIDTH / 4 - 8, BORDER_SIZE + 2);
    tft.print(pong.player_score);
    tft.setCursor(3 * SCREEN_WIDTH / 4 - 8, BORDER_SIZE + 2);
    tft.print(pong.ai_score);
}

void pong_gameover() {
    tft.fillScreen(TFT_BLACK);
    const char *result =
        pong.player_score > pong.ai_score ? "You Win!" : "Game Over!";
    draw_centered_text(result, 60, TFT_WHITE, 2);
    draw_centered_text("Press A", 100, TFT_WHITE, 1);

    vTaskDelay(pdMS_TO_TICKS(2000));
}

void pong_task(void *pv) {
    initialize_pong_game();
    static int prev_player_y = pong.player.y;
    static int prev_ai_y = pong.ai.y;
    static Position prev_ball = pong.ball;

    while (current_state == STATE_PONG) {
        xSemaphoreTake(pong_mutex, portMAX_DELAY);

        if (!pong.running) {
            xSemaphoreGive(pong_mutex);
            pong_gameover();
            vTaskSuspend(NULL);
            vTaskSuspend(pong_input_task_handle);
            break;
        }

        erase_previous_positions(prev_player_y, prev_ai_y, prev_ball);
        update_ball_position();
        handle_wall_collisions();
        handle_paddle_collisions();
        update_ai_paddle();
        update_scores();
        render_game_state();

        if (pong.player_score >= MAX_SCORE || pong.ai_score >= MAX_SCORE) {
            pong.running = 0;
        }

        prev_player_y = pong.player.y;
        prev_ai_y = pong.ai.y;
        prev_ball = pong.ball;

        xSemaphoreGive(pong_mutex);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void pong_init_mutex() { pong_mutex = xSemaphoreCreateMutex(); }

void pong_launch_tasks() {
    xTaskCreatePinnedToCore(pong_task, "Pong", 4096, NULL, 2, &pong_task_handle, 1);
    xTaskCreatePinnedToCore(pong_input_task, "PongInput", 2048, NULL, 3, &pong_input_task_handle, 0);
}

void pong_exit() {
    if (pong_task_handle != NULL) {
        vTaskDelete(pong_task_handle);
        pong_task_handle = NULL;
    }
    
    if (pong_input_task_handle != NULL) {
        vTaskDelete(pong_input_task_handle);
        pong_input_task_handle = NULL;
    }
}