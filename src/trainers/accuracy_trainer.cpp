#include "accuracy_trainer.h"
#include "hardware_abstraction.h"
#include "app_screens.h"
#include <Arduino.h>
#include <lvgl.h>
#include "fonts.h"

// === Game Constants ===
const int GET_READY_DURATION = 3000;           // 3 seconds
const int ACCURACY_TIMEOUT_EASY = 10000;       // 10 seconds for EASY
const int ACCURACY_TIMEOUT_MEDIUM = 15000;     // 15 seconds for MEDIUM (більше часу)
const int ACCURACY_TIMEOUT_HARD = 10000;       // 10 seconds for HARD
const int MAX_ACCURACY_MISSES = 5;             // Max misses before game over
const int MIN_ACCURACY_ROUNDS = 5;             // Minimum rounds per game
const int MAX_ACCURACY_ROUNDS = 10;            // Maximum rounds per game (не для MEDIUM)
const int FEEDBACK_DURATION = 260;             // ms for success/fail feedback effect
const int GAME_OVER_MSG_DURATION = 1000;       // ms to show "Game Over"
const int RESULTS_DISPLAY_DURATION = 5000;     // ms to show results

// Speed constants for different modes
const int ACCURACY_CHASER_SPEED_EASY = 200;
const int ACCURACY_CHASER_SPEED_MEDIUM = 150;
const int ACCURACY_CHASER_SPEED_HARD = 100;

// New mode constants
const int MOVING_TARGET_INTERVAL = 900;    // ms between target moves (Medium)
const int FLASH_TARGET_ON_TIME = 400;      // ms target is visible (Hard)
const int FLASH_TARGET_OFF_TIME = 600;     // ms target is hidden (Hard)
const int DOUBLE_TARGET_TIMEOUT = 4000;    // ms for double target mode
const int SEQUENCE_SHOW_TIME = 800;        // ms to show each target in sequence

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

// === Moving Target (Medium Mode) Variables ===
static unsigned long last_target_move = 0;

// === Flash Target (Hard Mode) Variables ===
static bool flash_visible = true;
static unsigned long last_flash_toggle = 0;

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
static void move_target_medium();
static void flash_target_hard();
static void display_results();
static void game_over_menu_event_handler(lv_event_t *e);
static void back_to_menu_event_handler(lv_event_t *e);

void create_accuracy_trainer_screen()
{
    // Use base function to create common elements
    TrainerScreenElements elements = create_trainer_screen_base(back_to_menu_event_handler);
    
    // Assign to local static variables
    accuracy_screen = elements.screen;
    hud_label = elements.top_label;
    info_label = elements.info_label;
    results_label = elements.results_label;
    back_btn = elements.back_btn;

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
            // ЛЕГКИЙ: Мигаюча ціль (було HARD)
            lv_label_set_text(info_label, "Влуч у спалах!");
            
            target_led = random(NUM_LEDS);
            prev_target_led = target_led;
            flash_visible = true;
            
            strip_Clear();
            strip_SetPixelColor(target_led, RgbColor(255, 0, 255)); // Magenta
            strip_Show();
            
            last_flash_toggle = lv_tick_get();
            round_start_time = lv_tick_get();
        }
        else if (current_difficulty == ACCURACY_MEDIUM)
        {
            // СЕРЕДНІЙ: Рухома жовта ціль (без обмежень)
            lv_label_set_text(info_label, "Спіймай мету!");
            
            target_led = random(NUM_LEDS);
            prev_target_led = target_led;
            
            strip_Clear();
            strip_SetPixelColor(target_led, RgbColor(255, 255, 0)); // Yellow
            strip_Show();
            
            last_target_move = lv_tick_get();
            round_start_time = lv_tick_get();
        }
        else // ACCURACY_HARD
        {
            // ВАЖКИЙ: Статична синя ціль + жовтий chaser (було EASY)
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
        break;
    }

    case AT_STATE_WAIT_FOR_PRESS:
        // Таймер вже встановлений в AT_STATE_SHOW_TARGET
        break;

    case AT_STATE_FEEDBACK:
        // Feedback is handled in run_accuracy_trainer
        break;

    case AT_STATE_GAME_OVER:
        lv_label_set_text(info_label, "Гру завершено!");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        strip_Clear();
        strip_Show();
        break;

    case AT_STATE_SHOW_RESULTS:
        display_results();
        strip_Clear();
        strip_Show();
        break;

    case AT_STATE_GAME_OVER_MENU:
        display_results();
        strip_Clear();
        strip_Show();
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
                // ЛЕГКИЙ: Влучити коли ціль видима (мигає)
                if (i == target_led && flash_visible)
                {
                    correct_presses++;
                    Serial.printf("Acc(Easy): Hit! Button: %d\n", i);
                    feedback_success = true;
                }
                else
                {
                    misses++;
                    Serial.printf("Acc(Easy): Miss! Button: %d, Target: %d, Visible: %d\n", i, target_led, flash_visible);
                    feedback_success = false;
                }
            }
            else if (current_difficulty == ACCURACY_MEDIUM)
            {
                // СЕРЕДНІЙ: Влучити в рухому ціль
                if (i == target_led)
                {
                    correct_presses++;
                    Serial.printf("Acc(Medium): Hit! Button: %d\n", i);
                    feedback_success = true;
                }
                else
                {
                    misses++;
                    Serial.printf("Acc(Medium): Miss! Button: %d, Target: %d\n", i, target_led);
                    feedback_success = false;
                }
            }
            else // ACCURACY_HARD
            {
                // ВАЖКИЙ: Влучити в будь-який LED змійки (3 LED: центр і сусіди)
                bool hit_chaser = false;
                
                // Перевірка чи кнопка є частиною змійки (центр або ±1)
                if (i == chaser_position || 
                    i == chaser_position - 1 || 
                    i == chaser_position + 1)
                {
                    // Додаткова перевірка чи це також ціль
                    if (i == target_led)
                    {
                        hit_chaser = true;
                    }
                }
                
                if (hit_chaser)
                {
                    correct_presses++;
                    Serial.printf("Acc(Hard): Hit chaser! Button: %d, Chaser: %d\n", i, chaser_position);
                    feedback_success = true;
                }
                else
                {
                    misses++;
                    Serial.printf("Acc(Hard): Miss! Button: %d, Target: %d, Chaser: %d\n", i, target_led, chaser_position);
                    feedback_success = false;
                }
            }

            set_accuracy_trainer_state(AT_STATE_FEEDBACK);
            break;
        }
    }

    last_button_state = current_button_state;

    // Check for timeout (різний для кожного режиму)
    int current_timeout;
    if (current_difficulty == ACCURACY_EASY)
        current_timeout = ACCURACY_TIMEOUT_EASY;
    else if (current_difficulty == ACCURACY_MEDIUM)
        current_timeout = ACCURACY_TIMEOUT_MEDIUM;
    else
        current_timeout = ACCURACY_TIMEOUT_HARD;

    if (lv_tick_get() - round_start_time > (unsigned long)current_timeout)
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

static void move_target_medium()
{
    if (current_trainer_state != AT_STATE_SHOW_TARGET && current_trainer_state != AT_STATE_WAIT_FOR_PRESS)
        return;

    unsigned long now = lv_tick_get();
    if (now - last_target_move >= MOVING_TARGET_INTERVAL)
    {
        // Move target to new random position
        int old_target = target_led;
        int tries = 0;
        do
        {
            target_led = random(NUM_LEDS);
            tries++;
        } while (target_led == old_target && tries < 5);

        // Update display
        strip_Clear();
        strip_SetPixelColor(target_led, RgbColor(255, 255, 0)); // Yellow
        strip_Show();

        last_target_move = now;
    }
}

static void flash_target_hard()
{
    if (current_trainer_state != AT_STATE_SHOW_TARGET && current_trainer_state != AT_STATE_WAIT_FOR_PRESS)
        return;

    unsigned long now = lv_tick_get();
    unsigned long flash_interval = flash_visible ? FLASH_TARGET_ON_TIME : FLASH_TARGET_OFF_TIME;

    if (now - last_flash_toggle >= flash_interval)
    {
        flash_visible = !flash_visible;

        strip_Clear();
        if (flash_visible)
        {
            strip_SetPixelColor(target_led, RgbColor(255, 0, 255)); // Magenta
        }
        strip_Show();

        last_flash_toggle = now;
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
            flash_target_hard(); // EASY = мигаюча ціль
        }
        else if (current_difficulty == ACCURACY_MEDIUM)
        {
            move_target_medium(); // MEDIUM = рухома ціль
        }
        else if (current_difficulty == ACCURACY_HARD)
        {
            if (lv_tick_get() - state_timer > 50)
            {
                set_accuracy_trainer_state(AT_STATE_WAIT_FOR_PRESS);
            }
            move_chaser_easy(); // HARD = chaser
        }
        check_button_presses();
        break;

    case AT_STATE_WAIT_FOR_PRESS:
        if (current_difficulty == ACCURACY_EASY)
        {
            flash_target_hard(); // EASY = мигаюча ціль
        }
        else if (current_difficulty == ACCURACY_MEDIUM)
        {
            move_target_medium(); // MEDIUM = рухома ціль
        }
        else if (current_difficulty == ACCURACY_HARD)
        {
            move_chaser_easy(); // HARD = chaser
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
                // Логіка завершення залежить від складності
                bool should_end = false;
                
                if (current_difficulty == ACCURACY_MEDIUM)
                {
                    // СЕРЕДНІЙ: Гра до повної поразки (тільки по промахах)
                    should_end = (total_rounds >= MIN_ACCURACY_ROUNDS && misses >= MAX_ACCURACY_MISSES);
                }
                else
                {
                    // ЛЕГКИЙ/ВАЖКИЙ: Обмеження по раундах
                    bool max_rounds_reached = (total_rounds >= MAX_ACCURACY_ROUNDS);
                    bool min_rounds_with_max_misses = (total_rounds >= MIN_ACCURACY_ROUNDS && misses >= MAX_ACCURACY_MISSES);
                    should_end = (max_rounds_reached || min_rounds_with_max_misses);
                }
                
                if (should_end)
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
                // Після промаху - перевірка чи досягнуто ліміту
                bool min_rounds_with_max_misses = (total_rounds >= MIN_ACCURACY_ROUNDS && misses >= MAX_ACCURACY_MISSES);
                
                if (min_rounds_with_max_misses)
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
        strip_Clear();
        strip_Show();
        if (lv_tick_get() - state_timer > GAME_OVER_MSG_DURATION)
        {
            Serial.println("Transitioning to AT_STATE_SHOW_RESULTS");
            set_accuracy_trainer_state(AT_STATE_SHOW_RESULTS);
        }
        break;

    case AT_STATE_SHOW_RESULTS:
        strip_Clear();
        strip_Show();
        if (lv_tick_get() - state_timer > RESULTS_DISPLAY_DURATION)
        {
            set_accuracy_trainer_state(AT_STATE_GAME_OVER_MENU);
        }
        break;

    case AT_STATE_GAME_OVER_MENU:
        strip_Clear();
        strip_Show();
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

    lv_label_set_text(results_label, "РЕЗУЛЬТАТИ");
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