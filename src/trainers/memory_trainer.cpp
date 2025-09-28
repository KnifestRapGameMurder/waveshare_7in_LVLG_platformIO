#include "memory_trainer.h"
#include "hardware_abstraction.h"
#include "app_screens.h"
#include <Arduino.h>
#include <lvgl.h>

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

// External font
extern const lv_font_t minecraft_ten_48;

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
    lv_obj_set_style_text_font(level_label, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(level_label, lv_color_white(), 0);
    lv_obj_align(level_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create info label
    info_label = lv_label_create(memory_screen);
    lv_obj_set_style_text_font(info_label, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);

    // Create results label (hidden initially)
    results_label = lv_label_create(memory_screen);
    lv_obj_set_style_text_font(results_label, &minecraft_ten_48, 0);
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
    lv_obj_set_style_text_font(back_label, &minecraft_ten_48, 0);
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
        current_sequence_length = 1;
        current_sequence_step = 0;
        current_user_input_step = 0;

        // Initialize arrays
        for (int i = 0; i < MAX_SEQUENCE_LENGTH; i++)
        {
            memory_sequence[i] = -1;
            user_input_sequence[i] = -1;
        }

        // Generate new random sequence
        generate_new_random_sequence();
        update_level_display();
        strip_Clear();
        break;

    case MT_STATE_SHOW_SEQUENCE:
        lv_label_set_text(info_label, "Запам'ятовуй...");
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
        }

        if (current_sequence_step < current_sequence_length)
        {
            Serial.print("Mem Show LED ");
            Serial.println(memory_sequence[current_sequence_step]);
            strip_SetPixelColor(memory_sequence[current_sequence_step], RgbColor(255, 0, 255)); // Purple for memory
            strip_Show();
        }
        else
        {
            set_memory_trainer_state(MT_STATE_WAIT_FOR_INPUT);
        }
        break;

    case MT_STATE_WAIT_FOR_INPUT:
        lv_label_set_text(info_label, "Твоя черга!");
        current_user_input_step = 0;
        strip_Clear();
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
        break;

    case MT_STATE_GAME_OVER_MENU:
        create_game_over_menu();
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

    // Generate new random sequence
    for (int pos = 0; pos < current_sequence_length; pos++)
    {
        int attempts = 0;
        int chosen = -1;

        do
        {
            chosen = random(NUM_LEDS);
            attempts++;

            // Check if button repeats with previous positions
            bool isValid = true;

            // Avoid direct repeats
            if (pos > 0 && chosen == memory_sequence[pos - 1])
            {
                isValid = false;
            }

            // Avoid repeats two positions back
            if (pos > 1 && chosen == memory_sequence[pos - 2])
            {
                isValid = false;
            }

            // For longer sequences, avoid too frequent repeats
            if (pos > 2)
            {
                int repeatCount = 0;
                for (int j = 0; j < pos; j++)
                {
                    if (memory_sequence[j] == chosen)
                    {
                        repeatCount++;
                    }
                }
                // Don't allow more than 1/3 of positions to be the same
                if (repeatCount > (pos / 3))
                {
                    isValid = false;
                }
            }

            if (isValid)
            {
                memory_sequence[pos] = chosen;
                break;
            }

        } while (attempts < 50); // Limit attempts

        // If no suitable option found, take any different from previous
        if (attempts >= 50)
        {
            do
            {
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

    for (int i = 0; i < NUM_LEDS; i++)
    {
        bool was_pressed = !(last_button_state & (1 << i));
        bool is_pressed = !(current_button_state & (1 << i));

        if (!was_pressed && is_pressed) // Button just pressed
        {
            Serial.print("Mem Btn ");
            Serial.println(i);

            // Show feedback
            strip_SetPixelColor(i, RgbColor(255, 0, 255)); // Purple feedback
            strip_Show();
            delay(120); // Small delay for visual feedback
            strip_Clear();
            strip_Show();

            user_input_sequence[current_user_input_step] = i;

            if (user_input_sequence[current_user_input_step] == memory_sequence[current_user_input_step])
            {
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
                Serial.println("Mem: Wrong!");
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

void run_memory_trainer()
{
    switch (memory_trainer_state)
    {
    case MT_STATE_GET_READY:
        if (lv_tick_get() - memory_trainer_timer > GET_READY_DURATION)
        {
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

            if (elapsed > show_dur && elapsed <= (show_dur + pause_dur))
            {
                strip_Clear();
                strip_Show();
            }
            else if (elapsed > (show_dur + pause_dur))
            {
                current_sequence_step++;
                if (current_sequence_step < current_sequence_length)
                {
                    strip_SetPixelColor(memory_sequence[current_sequence_step], RgbColor(255, 0, 255));
                    strip_Show();
                    memory_trainer_timer = lv_tick_get();
                }
                else
                {
                    set_memory_trainer_state(MT_STATE_WAIT_FOR_INPUT);
                }
            }
        }
        break;

    case MT_STATE_WAIT_FOR_INPUT:
        check_button_presses_memory();
        break;

    case MT_STATE_ROUND_COMPLETE:
    {
        // Pulse all LEDs green
        unsigned long elapsed = lv_tick_get() - memory_trainer_timer;
        if (elapsed < ROUND_COMPLETE_DURATION)
        {
            float phase = (elapsed % 600) / 600.0f;                                     // 0..1
            float s = (phase < 0.5f) ? (phase * 2.0f) : (1.0f - (phase - 0.5f) * 2.0f); // Triangle 0..1..0
            uint8_t bright = (uint8_t)(255 * (0.3f + 0.7f * s));
            for (int i = 0; i < NUM_LEDS; i++)
            {
                strip_SetPixelColor(i, RgbColor(0, bright, 0));
            }
            strip_Show();
        }

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
                set_memory_trainer_state(MT_STATE_SHOW_SEQUENCE);
            }
        }
        break;
    }

    case MT_STATE_GAME_OVER:
    {
        // Red pulse on defeat
        unsigned long elapsed_go = lv_tick_get() - memory_trainer_timer;
        if (elapsed_go < GAME_OVER_MESSAGE_DURATION)
        {
            float phase = (elapsed_go % 500) / 500.0f; // 0..1
            float w = sinf(phase * 3.14159f);          // 0..1..0
            uint8_t bright = (uint8_t)(255 * (0.2f + 0.8f * w));
            for (int i = 0; i < NUM_LEDS; i++)
            {
                strip_SetPixelColor(i, RgbColor(bright, 0, 0));
            }
            strip_Show();
        }

        if (lv_tick_get() - memory_trainer_timer > GAME_OVER_MESSAGE_DURATION)
        {
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
    // Hide other labels
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Create play again button
    play_again_btn = lv_btn_create(memory_screen);
    lv_obj_set_size(play_again_btn, 300, 80);
    lv_obj_align(play_again_btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(play_again_btn, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_bg_color(play_again_btn, lv_color_hex(0x00AA00), LV_STATE_PRESSED);

    lv_obj_t *play_label = lv_label_create(play_again_btn);
    lv_label_set_text(play_label, "Грати Знову");
    lv_obj_set_style_text_font(play_label, &minecraft_ten_48, 0);
    lv_obj_center(play_label);

    lv_obj_add_event_cb(play_again_btn, game_over_menu_event_handler, LV_EVENT_CLICKED, (void *)0);

    // Create exit button
    exit_btn = lv_btn_create(memory_screen);
    lv_obj_set_size(exit_btn, 300, 80);
    lv_obj_align(exit_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xAA0000), LV_STATE_PRESSED);

    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "Вихід");
    lv_obj_set_style_text_font(exit_label, &minecraft_ten_48, 0);
    lv_obj_center(exit_label);

    lv_obj_add_event_cb(exit_btn, game_over_menu_event_handler, LV_EVENT_CLICKED, (void *)1);
}

static void game_over_menu_event_handler(lv_event_t *e)
{
    int action = (int)(intptr_t)lv_event_get_user_data(e);

    if (action == 0) // Play again
    {
        Serial.println("Mem Menu: Play Again");
        set_memory_trainer_state(MT_STATE_GET_READY);
    }
    else if (action == 1) // Exit
    {
        Serial.println("Mem Menu: Exit");
        current_state = STATE_MAIN_MENU;
        set_memory_trainer_state(MT_STATE_IDLE);
        create_main_menu();
    }
}

static void back_to_menu_event_handler(lv_event_t *e)
{
    Serial.println("Mem: Back to menu");
    current_state = STATE_MAIN_MENU;
    set_memory_trainer_state(MT_STATE_IDLE);
    create_main_menu();
}