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
static lv_obj_t *back_btn = NULL;

// === Forward Declarations ===
static void update_level_display();
static void check_button_presses_memory();
static void check_hardware_back_button();
static void generate_new_random_sequence();
static void display_results();
static void game_over_menu_event_handler(lv_event_t *e);
static void back_to_menu_event_handler(lv_event_t *e);

void create_memory_trainer_screen()
{
    // Use base function to create common elements
    TrainerScreenElements elements = create_trainer_screen_base(back_to_menu_event_handler);
    
    // Assign to local static variables
    memory_screen = elements.screen;
    level_label = elements.top_label;
    info_label = elements.info_label;
    results_label = elements.results_label;
    back_btn = elements.back_btn;

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
        display_results();
        Serial.println("Mem: GAME_OVER_MENU state set, simple results displayed");
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

    for (int i = 0; i < NUM_LEDS; i++)
    {
        bool was_pressed = !(last_button_state & (1 << i));
        bool is_pressed = !(current_button_state & (1 << i));

        // Only register button press on rising edge (not pressed -> pressed)
        if (!was_pressed && is_pressed) 
        {
            Serial.printf("Mem Btn %d pressed\n", i);

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
                set_memory_trainer_state(MT_STATE_GAME_OVER);
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

static void check_hardware_back_button()
{
    // Check for hardware back button press (assuming button 15 is back button)
    static uint16_t last_back_button_state = 0xFFFF;
    static unsigned long last_back_check_time = 0;
    static unsigned long last_debug_print = 0;
    
    // Debug print every 3 seconds to show we're being called
    if (lv_tick_get() - last_debug_print > 3000) {
        Serial.printf("Mem: check_hardware_back_button() called in state %d\n", memory_trainer_state);
        last_debug_print = lv_tick_get();
    }
    
    // Debouncing
    if (lv_tick_get() - last_back_check_time < 100) {
        return;
    }
    last_back_check_time = lv_tick_get();
    
    uint16_t current_button_state = expanderRead();
    
    // Check if back button (15) was pressed
    bool was_pressed = !(last_back_button_state & (1 << 15));
    bool is_pressed = !(current_button_state & (1 << 15));
    
    if (!was_pressed && is_pressed) {
        Serial.printf("Mem: Hardware back button pressed in state: %d\n", memory_trainer_state);
        back_to_menu_event_handler(NULL);
    }
    
    last_back_button_state = current_button_state;
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
        check_hardware_back_button();
        if (lv_tick_get() - memory_trainer_timer > GAME_OVER_MESSAGE_DURATION)
        {
            Serial.println("Mem: GAME_OVER -> GAME_OVER_MENU");
            set_memory_trainer_state(MT_STATE_GAME_OVER_MENU);
        }
        break;
    }

    case MT_STATE_GAME_OVER_MENU:
        // Wait for user input on buttons
        check_hardware_back_button();
        
        // Add periodic heartbeat to check if we're still running
        static unsigned long last_heartbeat = 0;
        if (lv_tick_get() - last_heartbeat > 5000) { // Every 5 seconds
            Serial.println("Mem: GAME_OVER_MENU heartbeat - still running");
            last_heartbeat = lv_tick_get();
        }
        break;

    default:
        break;
    }
}

static void display_results()
{
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(results_label, "РЕЗУЛЬТАТИ");
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
    Serial.printf("Mem: Back button clicked in state: %d\n", memory_trainer_state);
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