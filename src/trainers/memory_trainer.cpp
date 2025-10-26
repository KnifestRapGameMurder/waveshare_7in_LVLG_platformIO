#include "memory_trainer.h"
#include "hardware_abstraction.h"
#include "app_screens.h"
#include <Arduino.h>
#include <lvgl.h>
#include "fonts.h"

// === Game Constants ===
const int GET_READY_DURATION = 3000;         // 3 seconds
const int LED_SHOW_DURATION = 600;           // ms to show each LED
const int LED_PAUSE_DURATION = 300;          // ms pause between LEDs
const int INPUT_TIMEOUT = 10000;             // 10 seconds to input sequence
const int ROUND_COMPLETE_DURATION = 2000;    // 2 seconds to show success
const int GAME_OVER_MESSAGE_DURATION = 2000; // 2 seconds to show game over
const int MAX_SEQUENCE_LENGTH = 8;           // Maximum sequence length

// === Game State Variables ===
static MemoryTrainerState memory_trainer_state = MT_STATE_IDLE;
static int memory_sequence[MAX_SEQUENCE_LENGTH];     // Sequence to remember
static int current_sequence_length = 1;              // Current sequence length
static int current_sequence_step = 0;                // Step in showing sequence
static int user_input_sequence[MAX_SEQUENCE_LENGTH]; // User's input
static int current_user_input_step = 0;              // Step in user input
static unsigned long memory_trainer_timer = 0;       // Timer for states
static uint16_t last_button_state = 0xFFFF;          // Start with all buttons released

// Quick LED feedback without delay()
static bool led_feedback_active = false;
static int led_feedback_index = -1;
static unsigned long led_feedback_start = 0;

// Sequence showing state
static bool led_is_showing = false;

// === UI Elements ===
static lv_obj_t *memory_screen = NULL;
static lv_obj_t *level_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_obj_t *results_label = NULL;
static lv_obj_t *play_again_btn = NULL;
static lv_obj_t *exit_btn = NULL;

// === Forward Declarations ===
static void update_level_display();
static void check_button_presses_memory();
static void generate_new_random_sequence();
static void display_results();
static void create_game_over_menu();
static void game_over_menu_event_handler(lv_event_t *e);
static void back_to_menu_event_handler(lv_event_t *e);

void create_memory_trainer_screen()
{
    // Clean the screen
    lv_obj_clean(lv_scr_act());

    // Create main screen container
    memory_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(memory_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(memory_screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(memory_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Create level label
    level_label = lv_label_create(memory_screen);
    lv_obj_set_style_text_font(level_label, Font2, 0);
    lv_obj_set_style_text_color(level_label, lv_color_white(), 0);
    lv_obj_align(level_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create info label
    info_label = lv_label_create(memory_screen);
    lv_obj_set_style_text_font(info_label, Font2, 0);
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);

    // Create results label (hidden initially)
    results_label = lv_label_create(memory_screen);
    lv_obj_set_style_text_font(results_label, Font2, 0);
    lv_obj_set_style_text_color(results_label, lv_color_white(), 0);
    lv_obj_align(results_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Create back button
    lv_obj_t *back_btn = lv_btn_create(memory_screen);
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
    set_memory_trainer_state(MT_STATE_GET_READY);
}

void set_memory_trainer_state(MemoryTrainerState newState)
{
    memory_trainer_state = newState;
    memory_trainer_timer = lv_tick_get();

    switch (memory_trainer_state)
    {
    case MT_STATE_IDLE:
        strip_Clear();
        break;

    case MT_STATE_GET_READY:
        lv_label_set_text(info_label, "Приготуйся!");
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
        current_sequence_length = 1;
        current_sequence_step = 0;
        current_user_input_step = 0;

        // Initialize arrays
        for (int i = 0; i < MAX_SEQUENCE_LENGTH; i++)
        {
            memory_sequence[i] = -1;
            user_input_sequence[i] = -1;
        }

        // Initialize button state to current state to prevent false triggers
        last_button_state = expanderRead();
        Serial.printf("Mem: Initial button state: 0x%04X (binary: ", last_button_state);
        for (int i = 15; i >= 0; i--) {
            Serial.print((last_button_state & (1 << i)) ? '1' : '0');
        }
        Serial.println(")");

        // Generate new random sequence
        generate_new_random_sequence();
        Serial.printf("Mem: Generated sequence for level %d\n", current_sequence_length);
        update_level_display();
        strip_Clear();
        strip_Show();
        break;

    case MT_STATE_SHOW_SEQUENCE:
        lv_label_set_text(info_label, "Запам'ятовуй...");
        lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
        update_level_display();

        if (current_sequence_step == 0)
        {
            Serial.print("Mem L");
            Serial.print(current_sequence_length);
            Serial.print(": ");
            for (int i = 0; i < current_sequence_length; i++)
            {
                Serial.print(memory_sequence[i]);
                Serial.print(' ');
            }
            Serial.println();
            
            // Reset sequence showing state
            led_is_showing = false;
        }
        break;

    case MT_STATE_WAIT_FOR_INPUT:
        lv_label_set_text(info_label, "Твоя черга!");
        lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
        current_user_input_step = 0;
        // Reset button state to prevent false triggers
        last_button_state = expanderRead();
        Serial.printf("Mem: WAIT_FOR_INPUT button state: 0x%04X (binary: ", last_button_state);
        for (int i = 15; i >= 0; i--) {
            Serial.print((last_button_state & (1 << i)) ? '1' : '0');
        }
        Serial.println(")");
        strip_Clear();
        strip_Show();
        break;

    case MT_STATE_ROUND_COMPLETE:
        lv_label_set_text(info_label, "Правильно!");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0x00FF00), 0);
        current_sequence_length++;
        if (current_sequence_length <= MAX_SEQUENCE_LENGTH)
        {
            generate_new_random_sequence();
        }
        break;

    case MT_STATE_GAME_OVER:
        lv_label_set_text(info_label, "Гру завершено!");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        strip_Clear();
        Serial.printf("Mem: GAME_OVER state set, timer: %lu\n", memory_trainer_timer);
        break;

    case MT_STATE_GAME_OVER_MENU:
        create_game_over_menu();
        Serial.println("Mem: GAME_OVER_MENU state set, menu created");
        break;

    default:
        break;
    }
}

static void update_level_display()
{
    char level_text[32];
    snprintf(level_text, sizeof(level_text), "Рівень: %d", current_sequence_length);
    lv_label_set_text(level_label, level_text);
}

static void generate_new_random_sequence()
{
    // Clear the sequence array
    for (int i = 0; i < MAX_SEQUENCE_LENGTH; i++)
    {
        memory_sequence[i] = -1;
    }
    
    // Generate new random sequence using simple but reliable logic from working version
    for (int pos = 0; pos < current_sequence_length; pos++) {
        int attempts = 0;
        int chosen = -1;
        
        do {
            chosen = random(NUM_LEDS);
            attempts++;
            
            // Check if button doesn't repeat with previous positions
            bool isValid = true;
            
            // Avoid direct repeats
            if (pos > 0 && chosen == memory_sequence[pos - 1]) {
                isValid = false;
            }
            
            // Avoid repeats one position back
            if (pos > 1 && chosen == memory_sequence[pos - 2]) {
                isValid = false;
            }
            
            // For longer sequences avoid too frequent repeats
            if (pos > 2) {
                int repeatCount = 0;
                for (int j = 0; j < pos; j++) {
                    if (memory_sequence[j] == chosen) {
                        repeatCount++;
                    }
                }
                // Don't allow more than 1/3 positions to be the same
                if (repeatCount > (pos / 3)) {
                    isValid = false;
                }
            }
            
            if (isValid) {
                memory_sequence[pos] = chosen;
                break;
            }
            
        } while (attempts < 50); // limit number of attempts
        
        // If no suitable variant found, take any different from previous
        if (attempts >= 50) {
            do {
                chosen = random(NUM_LEDS);
            } while (pos > 0 && chosen == memory_sequence[pos - 1]);
            memory_sequence[pos] = chosen;
        }
    }
}

static void check_button_presses_memory()
{
    if (memory_trainer_state != MT_STATE_WAIT_FOR_INPUT)
        return;

    uint16_t current_button_state = expanderRead();

    // Add debouncing - wait at least 50ms between checks
    static unsigned long last_check_time = 0;
    if (lv_tick_get() - last_check_time < 50) {
        return;
    }
    last_check_time = lv_tick_get();

    // Debug: print button state changes
    if (current_button_state != last_button_state) {
        Serial.printf("Mem: Button state changed from 0x%04X to 0x%04X\n", last_button_state, current_button_state);
        Serial.print("Mem: Previous: ");
        for (int i = 15; i >= 0; i--) {
            Serial.print((last_button_state & (1 << i)) ? '1' : '0');
        }
        Serial.print(", Current: ");
        for (int i = 15; i >= 0; i--) {
            Serial.print((current_button_state & (1 << i)) ? '1' : '0');
        }
        Serial.println();
    }

    for (int i = 0; i < NUM_LEDS; i++)
    {
        bool was_pressed = !(last_button_state & (1 << i));
        bool is_pressed = !(current_button_state & (1 << i));

        // Debug: print logic for each button
        if (current_button_state != last_button_state) {
            uint8_t last_bit = (last_button_state & (1 << i)) ? 1 : 0;
            uint8_t curr_bit = (current_button_state & (1 << i)) ? 1 : 0;
            Serial.printf("Mem: Btn%d - bit: %d->%d, pressed: %d->%d\n", 
                i, last_bit, curr_bit, was_pressed, is_pressed);
        }

        // Only register button press on rising edge (not pressed -> pressed)
        if (!was_pressed && is_pressed) 
        {
            Serial.printf("Mem Btn %d pressed (was: %d, now: %d)\n", i, was_pressed, is_pressed);

            // Show feedback without delay
            strip_SetPixelColor(i, RgbColor(255, 0, 255)); // Purple feedback
            strip_Show();
            led_feedback_active = true;
            led_feedback_index = i;
            led_feedback_start = lv_tick_get();

            user_input_sequence[current_user_input_step] = i;

            if (user_input_sequence[current_user_input_step] == memory_sequence[current_user_input_step])
            {
                Serial.printf("Mem: Correct! Expected %d, got %d\n", memory_sequence[current_user_input_step], i);
                current_user_input_step++;
                if (current_user_input_step == current_sequence_length)
                {
                    set_memory_trainer_state(MT_STATE_ROUND_COMPLETE);
                }
                else
                {
                    memory_trainer_timer = lv_tick_get();
                }
            }
            else
            {
                Serial.printf("Mem: Wrong! Expected %d, got %d\n", memory_sequence[current_user_input_step], i);
                // Skip the GAME_OVER delay and go directly to menu
                set_memory_trainer_state(MT_STATE_GAME_OVER_MENU);
            }
            break;
        }
    }

    last_button_state = current_button_state;

    // Check for timeout
    if (lv_tick_get() - memory_trainer_timer > INPUT_TIMEOUT)
    {
        Serial.println("Mem: Timeout");
        set_memory_trainer_state(MT_STATE_GAME_OVER);
    }
}

void run_memory_trainer()
{
    switch (memory_trainer_state)
    {
    case MT_STATE_GET_READY:
        if (lv_tick_get() - memory_trainer_timer > GET_READY_DURATION)
        {
            Serial.println("Mem: GET_READY -> SHOW_SEQUENCE");
            set_memory_trainer_state(MT_STATE_SHOW_SEQUENCE);
        }
        break;

    case MT_STATE_SHOW_SEQUENCE:
        if (current_sequence_step < current_sequence_length)
        {
            unsigned long elapsed = lv_tick_get() - memory_trainer_timer;
            // Adaptive timings (faster at higher levels)
            float accel = 0.5f * ((float)(current_sequence_length - 1) / (float)(MAX_SEQUENCE_LENGTH - 1)); // 0..0.5
            unsigned long show_dur = (unsigned long)(LED_SHOW_DURATION * (1.0f - accel));
            unsigned long pause_dur = (unsigned long)(LED_PAUSE_DURATION * (1.0f - accel * 0.6f));

            if (elapsed <= show_dur)
            {
                // Show current LED in sequence (only once per step)
                if (!led_is_showing) {
                    Serial.printf("Mem: Showing step %d, LED %d\n", current_sequence_step, memory_sequence[current_sequence_step]);
                    strip_Clear();
                    strip_SetPixelColor(memory_sequence[current_sequence_step], RgbColor(255, 0, 255)); // Purple
                    strip_Show();
                    led_is_showing = true;
                }
            }
            else if (elapsed > show_dur && elapsed <= (show_dur + pause_dur))
            {
                // Pause between LEDs (only clear once)
                if (led_is_showing) {
                    strip_Clear();
                    strip_Show();
                    led_is_showing = false;
                }
            }
            else if (elapsed > (show_dur + pause_dur))
            {
                // Move to next step
                current_sequence_step++;
                memory_trainer_timer = lv_tick_get(); // Reset timer for next LED
                led_is_showing = false; // Reset for next LED
                Serial.printf("Mem: Moving to step %d of %d\n", current_sequence_step, current_sequence_length);
                
                if (current_sequence_step >= current_sequence_length)
                {
                    // Finished showing sequence, wait for input
                    Serial.println("Mem: SHOW_SEQUENCE -> WAIT_FOR_INPUT");
                    set_memory_trainer_state(MT_STATE_WAIT_FOR_INPUT);
                }
            }
        }
        break;

    case MT_STATE_WAIT_FOR_INPUT:
        check_button_presses_memory();
        // Handle LED feedback timeout
        if (led_feedback_active && lv_tick_get() - led_feedback_start > 120) {
            led_feedback_active = false;
            led_feedback_index = -1;
            strip_Clear();
            strip_Show();
        }
        break;

    case MT_STATE_ROUND_COMPLETE:
    {
        // Just wait without LED animation
        if (lv_tick_get() - memory_trainer_timer > ROUND_COMPLETE_DURATION)
        {
            if (current_sequence_length > MAX_SEQUENCE_LENGTH)
            {
                set_memory_trainer_state(MT_STATE_GAME_OVER);
            }
            else
            {
                current_sequence_step = 0;
                current_user_input_step = 0;
                led_is_showing = false; // Reset LED showing state
                set_memory_trainer_state(MT_STATE_SHOW_SEQUENCE);
            }
        }
        break;
    }

    case MT_STATE_GAME_OVER:
    {
        // Just wait without LED animation
        unsigned long elapsed = lv_tick_get() - memory_trainer_timer;
        Serial.printf("Mem: GAME_OVER - elapsed: %lu, target: %d\n", elapsed, GAME_OVER_MESSAGE_DURATION);
        if (elapsed > GAME_OVER_MESSAGE_DURATION)
        {
            Serial.println("Mem: GAME_OVER -> GAME_OVER_MENU");
            set_memory_trainer_state(MT_STATE_GAME_OVER_MENU);
        }
        break;
    }

    case MT_STATE_GAME_OVER_MENU:
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
    if (current_sequence_length > MAX_SEQUENCE_LENGTH)
    {
        snprintf(results_text, sizeof(results_text),
                 "Результати Пам'яті:\n\nТи переміг!\nМаксимальний рівень: %d",
                 MAX_SEQUENCE_LENGTH);
    }
    else
    {
        snprintf(results_text, sizeof(results_text),
                 "Результати Пам'яті:\n\nТвій рівень: %d",
                 current_sequence_length - 1);
    }

    lv_label_set_text(results_label, results_text);
}

static void create_game_over_menu()
{
    // Hide other labels and clear buttons first
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);
    
    // Clean up existing buttons if they exist
    if (play_again_btn) {
        lv_obj_del(play_again_btn);
        play_again_btn = NULL;
    }
    if (exit_btn) {
        lv_obj_del(exit_btn);
        exit_btn = NULL;
    }

    // Display result message
    lv_obj_clear_flag(results_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(results_label, lv_color_white(), 0);
    
    char results_text[256];
    if (current_sequence_length > MAX_SEQUENCE_LENGTH)
    {
        snprintf(results_text, sizeof(results_text),
                 "Гру завершено!\n\nТи переміг!\nРівень: %d",
                 MAX_SEQUENCE_LENGTH);
    }
    else
    {
        snprintf(results_text, sizeof(results_text),
                 "Гру завершено!\n\nТвій рівень: %d",
                 current_sequence_length - 1);
    }
    
    lv_label_set_text(results_label, results_text);
    lv_obj_align(results_label, LV_ALIGN_CENTER, 0, -60);

    // Create play again button
    play_again_btn = lv_btn_create(memory_screen);
    lv_obj_set_size(play_again_btn, 300, 60);
    lv_obj_align(play_again_btn, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(play_again_btn, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_bg_color(play_again_btn, lv_color_hex(0x00AA00), LV_STATE_PRESSED);

    lv_obj_t *play_label = lv_label_create(play_again_btn);
    lv_label_set_text(play_label, "Грати Знову");
    lv_obj_set_style_text_font(play_label, Font2, 0);
    lv_obj_center(play_label);

    lv_obj_add_event_cb(play_again_btn, game_over_menu_event_handler, LV_EVENT_CLICKED, (void *)0);

    // Create exit button
    exit_btn = lv_btn_create(memory_screen);
    lv_obj_set_size(exit_btn, 300, 60);
    lv_obj_align(exit_btn, LV_ALIGN_CENTER, 0, 80);
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
        Serial.println("Mem Menu: Play Again");
        // Clean up buttons before resetting
        if (play_again_btn) {
            lv_obj_del(play_again_btn);
            play_again_btn = NULL;
        }
        if (exit_btn) {
            lv_obj_del(exit_btn);
            exit_btn = NULL;
        }
        set_memory_trainer_state(MT_STATE_GET_READY);
    }
    else if (action == 1) // Exit
    {
        Serial.println("Mem Menu: Exit");
        last_interaction_time = lv_tick_get();
        current_state = STATE_MAIN_MENU;
        set_memory_trainer_state(MT_STATE_IDLE);
        create_main_menu();
    }
}

static void back_to_menu_event_handler(lv_event_t *e)
{
    Serial.println("Mem: Back to menu");
    last_interaction_time = lv_tick_get();
    current_state = STATE_MAIN_MENU;
    set_memory_trainer_state(MT_STATE_IDLE);
    // Clean up LED feedback state
    led_feedback_active = false;
    led_feedback_index = -1;
    strip_Clear();
    strip_Show();
    create_main_menu();
}