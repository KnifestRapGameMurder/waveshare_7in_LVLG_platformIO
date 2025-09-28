#include "coordination_trainer.h"
#include "hardware_abstraction.h"
#include "app_screens.h"
#include <Arduino.h>
#include <lvgl.h>
#include "fonts.h"

// === Game Constants ===
const int GET_READY_DURATION = 3000;                // 3 seconds
const int COORDINATION_TIMEOUT = 10000;             // 10 seconds to complete level
const int ROUND_COMPLETE_DURATION = 1500;           // 1.5 seconds to show success
const int GAME_OVER_MESSAGE_DURATION = 2000;        // 2 seconds to show game over
const int SURVIVAL_RESULTS_DISPLAY_DURATION = 5000; // 5 seconds for results

// Coordination specific constants
const int COORDINATION_EASY_START_LEDS = 2;
const int COORDINATION_EASY_MAX_LEDS = 8;
const int COORDINATION_HARD_START_LEDS = 3;
const int COORDINATION_HARD_MAX_LEDS = 16;
const int COORDINATION_INITIAL_SHOW_TIME = 2000; // 2 seconds
const int COORDINATION_MIN_SHOW_TIME = 500;      // 0.5 seconds
const int COORDINATION_TIME_DECREASE_STEP = 200; // Decrease by 200ms per level

// === Game State Variables ===
static CoordinationTrainerState current_trainer_state = CT_STATE_IDLE;
static CoordinationSubmenuState current_submenu_state = CS_SUBMENU_IDLE;
static unsigned long state_timer = 0;
static int current_level = 1;
static int targets_to_press[NUM_LEDS]; // Array of targets for current level
static int targets_pressed[NUM_LEDS];  // Array of pressed buttons
static int correct_presses_in_level = 0;
static unsigned long target_show_duration = 2000; // Initial show duration
static unsigned long round_start_time = 0;
static int correct_coordination_presses = 0;
static int total_coordination_rounds = 0;
static uint16_t last_button_state = 0xFFFF; // Start with all buttons released

// === UI Elements ===
static lv_obj_t *coordination_screen = NULL;
static lv_obj_t *level_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_obj_t *results_label = NULL;
static lv_obj_t *play_again_btn = NULL;
static lv_obj_t *exit_btn = NULL;

// === Forward Declarations ===
static void update_level_display();
static void check_button_presses_coordination();
static void display_results();
static void create_game_over_menu();
static void game_over_menu_event_handler(lv_event_t *e);
static void back_to_menu_event_handler(lv_event_t *e);

void create_coordination_trainer_screen()
{
    // Clean the screen
    lv_obj_clean(lv_scr_act());

    // Create main screen container
    coordination_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(coordination_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(coordination_screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(coordination_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Create level label
    level_label = lv_label_create(coordination_screen);
    lv_obj_set_style_text_font(level_label, Font2, 0);
    lv_obj_set_style_text_color(level_label, lv_color_white(), 0);
    lv_obj_align(level_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create info label
    info_label = lv_label_create(coordination_screen);
    lv_obj_set_style_text_font(info_label, Font2, 0);
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);

    // Create results label (hidden initially)
    results_label = lv_label_create(coordination_screen);
    lv_obj_set_style_text_font(results_label, Font2, 0);
    lv_obj_set_style_text_color(results_label, lv_color_white(), 0);
    lv_obj_align(results_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Create back button
    lv_obj_t *back_btn = lv_btn_create(coordination_screen);
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
    set_coordination_trainer_state(CT_STATE_GET_READY);
}

void set_coordination_trainer_state(CoordinationTrainerState newState)
{
    current_trainer_state = newState;
    state_timer = lv_tick_get();

    switch (current_trainer_state)
    {
    case CT_STATE_IDLE:
        strip_Clear();
        break;

    case CT_STATE_GET_READY:
        lv_label_set_text(info_label, "Приготуйся!");
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);
        correct_coordination_presses = 0;
        total_coordination_rounds = 0;
        correct_presses_in_level = 0;
        target_show_duration = COORDINATION_INITIAL_SHOW_TIME;

        // Set initial level based on mode
        if (current_submenu_state == CS_EASY_MODE)
        {
            current_level = COORDINATION_EASY_START_LEDS;
        }
        else if (current_submenu_state == CS_HARD_MODE)
        {
            current_level = COORDINATION_HARD_START_LEDS;
        }

        // Initialize arrays
        for (int i = 0; i < NUM_LEDS; i++)
        {
            targets_to_press[i] = 0;
            targets_pressed[i] = 0;
        }

        strip_Clear();
        update_level_display();
        break;

    case CT_STATE_SHOW_TARGET:
    {
        lv_label_set_text(info_label, "Запам'ятай кнопки!");
        update_level_display();

        // Clear arrays for new level
        for (int i = 0; i < NUM_LEDS; i++)
        {
            targets_to_press[i] = 0;
            targets_pressed[i] = 0;
        }
        correct_presses_in_level = 0;

        // Generate targets for current level
        int targets_generated = 0;
        while (targets_generated < current_level)
        {
            int rand_led = random(NUM_LEDS);
            if (targets_to_press[rand_led] == 0)
            {
                targets_to_press[rand_led] = 1;
                targets_generated++;
            }
        }

        // Show all targets simultaneously
        strip_Clear();
        for (int i = 0; i < NUM_LEDS; i++)
        {
            if (targets_to_press[i] == 1)
            {
                strip_SetPixelColor(i, RgbColor(0, 255, 0)); // Green for coordination
            }
        }
        strip_Show();

        Serial.print("Coord Level ");
        Serial.print(current_level);
        Serial.print(" targets: ");
        for (int i = 0; i < NUM_LEDS; i++)
        {
            if (targets_to_press[i] == 1)
            {
                Serial.print(i);
                Serial.print(" ");
            }
        }
        Serial.println();
        break;
    }

    case CT_STATE_WAIT_FOR_PRESS:
        lv_label_set_text(info_label, "Натисни кнопки!");
        round_start_time = lv_tick_get();
        break;

    case CT_STATE_ROUND_COMPLETE:
        lv_label_set_text(info_label, "Правильно!");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0x00FF00), 0);
        strip_Clear();
        break;

    case CT_STATE_GAME_OVER:
        lv_label_set_text(info_label, "Гру завершено!");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        strip_Clear();
        break;

    case CT_STATE_SHOW_RESULTS:
        display_results();
        break;

    case CT_STATE_GAME_OVER_MENU:
        create_game_over_menu();
        break;

    default:
        break;
    }
}

static void update_level_display()
{
    char level_text[32];
    snprintf(level_text, sizeof(level_text), "Рівень: %d", current_level);
    lv_label_set_text(level_label, level_text);
}

static void check_button_presses_coordination()
{
    if (current_trainer_state != CT_STATE_WAIT_FOR_PRESS)
        return;

    uint16_t current_button_state = expanderRead();

    for (int i = 0; i < NUM_LEDS; i++)
    {
        bool was_pressed = !(last_button_state & (1 << i));
        bool is_pressed = !(current_button_state & (1 << i));

        if (!was_pressed && is_pressed) // Button just pressed
        {
            if (targets_to_press[i] == 1 && targets_pressed[i] == 0)
            {
                // Correct button that hasn't been pressed yet
                targets_pressed[i] = 1;
                correct_presses_in_level++;
                strip_SetPixelColor(i, RgbColor(0, 255, 0)); // Green feedback
                strip_Show();
                delay(100); // Small delay for visual feedback
                strip_Clear();
                strip_Show();

                Serial.printf("Coord: Correct button %d (%d/%d)\n", i, correct_presses_in_level, current_level);

                // Check if all buttons are pressed
                if (correct_presses_in_level == current_level)
                {
                    correct_coordination_presses++;
                    total_coordination_rounds++;
                    set_coordination_trainer_state(CT_STATE_ROUND_COMPLETE);
                }
                else
                {
                    round_start_time = lv_tick_get(); // Reset timer
                }
            }
            else if (targets_to_press[i] == 1 && targets_pressed[i] == 1)
            {
                // Button already pressed
                strip_SetPixelColor(i, RgbColor(0, 255, 0)); // Green feedback
                strip_Show();
                delay(100);
                strip_Clear();
                strip_Show();
                Serial.printf("Coord: Button %d already pressed\n", i);
            }
            else
            {
                // Wrong button
                strip_SetPixelColor(i, RgbColor(255, 0, 0)); // Red feedback
                strip_Show();
                delay(100);
                strip_Clear();
                strip_Show();
                Serial.printf("Coord: Wrong button %d\n", i);
                total_coordination_rounds++;
                set_coordination_trainer_state(CT_STATE_GAME_OVER);
            }
            break;
        }
    }

    last_button_state = current_button_state;

    // Check for timeout
    if (lv_tick_get() - round_start_time > COORDINATION_TIMEOUT)
    {
        Serial.println("Coord: Timeout");
        total_coordination_rounds++;
        set_coordination_trainer_state(CT_STATE_GAME_OVER);
    }
}

void run_coordination_trainer()
{
    switch (current_trainer_state)
    {
    case CT_STATE_GET_READY:
        if (lv_tick_get() - state_timer > GET_READY_DURATION)
        {
            set_coordination_trainer_state(CT_STATE_SHOW_TARGET);
        }
        break;

    case CT_STATE_SHOW_TARGET:
        // Show targets for target_show_duration
        if (lv_tick_get() - state_timer > target_show_duration)
        {
            strip_Clear();
            strip_Show();
            set_coordination_trainer_state(CT_STATE_WAIT_FOR_PRESS);
        }
        break;

    case CT_STATE_WAIT_FOR_PRESS:
        check_button_presses_coordination();
        break;

    case CT_STATE_ROUND_COMPLETE:
        if (lv_tick_get() - state_timer > ROUND_COMPLETE_DURATION)
        {
            // Move to next level
            int max_level;
            if (current_submenu_state == CS_EASY_MODE)
            {
                max_level = COORDINATION_EASY_MAX_LEDS;
            }
            else if (current_submenu_state == CS_HARD_MODE)
            {
                max_level = COORDINATION_HARD_MAX_LEDS;
            }
            else
            {
                max_level = NUM_LEDS; // fallback
            }

            if (current_level < max_level && current_level < NUM_LEDS)
            {
                current_level++;
                // Decrease show time with each level
                target_show_duration = max((int)COORDINATION_MIN_SHOW_TIME,
                                           (int)(COORDINATION_INITIAL_SHOW_TIME - (current_level - 1) * COORDINATION_TIME_DECREASE_STEP));
                set_coordination_trainer_state(CT_STATE_SHOW_TARGET);
            }
            else
            {
                // Reached maximum level
                set_coordination_trainer_state(CT_STATE_GAME_OVER);
            }
        }
        break;

    case CT_STATE_GAME_OVER:
        if (lv_tick_get() - state_timer > GAME_OVER_MESSAGE_DURATION)
        {
            set_coordination_trainer_state(CT_STATE_SHOW_RESULTS);
        }
        break;

    case CT_STATE_SHOW_RESULTS:
        if (lv_tick_get() - state_timer > SURVIVAL_RESULTS_DISPLAY_DURATION)
        {
            set_coordination_trainer_state(CT_STATE_GAME_OVER_MENU);
        }
        break;

    case CT_STATE_GAME_OVER_MENU:
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

    char results_text[256];
    snprintf(results_text, sizeof(results_text),
             "Результати Координації:\n\nДосягнутий рівень: %d\nПройдених рівнів: %d\nВсього спроб: %d",
             current_level, correct_coordination_presses, total_coordination_rounds);

    if (current_level == NUM_LEDS)
    {
        strcat(results_text, "\n\nІДЕАЛЬНИЙ РЕЗУЛЬТАТ!");
    }

    lv_label_set_text(results_label, results_text);
}

static void create_game_over_menu()
{
    // Hide other labels
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Create play again button
    play_again_btn = lv_btn_create(coordination_screen);
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
    exit_btn = lv_btn_create(coordination_screen);
    lv_obj_set_size(exit_btn, 300, 80);
    lv_obj_align(exit_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xAA0000), LV_STATE_PRESSED);

    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "Вихід");
    lv_obj_set_style_text_font(exit_label, Font2, 0);
    lv_obj_center(exit_label);

    lv_obj_add_event_cb(exit_btn, game_over_menu_event_handler, LV_EVENT_CLICKED, (void *)1);
}

static void game_over_menu_event_handler(lv_event_t *e)
{
    int action = (int)(intptr_t)lv_event_get_user_data(e);

    if (action == 0) // Play again
    {
        Serial.println("Coord Menu: Play Again");
        set_coordination_trainer_state(CT_STATE_GET_READY);
    }
    else if (action == 1) // Exit
    {
        Serial.println("Coord Menu: Exit");
        last_interaction_time = lv_tick_get(); // Add this
        current_state = STATE_COORDINATION_SUBMENU;
        set_coordination_trainer_state(CT_STATE_IDLE);
        current_submenu_state = CS_SUBMENU_IDLE;
        create_coordination_submenu();
    }
}

static void back_to_menu_event_handler(lv_event_t *e)
{
    Serial.println("Coord: Back to menu");
    last_interaction_time = lv_tick_get(); // Add this
    current_state = STATE_COORDINATION_SUBMENU;
    set_coordination_trainer_state(CT_STATE_IDLE);
    current_submenu_state = CS_SUBMENU_IDLE;
    create_coordination_submenu();
}

// Submenu state setters
void set_coordination_easy_mode() { current_submenu_state = CS_EASY_MODE; }
void set_coordination_hard_mode() { current_submenu_state = CS_HARD_MODE; }