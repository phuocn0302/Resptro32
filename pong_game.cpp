#include "pong_game.h"

enum Difficulty { EASY, NORMAL, HARD, IMPOSSIBLE };
const char* DIFFICULTY_NAMES[] = {"Easy", "Normal", "Hard", "Impossible"};
const int SCORE_LIMITS[] = {5, 10, 15, 20};
const int NUM_DIFFICULTIES = 4;
const int NUM_SCORE_LIMITS = 4;

struct PongGame {
    Position player;
    Position ai;
    Position ball;
    float ball_dx;
    float ball_dy;
    int player_score;
    int ai_score;
    uint8_t running;
    Difficulty difficulty;
    int score_limit;
};

const int PADDLE_WIDTH = 4;
const int PADDLE_HEIGHT = 20;
const int BALL_SIZE = 4;
const float BALL_SPEED_INCREASE = 0.5;  // How much to increase speed on each hit
const float MAX_BALL_SPEED_X = 5.0;     // Maximum horizontal ball speed
const float MAX_BALL_SPEED_Y = 3.0;     // Maximum vertical ball speed
const int MAX_SCORE = 20;

PongGame pong;
SemaphoreHandle_t pong_mutex;
TaskHandle_t pong_task_handle = NULL;
TaskHandle_t pong_input_task_handle = NULL;

void show_pong_settings() {
    static int selected_option = 0;  
    static int difficulty_idx = 1;   
    static int score_limit_idx = 0;  
    bool settings_done = false;

    tft.fillScreen(TFT_BLACK);

    while (!settings_done && current_state == STATE_PONG) {

        draw_centered_text("Pong Settings", 20, TFT_WHITE, 1);

        tft.setTextSize(1);
        if (selected_option == 0) {
            tft.setTextColor(TFT_GREEN);
            tft.setCursor(10, 50);
            tft.print(">");
        }
        draw_centered_text("Difficulty:", 50, TFT_BLUE, 1);
        draw_centered_text(DIFFICULTY_NAMES[difficulty_idx], 65, TFT_YELLOW, 1);

        if (selected_option == 1) {
            tft.setTextColor(TFT_GREEN);
            tft.setCursor(10, 90);
            tft.print(">");
        }
        draw_centered_text("Score Limit:", 90, TFT_BLUE, 1);
        char score_text[3];
        sprintf(score_text, "%d", SCORE_LIMITS[score_limit_idx]);
        draw_centered_text(score_text, 105, TFT_YELLOW, 1);

        draw_centered_text("UP/DOWN: Select", 130, TFT_CYAN, 1);
        draw_centered_text("LEFT/RIGHT: Change", 140, TFT_CYAN, 1);
        draw_centered_text("Press B to start", 150, TFT_GREEN, 1);

        if (!digitalRead(BTN_UP)) {
            int old_option = selected_option;
            selected_option = 0;

            tft.setTextColor(TFT_BLACK);
            tft.setCursor(10, old_option == 0 ? 50 : 90);
            tft.print(">");
            while (!digitalRead(BTN_UP)) { delay(10); }
            delay(50);
        }
        if (!digitalRead(BTN_DOWN)) {
            int old_option = selected_option;
            selected_option = 1;

            tft.setTextColor(TFT_BLACK);
            tft.setCursor(10, old_option == 0 ? 50 : 90);
            tft.print(">");
            while (!digitalRead(BTN_DOWN)) { delay(10); }
            delay(50);
        }
        if (!digitalRead(BTN_LEFT)) {
            if (selected_option == 0) {
                difficulty_idx = (difficulty_idx - 1 + NUM_DIFFICULTIES) % NUM_DIFFICULTIES;

                tft.fillRect(0, 65, SCREEN_WIDTH, 10, TFT_BLACK);
            } else {
                score_limit_idx = (score_limit_idx - 1 + NUM_SCORE_LIMITS) % NUM_SCORE_LIMITS;

                tft.fillRect(0, 105, SCREEN_WIDTH, 10, TFT_BLACK);
            }
            while (!digitalRead(BTN_LEFT)) { delay(10); }
            delay(50);
        }
        if (!digitalRead(BTN_RIGHT)) {
            if (selected_option == 0) {
                difficulty_idx = (difficulty_idx + 1) % NUM_DIFFICULTIES;

                tft.fillRect(0, 65, SCREEN_WIDTH, 10, TFT_BLACK);
            } else {
                score_limit_idx = (score_limit_idx + 1) % NUM_SCORE_LIMITS;

                tft.fillRect(0, 105, SCREEN_WIDTH, 10, TFT_BLACK);
            }
            while (!digitalRead(BTN_RIGHT)) { delay(10); }
            delay(50);
        }
        if (!digitalRead(BTN_B)) {
            settings_done = true;
            while (!digitalRead(BTN_B)) { delay(10); }
            delay(50);
        }

        delay(10);
    }

    pong.difficulty = static_cast<Difficulty>(difficulty_idx);
    pong.score_limit = SCORE_LIMITS[score_limit_idx];
}

void initialize_pong_game() {
    show_pong_settings();  
    
    xSemaphoreTake(pong_mutex, portMAX_DELAY);

    float base_speed = 2.0f;
    if (pong.difficulty == IMPOSSIBLE) {
        base_speed = 3.0f;
    } else if (pong.difficulty == HARD) {
        base_speed = 2.5f;
    }

    pong = (PongGame){
        .player = {BORDER_SIZE, (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2},
        .ai = {SCREEN_WIDTH - BORDER_SIZE - PADDLE_WIDTH,
               (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2},
        .ball = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2},
        .ball_dx = (rand() % 2) ? base_speed : -base_speed,
        .ball_dy = ((float)(rand() % 3) - 1.0f) * 0.6f,
        .player_score = 0,
        .ai_score = 0,
        .running = 1,
        .difficulty = pong.difficulty,
        .score_limit = pong.score_limit 
    };

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
    pong.ball.x += round(pong.ball_dx);
    pong.ball.y += round(pong.ball_dy);
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
        
        // Increase speed on hit
        pong.ball_dx = min(abs(pong.ball_dx) + BALL_SPEED_INCREASE, MAX_BALL_SPEED_X);
        
        // Add spin effect based on where the paddle is hit
        int hit_pos = (pong.ball.y + BALL_SIZE/2) - pong.player.y;
        float relative_pos = (float)hit_pos / PADDLE_HEIGHT;
        pong.ball_dy += (relative_pos - 0.5) * 2;  // -1.0 to +1.0 added to current speed
        
        // Cap vertical speed
        if (pong.ball_dy > MAX_BALL_SPEED_Y) pong.ball_dy = MAX_BALL_SPEED_Y;
        if (pong.ball_dy < -MAX_BALL_SPEED_Y) pong.ball_dy = -MAX_BALL_SPEED_Y;
    }

    // AI paddle collision
    if (pong.ball.x + BALL_SIZE >= SCREEN_WIDTH - BORDER_SIZE - PADDLE_WIDTH &&
        pong.ball.y + BALL_SIZE > pong.ai.y &&
        pong.ball.y < pong.ai.y + PADDLE_HEIGHT) {
        
        // Increase speed on hit
        pong.ball_dx = -min(abs(pong.ball_dx) + BALL_SPEED_INCREASE, MAX_BALL_SPEED_X);
        
        // AI spin effects - different behavior based on difficulty
        int hit_pos = (pong.ball.y + BALL_SIZE/2) - pong.ai.y;
        float relative_pos = (float)hit_pos / PADDLE_HEIGHT;
        
        if (pong.difficulty == IMPOSSIBLE) {
            // AI aims away from player paddle
            if (pong.player.y > SCREEN_HEIGHT/2) {
                pong.ball_dy = -2.0f - (rand() % 200) / 100.0f;  // Aim upward
            } else {
                pong.ball_dy = 2.0f + (rand() % 200) / 100.0f;  // Aim downward
            }
        } else {
            // Normal spin based on hit position
            pong.ball_dy += (relative_pos - 0.5) * 2;
        }
        
        // Cap vertical speed
        if (pong.ball_dy > MAX_BALL_SPEED_Y) pong.ball_dy = MAX_BALL_SPEED_Y;
        if (pong.ball_dy < -MAX_BALL_SPEED_Y) pong.ball_dy = -MAX_BALL_SPEED_Y;
    }
}

void update_ai_paddle() {
    // Calculate prediction frames based on difficulty and ball speed
    float speed_factor = min(6.0f / abs(pong.ball_dx), 1.0f);  // Adjust prediction based on ball speed
    
    const int base_prediction_frames = pong.difficulty == EASY ? 1 : 
                                      (pong.difficulty == NORMAL ? 3 : 
                                      (pong.difficulty == HARD ? 8 : 15));
                                      
    const int prediction_frames = round(base_prediction_frames * speed_factor);
    
    const int max_speed = pong.difficulty == EASY ? 2 : 
                         (pong.difficulty == NORMAL ? 4 : 
                         (pong.difficulty == HARD ? 6 : 8));
    
    // For IMPOSSIBLE difficulty: speed up ball occasionally during gameplay
    if (pong.difficulty == IMPOSSIBLE && abs(pong.ball_dx) < MAX_BALL_SPEED_X && 
        (pong.ball.x == SCREEN_WIDTH / 2 || rand() % 100 < 2)) {
        // Small chance (2%) to increase ball speed during play
        pong.ball_dx = (pong.ball_dx > 0) ? 
            min(pong.ball_dx + 0.2f, MAX_BALL_SPEED_X) : 
            max(pong.ball_dx - 0.2f, -MAX_BALL_SPEED_X);
    }
    
    // Calculate predicted ball position
    const int prediction_y = pong.ball.y + round(pong.ball_dy * prediction_frames);
    const int ai_center = pong.ai.y + PADDLE_HEIGHT / 2;
    
    // Add randomness based on difficulty
    int randomOffset = 0;
    if (pong.difficulty == EASY) randomOffset = rand() % 11 - 5;      // -5 to +5
    else if (pong.difficulty == NORMAL) randomOffset = rand() % 7 - 3; // -3 to +3
    else if (pong.difficulty == HARD) randomOffset = 0;               // Perfect
    else if (pong.difficulty == IMPOSSIBLE) {
        // AI will always predict perfectly and aim for center hit
        randomOffset = -1; // Slight advantage to hit ball toward center
    }
    
    const int target_y = prediction_y - PADDLE_HEIGHT / 2 + randomOffset;

    const int constrained_target = constrain(
        target_y, BORDER_SIZE, SCREEN_HEIGHT - BORDER_SIZE - PADDLE_HEIGHT);

    // For IMPOSSIBLE, make the AI movement even smoother
    int divider = (pong.difficulty == IMPOSSIBLE) ? 1 : 2;
    pong.ai.y += constrain((constrained_target - ai_center) / divider, -max_speed, max_speed);
    pong.ai.y = constrain(pong.ai.y, BORDER_SIZE, 
                         SCREEN_HEIGHT - BORDER_SIZE - PADDLE_HEIGHT);
    
    // For IMPOSSIBLE, add perfect interception capability
    if (pong.difficulty == IMPOSSIBLE && pong.ball_dx > 0 && 
        pong.ball.x > SCREEN_WIDTH * 3/4) {
        // When ball is moving toward AI and past 3/4 of screen, AI will directly move to intercept
        pong.ai.y = constrain(pong.ball.y - PADDLE_HEIGHT/2, 
                          BORDER_SIZE, 
                          SCREEN_HEIGHT - BORDER_SIZE - PADDLE_HEIGHT);
    }
}

void reset_ball(bool player_scored) {
    pong.ball.x = SCREEN_WIDTH / 2;
    pong.ball.y =
        BORDER_SIZE + rand() % (SCREEN_HEIGHT - 2 * BORDER_SIZE - BALL_SIZE);

    // Determine base speed based on difficulty
    float base_speed = 2.0f;
    if (pong.difficulty == IMPOSSIBLE) {
        base_speed = 3.0f;
    } else if (pong.difficulty == HARD) {
        base_speed = 2.5f;
    }

    if (player_scored) {
        pong.ball_dx = -base_speed;
    } else {
        pong.ball_dx = base_speed;
    }
    
    // Set vertical speed based on difficulty
    if (pong.difficulty == IMPOSSIBLE) {
        pong.ball_dy = ((float)(rand() % 5) - 2.0f) * 0.8f;  // More vertical movement
    } else {
        pong.ball_dy = ((float)(rand() % 3) - 1.0f) * 0.6f;  // Standard vertical movement
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
        
        // Render game elements
        tft.fillRect(pong.player.x, pong.player.y, PADDLE_WIDTH, PADDLE_HEIGHT, TFT_WHITE);
        tft.fillRect(pong.ai.x, pong.ai.y, PADDLE_WIDTH, PADDLE_HEIGHT, TFT_WHITE);
        tft.fillRect(pong.ball.x, pong.ball.y, BALL_SIZE, BALL_SIZE, TFT_WHITE);
        
        // Always render score on top
        tft.setTextColor(TFT_WHITE);
        tft.fillRect(SCREEN_WIDTH/4 - 10, BORDER_SIZE + 2, 20, 10, TFT_BLACK); // Clear score area
        tft.fillRect(3*SCREEN_WIDTH/4 - 10, BORDER_SIZE + 2, 20, 10, TFT_BLACK);
        tft.setCursor(SCREEN_WIDTH/4 - 8, BORDER_SIZE + 2);
        tft.print(pong.player_score);
        tft.setCursor(3*SCREEN_WIDTH/4 - 8, BORDER_SIZE + 2);
        tft.print(pong.ai_score);

        if (pong.player_score >= pong.score_limit || pong.ai_score >= pong.score_limit) {
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
    initialize_pong_game();
    xTaskCreate(pong_task, "PongTask", 4096, NULL, 1, &pong_task_handle);
    xTaskCreate(pong_input_task, "PongInput", 4096, NULL, 1, &pong_input_task_handle);
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
