#include "coordination_trainer.h"
#include "hardware_abstraction.h"
#include "app_screens.h"
#include "globals.h"
#include "uart_protocol.h"
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

// === UI Elements ===
static lv_obj_t *coordination_screen = NULL;
static lv_obj_t *level_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_obj_t *results_label = NULL;

// === Forward Declarations ===
static void update_level_display();
static void check_button_presses_coordination();
static void display_results();
static void back_to_menu_event_handler(lv_event_t *e);

// Async wrappers for transitions
static void async_open_coordination_submenu(void *user_data) { create_coordination_submenu(); }
static void async_restart_coordination(void *user_data) { set_coordination_trainer_state(CT_STATE_GET_READY); }


void create_coordination_trainer_screen(AppState target_mode)
{
    // Use base function to create common elements
    TrainerScreenElements elements = create_trainer_screen_base(back_to_menu_event_handler);
    
    // Assign to local static variables
    coordination_screen = elements.screen;
    level_label = elements.top_label;
    info_label = elements.info_label;
    results_label = elements.results_label;
    // back_btn is local in base function

    // Update global app state ONLY after UI is ready
    current_state = target_mode;

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
        playAudioPrompt(AUDIO_GET_READY);  // Голосова підказка
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
        playAudioPrompt(AUDIO_REMEMBER_BTNS);  // Голосова підказка
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
        playAudioPrompt(AUDIO_PRESS_BTNS);  // Голосова підказка
        round_start_time = lv_tick_get();
        break;

    case CT_STATE_ROUND_COMPLETE:
        lv_label_set_text(info_label, "Правильно!");
        playAudioPrompt(AUDIO_CORRECT);  // Голосова підказка
        lv_obj_set_style_text_color(info_label, lv_color_hex(0x00FF00), 0);
        strip_Clear();
        break;

    case CT_STATE_GAME_OVER:
        lv_label_set_text(info_label, "Гру завершено!");
        playAudioPrompt(AUDIO_GAME_OVER);  // Голосова підказка
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        strip_Clear();
        break;

    case CT_STATE_SHOW_RESULTS:
        display_results();
        break;

    case CT_STATE_GAME_OVER_MENU:
        display_results();
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
    lv_obj_add_flag(level_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    int max_level = (current_submenu_state == CS_EASY_MODE) ? 
        COORDINATION_EASY_MAX_LEDS : COORDINATION_HARD_MAX_LEDS;
    
    // === Зберігаємо статистику пацієнта ===
    PatientStats *stats = &patientStats[currentPatientIndex];
    stats->coordination_sessions++;
    stats->coordination_total_hits += correct_coordination_presses;
    if (current_level > stats->coordination_best_score)
    {
        stats->coordination_best_score = current_level;
    }
    // Записуємо в історію сесій (рівень, влучення)
    addCoordinationSession(currentPatientIndex, current_level, correct_coordination_presses);
    savePatientStats(currentPatientIndex);
    // =======================================
    
    char results_text[32];
    snprintf(results_text, sizeof(results_text),
             "%d / %d",
             current_level, max_level);
    
    lv_obj_set_style_text_font(results_label, &lv_lilita_one_regular_96, 0);
    lv_label_set_text(results_label, results_text);
    lv_obj_set_style_text_align(results_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(results_label, LV_ALIGN_CENTER, 0, 0);
    
    if (current_level >= max_level / 2) {
        lv_obj_set_style_text_color(results_label, lv_color_hex(0x00FF00), 0);
    } else {
        lv_obj_set_style_text_color(results_label, lv_color_white(), 0);
    }
    lv_obj_update_layout(results_label);
}

static void back_to_menu_event_handler(lv_event_t *e)
{
    Serial.println("Coord: Back to menu");
    last_interaction_time = lv_tick_get();
    current_state = STATE_COORDINATION_SUBMENU;
    set_coordination_trainer_state(CT_STATE_IDLE);
    current_submenu_state = CS_SUBMENU_IDLE;
    lv_async_call(async_open_coordination_submenu, NULL);
}

// Submenu state setters
void set_coordination_easy_mode() { current_submenu_state = CS_EASY_MODE; }
void set_coordination_hard_mode() { current_submenu_state = CS_HARD_MODE; }