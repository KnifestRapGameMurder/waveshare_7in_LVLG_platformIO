#include "accuracy_trainer.h"
#include "hardware_abstraction.h"
#include "app_screens.h"
#include <Arduino.h>
#include <lvgl.h>
#include "fonts.h"

// === Game Constants ===
const int GET_READY_DURATION = 3000;       // 3 seconds
const int ACCURACY_TIMEOUT = 5000;         // 5 seconds to hit the target
const int MAX_ACCURACY_MISSES = 3;         // Max misses before game over
const int TOTAL_ACCURACY_ROUNDS = 10;      // Rounds per game
const int FEEDBACK_DURATION = 260;         // ms for success/fail feedback effect
const int GAME_OVER_MSG_DURATION = 1000;   // ms to show "Game Over"
const int RESULTS_DISPLAY_DURATION = 5000; // ms to show results

// Speed constants
const int ACCURACY_CHASER_SPEED_EASY = 200;
const int ACCURACY_CHASER_SPEED_MEDIUM = 150;
const int ACCURACY_CHASER_SPEED_HARD = 100;

// === Game State Variables ===
static AccuracyTrainerState current_trainer_state = AT_STATE_IDLE;
static AccuracyDifficulty current_difficulty = ACCURACY_EASY;
static unsigned long state_timer = 0;
static int target_led = 0;
static int prev_target_led = -1;
static unsigned long round_start_time = 0;
static int correct_presses = 0;
static int total_rounds = 0;
static int misses = 0;
static bool feedback_success = false;
static uint16_t last_button_state = 0xFFFF; // Start with all buttons released

// === Chaser (Easy Mode) Variables ===
static int chaser_position = 0;
static bool chaser_direction = true; // true = right, false = left
static unsigned long last_chaser_move = 0;

// === UI Elements ===
static lv_obj_t *accuracy_screen = NULL;
static lv_obj_t *hud_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_obj_t *results_label = NULL;
static lv_obj_t *play_again_btn = NULL;
static lv_obj_t *exit_btn = NULL;
static lv_obj_t *back_btn = NULL;

// === Forward Declarations ===
static void update_hud();
static void show_feedback_effect();
static void check_button_presses();
static void move_chaser_easy();
static void display_results();
static void create_game_over_menu();
static void game_over_menu_event_handler(lv_event_t *e);
static void back_to_menu_event_handler(lv_event_t *e);

void create_accuracy_trainer_screen()
{
    // Clean the screen
    lv_obj_clean(lv_scr_act());

    // Create main screen container
    accuracy_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(accuracy_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(accuracy_screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(accuracy_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Create HUD label
    hud_label = lv_label_create(accuracy_screen);
    lv_obj_set_style_text_font(hud_label, Font2, 0);
    lv_obj_set_style_text_color(hud_label, lv_color_white(), 0);
    lv_obj_align(hud_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create info label
    info_label = lv_label_create(accuracy_screen);
    lv_obj_set_style_text_font(info_label, Font2, 0);
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);

    // Create results label (hidden initially)
    results_label = lv_label_create(accuracy_screen);
    lv_obj_set_style_text_font(results_label, Font2, 0);
    lv_obj_set_style_text_color(results_label, lv_color_white(), 0);
    lv_obj_align(results_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Create back button
    back_btn = lv_btn_create(accuracy_screen);
    lv_obj_set_size(back_btn, 200, 80);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x666666), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(back_btn, lv_color_white(), 0);
    lv_obj_set_style_border_width(back_btn, 2, 0);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_btn, back_to_menu_event_handler, LV_EVENT_CLICKED, NULL);

    // Initialize game state
    set_accuracy_trainer_state(AT_STATE_GET_READY);
}

void set_accuracy_trainer_state(AccuracyTrainerState newState)
{
    current_trainer_state = newState;
    state_timer = lv_tick_get();

    switch (current_trainer_state)
    {
    case AT_STATE_IDLE:
        strip_Clear();
        break;

    case AT_STATE_GET_READY:
        lv_label_set_text(info_label, "Приготуйся!");
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);
        correct_presses = 0;
        total_rounds = 0;
        misses = 0;
        strip_Clear();

        // Set chaser speed based on difficulty
        // (Speed logic can be added here if needed)

        // Initialize chaser
        chaser_position = 0;
        chaser_direction = true;
        last_chaser_move = lv_tick_get();
        break;

    case AT_STATE_SHOW_TARGET:
    {
        char round_text[32];
        snprintf(round_text, sizeof(round_text), "Раунд %d", total_rounds + 1);
        lv_label_set_text(hud_label, round_text);
        update_hud();

        if (current_difficulty == ACCURACY_EASY)
        {
            lv_label_set_text(info_label, "Спіймай зв'язку!");
            // Set static blue target
            int tries = 0;
            do
            {
                target_led = random(NUM_LEDS);
                tries++;
            } while (target_led == prev_target_led && tries < 5);
            prev_target_led = target_led;

            // Initialize chaser
            chaser_position = 0;
            chaser_direction = true;

            // Show blue target
            strip_Clear();
            strip_SetPixelColor(target_led, RgbColor(0, 0, 255));
            strip_Show();

            last_chaser_move = lv_tick_get();
            round_start_time = lv_tick_get();
        }
        else
        {
            lv_label_set_text(info_label, "Спіймай мету!");
            chaser_position = random(NUM_LEDS);
            chaser_direction = (esp_random() & 1);
            strip_Clear();
            strip_SetPixelColor(chaser_position, RgbColor(255, 255, 0)); // Yellow for target
            strip_Show();
            last_chaser_move = lv_tick_get();
            round_start_time = lv_tick_get();
        }
        break;
    }

    case AT_STATE_WAIT_FOR_PRESS:
        round_start_time = lv_tick_get();
        break;

    case AT_STATE_FEEDBACK:
        // Feedback is handled in run_accuracy_trainer
        break;

    case AT_STATE_GAME_OVER:
        lv_label_set_text(info_label, "Гру завершено!");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        strip_Clear();
        break;

    case AT_STATE_SHOW_RESULTS:
        display_results();
        break;

    case AT_STATE_GAME_OVER_MENU:
        create_game_over_menu();
        break;

    default:
        break;
    }
}

static void update_hud()
{
    if (total_rounds > 0)
    {
        float accuracy = (100.0f * correct_presses) / total_rounds;
        char hud_text[64];
        snprintf(hud_text, sizeof(hud_text), "Влучність %.1f%%  Раундів: %d", accuracy, total_rounds);
        lv_label_set_text(hud_label, hud_text);
    }
    else
    {
        lv_label_set_text(hud_label, "Влучність 0.0%  Раундів: 0");
    }
}

static void show_feedback_effect()
{
    // This will be called from run_accuracy_trainer for feedback animation
}

void check_button_presses()
{
    uint16_t current_button_state = expanderRead();

    for (int i = 0; i < NUM_LEDS; i++)
    {
        bool was_pressed = !(last_button_state & (1 << i));
        bool is_pressed = !(current_button_state & (1 << i));

        if (!was_pressed && is_pressed) // Button just pressed
        {
            total_rounds++;

            if (current_difficulty == ACCURACY_EASY)
            {
                if (i == target_led && chaser_position == target_led)
                {
                    correct_presses++;
                    Serial.printf("Acc(Easy): Perfect hit! Button: %d\n", i);
                    feedback_success = true;
                }
                else
                {
                    misses++;
                    Serial.printf("Acc(Easy): Miss! Button: %d, Target: %d, Chaser: %d\n", i, target_led, chaser_position);
                    feedback_success = false;
                }
            }
            else
            {
                if (i == chaser_position)
                {
                    correct_presses++;
                    Serial.printf("Acc: Hit! Button: %d\n", i);
                    feedback_success = true;
                }
                else
                {
                    misses++;
                    Serial.printf("Acc: Miss! Button: %d, Target: %d\n", i, chaser_position);
                    feedback_success = false;
                }
            }

            set_accuracy_trainer_state(AT_STATE_FEEDBACK);
            break;
        }
    }

    last_button_state = current_button_state;

    // Check for timeout
    if (lv_tick_get() - round_start_time > ACCURACY_TIMEOUT)
    {
        Serial.println("Acc: Timeout");
        total_rounds++;
        misses++;

        if (misses >= MAX_ACCURACY_MISSES)
        {
            set_accuracy_trainer_state(AT_STATE_GAME_OVER);
        }
        else
        {
            set_accuracy_trainer_state(AT_STATE_SHOW_TARGET);
        }
    }
}

static void move_chaser_easy()
{
    if (current_trainer_state != AT_STATE_SHOW_TARGET && current_trainer_state != AT_STATE_WAIT_FOR_PRESS)
        return;

    int base_speed = ACCURACY_CHASER_SPEED_EASY;
    int min_speed = 80;
    int dynamic_speed = base_speed - correct_presses * 15;
    if (dynamic_speed < min_speed)
        dynamic_speed = min_speed;

    unsigned long now = lv_tick_get();
    if (now - last_chaser_move >= (unsigned long)dynamic_speed)
    {
        strip_Clear();

        // Show blue target
        strip_SetPixelColor(target_led, RgbColor(0, 0, 255));

        // Show chaser (3 LEDs)
        for (int i = -1; i <= 1; i++)
        {
            int pos = chaser_position + i;
            if (pos >= 0 && pos < NUM_LEDS)
            {
                uint8_t brightness = (i == 0) ? 255 : 127;                     // Center brighter
                strip_SetPixelColor(pos, RgbColor(brightness, brightness, 0)); // Yellow
            }
        }

        strip_Show();

        // Move chaser
        if (chaser_direction)
        {
            chaser_position++;
            if (chaser_position >= NUM_LEDS - 1)
            {
                chaser_position = NUM_LEDS - 1;
                chaser_direction = false;
            }
        }
        else
        {
            chaser_position--;
            if (chaser_position <= 0)
            {
                chaser_position = 0;
                chaser_direction = true;
            }
        }

        last_chaser_move = now;
    }
}

void run_accuracy_trainer()
{
    switch (current_trainer_state)
    {
    case AT_STATE_GET_READY:
        if (lv_tick_get() - state_timer > GET_READY_DURATION)
        {
            set_accuracy_trainer_state(AT_STATE_SHOW_TARGET);
        }
        break;

    case AT_STATE_SHOW_TARGET:
        if (current_difficulty == ACCURACY_EASY)
        {
            if (lv_tick_get() - state_timer > 50)
            {
                set_accuracy_trainer_state(AT_STATE_WAIT_FOR_PRESS);
            }
            move_chaser_easy();
            check_button_presses();
        }
        else
        {
            // For medium/hard, implement chaser movement if needed
            check_button_presses();
        }
        break;

    case AT_STATE_WAIT_FOR_PRESS:
        if (current_difficulty == ACCURACY_EASY)
        {
            move_chaser_easy();
        }
        check_button_presses();
        break;

    case AT_STATE_FEEDBACK:
    {
        unsigned long elapsed = lv_tick_get() - state_timer;
        float phase = (float)elapsed / FEEDBACK_DURATION;
        if (phase > 1.0f)
            phase = 1.0f;
        float intensity = (phase < 0.5f) ? (phase * 2.0f) : (1.0f - (phase - 0.5f) * 2.0f);
        uint8_t brightness = (uint8_t)(255 * intensity);
        RgbColor color = feedback_success ? RgbColor(0, brightness, 0) : RgbColor(brightness, 0, 0);

        for (int i = 0; i < NUM_LEDS; i++)
        {
            strip_SetPixelColor(i, color);
        }
        strip_Show();

        if (elapsed > FEEDBACK_DURATION)
        {
            strip_Clear();
            strip_Show();

            if (feedback_success)
            {
                if (total_rounds >= TOTAL_ACCURACY_ROUNDS || misses >= MAX_ACCURACY_MISSES)
                {
                    set_accuracy_trainer_state(AT_STATE_GAME_OVER);
                }
                else
                {
                    set_accuracy_trainer_state(AT_STATE_SHOW_TARGET);
                }
            }
            else
            {
                if (misses >= MAX_ACCURACY_MISSES)
                {
                    set_accuracy_trainer_state(AT_STATE_GAME_OVER);
                }
                else
                {
                    set_accuracy_trainer_state(AT_STATE_SHOW_TARGET);
                }
            }
        }
        break;
    }

    case AT_STATE_GAME_OVER:
        Serial.printf("AT_STATE_GAME_OVER: elapsed %lu ms\n", lv_tick_get() - state_timer);
        if (lv_tick_get() - state_timer > GAME_OVER_MSG_DURATION)
        {
            Serial.println("Transitioning to AT_STATE_SHOW_RESULTS");
            set_accuracy_trainer_state(AT_STATE_SHOW_RESULTS);
        }
        break;

    case AT_STATE_SHOW_RESULTS:
        if (lv_tick_get() - state_timer > RESULTS_DISPLAY_DURATION)
        {
            set_accuracy_trainer_state(AT_STATE_GAME_OVER_MENU);
        }
        break;

    case AT_STATE_GAME_OVER_MENU:
        // Wait for user input on buttons
        break;

    default:
        break;
    }
}

static void display_results()
{
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    float accuracy = (total_rounds > 0) ? ((float)correct_presses / total_rounds * 100.0f) : 0.0f;
    char results_text[256];
    snprintf(results_text, sizeof(results_text),
             "Результати Влучності:\n\nПравильних: %d\nВсього раундів: %d\nВлучність: %.1f%%",
             correct_presses, total_rounds, accuracy);
    lv_label_set_text(results_label, results_text);
}

static void create_game_over_menu()
{
    // Hide other labels
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Create play again button
    play_again_btn = lv_btn_create(accuracy_screen);
    lv_obj_set_size(play_again_btn, 300, 80);
    lv_obj_align(play_again_btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(play_again_btn, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_bg_color(play_again_btn, lv_color_hex(0x00AA00), LV_STATE_PRESSED);

    lv_obj_t *play_label = lv_label_create(play_again_btn);
    lv_label_set_text(play_label, "Грати Знову");
    lv_obj_set_style_text_font(play_label, Font2, 0);
    lv_obj_center(play_label);

    lv_obj_add_event_cb(play_again_btn, game_over_menu_event_handler, LV_EVENT_CLICKED, (void *)0);

    // Create exit button
    exit_btn = lv_btn_create(accuracy_screen);
    lv_obj_set_size(exit_btn, 300, 80);
    lv_obj_align(exit_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xAA0000), LV_STATE_PRESSED);

    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "Вихід");
    lv_obj_set_style_text_font(exit_label, Font2, 0);
    lv_obj_center(exit_label);

    lv_obj_add_event_cb(exit_btn, game_over_menu_event_handler, LV_EVENT_CLICKED, (void *)1);
    lv_obj_move_foreground(back_btn);
}

static void game_over_menu_event_handler(lv_event_t *e)
{
    int action = (int)(intptr_t)lv_event_get_user_data(e);

    if (action == 0) // Play again
    {
        Serial.println("Acc Menu: Play Again");
        set_accuracy_trainer_state(AT_STATE_GET_READY);
    }
    else if (action == 1) // Exit
    {
        Serial.println("Acc Menu: Exit");
        last_interaction_time = lv_tick_get(); // Add this
        current_state = STATE_MAIN_MENU;
        set_accuracy_trainer_state(AT_STATE_IDLE);
        create_main_menu();
    }
}

static void back_to_menu_event_handler(lv_event_t *e)
{
    Serial.println("Back button pressed in accuracy trainer");

    if (current_trainer_state == AT_STATE_GAME_OVER || current_trainer_state == AT_STATE_SHOW_RESULTS)
    {
        // Skip to menu directly
        current_state = STATE_MAIN_MENU;
        set_accuracy_trainer_state(AT_STATE_IDLE);
        create_main_menu();
        return;
    }

    last_interaction_time = lv_tick_get(); // Add this
    current_state = STATE_MAIN_MENU;
    set_accuracy_trainer_state(AT_STATE_IDLE);
    create_main_menu();
}

// Difficulty setters
void set_accuracy_easy_mode() { current_difficulty = ACCURACY_EASY; }
void set_accuracy_medium_mode() { current_difficulty = ACCURACY_MEDIUM; }
void set_accuracy_hard_mode() { current_difficulty = ACCURACY_HARD; }