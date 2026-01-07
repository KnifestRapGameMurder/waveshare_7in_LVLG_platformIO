/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "app_screens.h"
#include "constants.h"
#include "globals.h"

// Extern declarations for trainer functions
extern void set_accuracy_easy_mode();
extern void set_accuracy_medium_mode();
extern void set_accuracy_hard_mode();
extern void create_accuracy_trainer_screen();
extern void create_reaction_trainer_screen();
extern void set_time_trial_state(TimeTrialState);
extern void set_coordination_easy_mode();
extern void set_coordination_hard_mode();
extern void create_coordination_trainer_screen();
extern void set_survival_duration_1_min();
extern void set_survival_duration_2_min();
extern void set_survival_duration_3_min();
extern void set_survival_time_state(SurvivalTimeState);
extern void create_back_button();
extern void create_dark_background();

// LVGL UI objects (defined here)
lv_obj_t *menu_buttons[4] = {NULL};
lv_obj_t *back_button = NULL;

lv_obj_t *debug_label = NULL;

// Тип тренажера для екрану історії
enum TrainerType {
    TRAINER_ACCURACY = 0,
    TRAINER_REACTION = 1,
    TRAINER_MEMORY = 2,
    TRAINER_COORDINATION = 3
};
static TrainerType selected_trainer = TRAINER_ACCURACY;

// Event handlers
static void accuracy_difficulty_event_cb(lv_event_t *e)
{
    if (isScreenTransitionActive()) return;
    int difficulty = (int)(intptr_t)lv_event_get_user_data(e);
    switch (difficulty)
    {
    case 0:
        set_accuracy_easy_mode();
        break;
    case 1:
        set_accuracy_medium_mode();
        break;
    case 2:
        set_accuracy_hard_mode();
        break;
    }
    current_state = STATE_ACCURACY_TRAINER;
    create_accuracy_trainer_screen();
}

static void reaction_mode_event_cb(lv_event_t *e)
{
    if (isScreenTransitionActive()) return;
    int mode = (int)(intptr_t)lv_event_get_user_data(e);
    if (mode == 0)
    {
        current_state = STATE_REACTION_TIME_TRIAL;
        create_reaction_trainer_screen();
        set_time_trial_state(TT_STATE_GET_READY);
    }
    else
    {
        current_state = STATE_REACTION_SURVIVAL_SUBMENU;
        create_reaction_survival_submenu();
    }
}

static void coordination_difficulty_event_cb(lv_event_t *e)
{
    if (isScreenTransitionActive()) return;
    int difficulty = (int)(intptr_t)lv_event_get_user_data(e);
    if (difficulty == 0)
        set_coordination_easy_mode();
    else
        set_coordination_hard_mode();
    current_state = STATE_COORDINATION_TRAINER;
    create_coordination_trainer_screen();
}

static void survival_duration_event_cb(lv_event_t *e)
{
    if (isScreenTransitionActive()) return;
    int duration = (int)(intptr_t)lv_event_get_user_data(e);
    switch (duration)
    {
    case 1:
        set_survival_duration_1_min();
        break;
    case 2:
        set_survival_duration_2_min();
        break;
    case 3:
        set_survival_duration_3_min();
        break;
    }
    current_state = STATE_REACTION_SURVIVAL;
    create_reaction_trainer_screen();
    set_survival_time_state(ST_STATE_GET_READY);
}

// Button event handler for main menu
static void menu_button_event_cb(lv_event_t *event)
{
    if (isScreenTransitionActive()) return;
    lv_obj_t *btn = lv_event_get_target(event);
    int trainer_id = (int)(intptr_t)lv_event_get_user_data(event);

    Serial.printf("[МЕНЮ] Натиснуто кнопку %d\n", trainer_id + 1);

    // Update interaction time to reset idle timeout (uses extern var)
    last_interaction_time = lv_tick_get();
    state_start_time = lv_tick_get();

    // Switch to selected trainer (uses extern var)
    switch (trainer_id)
    {
    case 0: // Accuracy Trainer
        current_state = STATE_ACCURACY_DIFFICULTY_SUBMENU;
        create_accuracy_difficulty_submenu();
        break;
    case 1: // Reaction Trainer
        current_state = STATE_REACTION_SUBMENU;
        create_reaction_submenu();
        break;
    case 2: // Memory Trainer
        current_state = STATE_MEMORY_TRAINER;
        create_memory_trainer_screen();
        break;
    case 3: // Coordination Trainer
        current_state = STATE_COORDINATION_SUBMENU;
        create_coordination_submenu();
        break;
    default:
        // Fallback to generic trainer screen
        create_trainer_screen(trainer_id);
        break;
    }
}

// Back button event handler
static void back_button_event_cb(lv_event_t *event)
{
    if (isScreenTransitionActive()) return;
    Serial.println("[НАЗАД] Натиснуто кнопку назад - повернення до головного меню");
    // Update interaction time to reset idle timeout (uses extern var)
    last_interaction_time = lv_tick_get();

    // Switch to main menu (uses extern var)
    current_state = STATE_MAIN_MENU;
    state_start_time = lv_tick_get();
    create_main_menu();
}

/**
 * @brief Handles touch events across the application.
 * Currently only used to transition from STATE_LOADING to STATE_MAIN_MENU.
 */
void app_screen_touch_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    
    // Відстежуємо стан дотику для захисту від фантомних кліків
    if (code == LV_EVENT_PRESSED) {
        markTouchPressed();
    }
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        markTouchReleased();
    }
    
    if (isScreenTransitionActive()) return;

    if (current_state == STATE_LOADING)
    {
        // Для екрану завантаження реагуємо на RELEASED (після зняття пальця)
        if (code == LV_EVENT_RELEASED)
        {
            Serial.println("[ДОТИК] Перехід від завантаження до вибору пацієнта");
            current_state = STATE_PATIENT_SELECT;
            state_start_time = lv_tick_get();
            last_interaction_time = lv_tick_get();
            create_patient_select_screen();
        }
    }
    else
    {
        // Only track interaction time for non-loading states
        last_interaction_time = lv_tick_get();
    }
}

// Create main menu with 4 trainer buttons taking full screen
void create_main_menu()
{
    Serial.println("[НАЛАГОДЖЕННЯ] Створення головного меню...");
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    // Create 4 trainer buttons taking all screen space in 2x2 grid
    const char *trainer_names[] = {
        "ВЛУЧНІСТЬ",  // Accuracy Trainer
        "РЕАКЦІЯ",    // Reaction Trainer
        "ПАМ'ЯТЬ",    // Memory Trainer
        "КООРДИНАЦІЯ" // Coordination Trainer
    };

    // Different colors for each button
    uint32_t button_colors[] = {
        COLOR_MENU_ACCURACY,     // Gold for Accuracy
        COLOR_MENU_REACTION,     // Dark Turquoise for Reaction
        COLOR_MENU_MEMORY,       // Medium Purple for Memory
        COLOR_MENU_COORDINATION  // Lime Green for Coordination
    };

    // Each button takes half of screen width and height (uses extern vars)
    int btn_width = SCR_W / 2;
    int btn_height = SCR_H / 2;

    for (int i = 0; i < 4; i++)
    {
        int row = i / 2;
        int col = i % 2;

        menu_buttons[i] = lv_btn_create(lv_scr_act());
        lv_obj_set_size(menu_buttons[i], btn_width, btn_height);

        // Position buttons in corners
        int x_pos = col * btn_width;
        int y_pos = row * btn_height;
        lv_obj_set_pos(menu_buttons[i], x_pos, y_pos);

        // Button styling with different colors
        lv_obj_set_style_bg_color(menu_buttons[i], lv_color_hex(button_colors[i]), 0);
        lv_obj_set_style_bg_color(menu_buttons[i], lv_color_hex(button_colors[i] + COLOR_MENU_PRESSED_OFFSET), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(menu_buttons[i], lv_color_white(), 0);
        lv_obj_set_style_border_width(menu_buttons[i], 3, 0);
        lv_obj_set_style_radius(menu_buttons[i], 0, 0); // Square corners

        // Button label
        lv_obj_t *label = lv_label_create(menu_buttons[i]);
        lv_label_set_text(label, trainer_names[i]);
        // NOTE: Commenting out font style since 'minecraft_ten_48' is not defined here.
        lv_obj_set_style_text_font(label, Font2, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_center(label);

        // Add event handler - реагуємо на відпускання пальця
        lv_obj_add_event_cb(menu_buttons[i], menu_button_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
    }

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Create trainer screen
void create_trainer_screen(int trainer_id)
{
    Serial.printf("[НАЛАГОДЖЕННЯ] Створення екрану тренажера %d...\n", trainer_id + 1);
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    // Create dark background
    create_dark_background();

    // Title
    char title_text[32];
    snprintf(title_text, sizeof(title_text), "ТРЕНАЖЕР %d", trainer_id + 1);
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Placeholder content - THIS IS WHERE YOU WILL CALL THE UNIQUE TRAINER FUNCTION LATER
    lv_obj_t *content = lv_label_create(lv_scr_act());
    lv_label_set_text(content, "Тут буде вміст тренажера (Trainer Specific Logic Goes Here)");
    lv_obj_set_style_text_font(content, Font2, 0);
    lv_obj_set_style_text_color(content, lv_color_hex(COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);

    // Back button
    create_back_button();

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_RELEASED, NULL);
}

// Create accuracy difficulty submenu
void create_accuracy_difficulty_submenu()
{
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    // Create dark background
    create_dark_background();

    // Title
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "РІВЕНЬ СКЛАДНОСТІ");
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Easy button
    lv_obj_t *easy_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(easy_btn, 300, 80);
    lv_obj_align(easy_btn, LV_ALIGN_CENTER, 0, -120);
    lv_obj_set_style_bg_color(easy_btn, lv_color_hex(COLOR_BTN_GREEN), 0);
    lv_obj_set_style_bg_color(easy_btn, lv_color_hex(COLOR_BTN_GREEN_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *easy_label = lv_label_create(easy_btn);
    lv_label_set_text(easy_label, "ЛЕГКИЙ");
    lv_obj_set_style_text_font(easy_label, Font2, 0);
    lv_obj_center(easy_label);

    lv_obj_add_event_cb(easy_btn, accuracy_difficulty_event_cb, LV_EVENT_RELEASED, (void *)0);

    // Medium button
    lv_obj_t *medium_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(medium_btn, 300, 80);
    lv_obj_align(medium_btn, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(medium_btn, lv_color_hex(COLOR_BTN_YELLOW), 0);
    lv_obj_set_style_bg_color(medium_btn, lv_color_hex(COLOR_BTN_YELLOW_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *medium_label = lv_label_create(medium_btn);
    lv_label_set_text(medium_label, "СЕРЕДНІЙ");
    lv_obj_set_style_text_font(medium_label, Font2, 0);
    lv_obj_center(medium_label);

    lv_obj_add_event_cb(medium_btn, accuracy_difficulty_event_cb, LV_EVENT_RELEASED, (void *)1);

    // Hard button
    lv_obj_t *hard_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(hard_btn, 300, 80);
    lv_obj_align(hard_btn, LV_ALIGN_CENTER, 0, 80);
    lv_obj_set_style_bg_color(hard_btn, lv_color_hex(COLOR_BTN_RED), 0);
    lv_obj_set_style_bg_color(hard_btn, lv_color_hex(COLOR_BTN_RED_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *hard_label = lv_label_create(hard_btn);
    lv_label_set_text(hard_label, "ВАЖКИЙ");
    lv_obj_set_style_text_font(hard_label, Font2, 0);
    lv_obj_center(hard_label);

    lv_obj_add_event_cb(hard_btn, accuracy_difficulty_event_cb, LV_EVENT_RELEASED, (void *)2);

    // Back button
    create_back_button();

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_RELEASED, NULL);

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Create reaction submenu
void create_reaction_submenu()
{
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    // Create dark background
    create_dark_background();

    // Title
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "ВИБЕРІТЬ РЕЖИМ");
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Time Trial button
    lv_obj_t *trial_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(trial_btn, 300, 80);
    lv_obj_align(trial_btn, LV_ALIGN_CENTER, 0, -100);
    lv_obj_set_style_bg_color(trial_btn, lv_color_hex(COLOR_BTN_CYAN), 0);
    lv_obj_set_style_bg_color(trial_btn, lv_color_hex(COLOR_BTN_CYAN_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *trial_label = lv_label_create(trial_btn);
    lv_label_set_text(trial_label, "ЧАС РЕАКЦІЇ");
    lv_obj_set_style_text_font(trial_label, Font2, 0);
    lv_obj_center(trial_label);

    lv_obj_add_event_cb(trial_btn, reaction_mode_event_cb, LV_EVENT_RELEASED, (void *)0);

    // Survival button
    lv_obj_t *survival_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(survival_btn, 300, 80);
    lv_obj_align(survival_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(survival_btn, lv_color_hex(COLOR_BTN_ORANGE), 0);
    lv_obj_set_style_bg_color(survival_btn, lv_color_hex(COLOR_BTN_ORANGE_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *survival_label = lv_label_create(survival_btn);
    lv_label_set_text(survival_label, "ВИЖИВАННЯ");
    lv_obj_set_style_text_font(survival_label, Font2, 0);
    lv_obj_center(survival_label);

    lv_obj_add_event_cb(survival_btn, reaction_mode_event_cb, LV_EVENT_RELEASED, (void *)1);

    // Back button
    create_back_button();

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_RELEASED, NULL);

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Create coordination submenu
void create_coordination_submenu()
{
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    // Create dark background
    create_dark_background();

    // Title
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "РІВЕНЬ СКЛАДНОСТІ");
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Easy button
    lv_obj_t *easy_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(easy_btn, 300, 80);
    lv_obj_align(easy_btn, LV_ALIGN_CENTER, 0, -70);
    lv_obj_set_style_bg_color(easy_btn, lv_color_hex(COLOR_BTN_GREEN), 0);
    lv_obj_set_style_bg_color(easy_btn, lv_color_hex(COLOR_BTN_GREEN_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *easy_label = lv_label_create(easy_btn);
    lv_label_set_text(easy_label, "ЛЕГКИЙ");
    lv_obj_set_style_text_font(easy_label, Font2, 0);
    lv_obj_center(easy_label);

    lv_obj_add_event_cb(easy_btn, coordination_difficulty_event_cb, LV_EVENT_RELEASED, (void *)0);

    // Hard button
    lv_obj_t *hard_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(hard_btn, 300, 80);
    lv_obj_align(hard_btn, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(hard_btn, lv_color_hex(COLOR_BTN_RED), 0);
    lv_obj_set_style_bg_color(hard_btn, lv_color_hex(COLOR_BTN_RED_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *hard_label = lv_label_create(hard_btn);
    lv_label_set_text(hard_label, "ВАЖКИЙ");
    lv_obj_set_style_text_font(hard_label, Font2, 0);
    lv_obj_center(hard_label);

    lv_obj_add_event_cb(hard_btn, coordination_difficulty_event_cb, LV_EVENT_RELEASED, (void *)1);

    // Back button
    create_back_button();

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_RELEASED, NULL);

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Create reaction survival submenu
void create_reaction_survival_submenu()
{
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    // Create dark background
    create_dark_background();

    // Title
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "ВИБЕРІТЬ ТРИВАЛІСТЬ");
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // 1 minute button
    lv_obj_t *min1_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(min1_btn, 300, 80);
    lv_obj_align(min1_btn, LV_ALIGN_CENTER, 0, -100);
    lv_obj_set_style_bg_color(min1_btn, lv_color_hex(COLOR_BTN_GREEN), 0);
    lv_obj_set_style_bg_color(min1_btn, lv_color_hex(COLOR_BTN_GREEN_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *min1_label = lv_label_create(min1_btn);
    lv_label_set_text(min1_label, "1 ХВИЛИНА");
    lv_obj_set_style_text_font(min1_label, Font2, 0);
    lv_obj_center(min1_label);

    lv_obj_add_event_cb(min1_btn, survival_duration_event_cb, LV_EVENT_RELEASED, (void *)1);

    // 2 minutes button
    lv_obj_t *min2_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(min2_btn, 300, 80);
    lv_obj_align(min2_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(min2_btn, lv_color_hex(COLOR_BTN_YELLOW), 0);
    lv_obj_set_style_bg_color(min2_btn, lv_color_hex(COLOR_BTN_YELLOW_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *min2_label = lv_label_create(min2_btn);
    lv_label_set_text(min2_label, "2 ХВИЛИНИ");
    lv_obj_set_style_text_font(min2_label, Font2, 0);
    lv_obj_center(min2_label);

    lv_obj_add_event_cb(min2_btn, survival_duration_event_cb, LV_EVENT_RELEASED, (void *)2);

    // 3 minutes button
    lv_obj_t *min3_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(min3_btn, 300, 80);
    lv_obj_align(min3_btn, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_bg_color(min3_btn, lv_color_hex(COLOR_BTN_RED), 0);
    lv_obj_set_style_bg_color(min3_btn, lv_color_hex(COLOR_BTN_RED_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *min3_label = lv_label_create(min3_btn);
    lv_label_set_text(min3_label, "3 ХВИЛИНИ");
    lv_obj_set_style_text_font(min3_label, Font2, 0);
    lv_obj_center(min3_label);

    lv_obj_add_event_cb(min3_btn, survival_duration_event_cb, LV_EVENT_RELEASED, (void *)3);

    // Back button
    create_back_button();

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_RELEASED, NULL);

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Function to create/update the debug label for button states
void create_debug_label()
{
    return;

    // Create/update debug label for button states
    if (debug_label)
        lv_obj_del(debug_label);

    debug_label = lv_label_create(lv_scr_act());
    lv_label_set_text(debug_label, "0000000000000000");
    lv_obj_align(debug_label, LV_ALIGN_TOP_LEFT, 10, 10); // Top-left with small offset
    lv_obj_set_style_text_color(debug_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(debug_label, Font3, 0); // Use small font

    // Remove background and border to prevent glitching
    lv_obj_set_style_bg_opa(debug_label, LV_OPA_0, 0); // Fully transparent background
    lv_obj_set_style_border_width(debug_label, 0, 0);  // No border
}

void create_back_button()
{
    back_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_button, 200, 80);
    lv_obj_align(back_button, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(COLOR_BTN_BACK), 0);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(COLOR_BTN_BACK_PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(back_button, lv_color_white(), 0);
    lv_obj_set_style_border_width(back_button, 2, 0);
}

void create_dark_background()
{
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(COLOR_BG_DARK), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
}

TrainerScreenElements create_trainer_screen_base(lv_event_cb_t back_event_cb)
{
    TrainerScreenElements elements;

    // Clean the screen
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    // Create main screen container
    elements.screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(elements.screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(elements.screen, lv_color_hex(COLOR_BG_DARK), 0);
    lv_obj_clear_flag(elements.screen, LV_OBJ_FLAG_SCROLLABLE);

    // Create top label (for round/level/hud display)
    elements.top_label = lv_label_create(elements.screen);
    lv_obj_set_style_text_font(elements.top_label, Font2, 0);
    lv_obj_set_style_text_color(elements.top_label, lv_color_white(), 0);
    lv_obj_align(elements.top_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create info label (center)
    elements.info_label = lv_label_create(elements.screen);
    lv_obj_set_style_text_font(elements.info_label, Font2, 0);
    lv_obj_set_style_text_color(elements.info_label, lv_color_white(), 0);
    lv_obj_align(elements.info_label, LV_ALIGN_CENTER, 0, 0);

    // Create results label (hidden initially)
    elements.results_label = lv_label_create(elements.screen);
    lv_obj_set_style_text_font(elements.results_label, Font2, 0);
    lv_obj_set_style_text_color(elements.results_label, lv_color_white(), 0);
    lv_obj_align(elements.results_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(elements.results_label, LV_OBJ_FLAG_HIDDEN);

    // Create back button
    elements.back_btn = lv_btn_create(elements.screen);
    lv_obj_set_size(elements.back_btn, 200, 80);
    lv_obj_align(elements.back_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(elements.back_btn, lv_color_hex(COLOR_BTN_BACK), 0);
    lv_obj_set_style_bg_color(elements.back_btn, lv_color_hex(COLOR_BTN_BACK_PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(elements.back_btn, lv_color_white(), 0);
    lv_obj_set_style_border_width(elements.back_btn, 2, 0);

    lv_obj_t *back_label = lv_label_create(elements.back_btn);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(elements.back_btn, back_event_cb, LV_EVENT_RELEASED, NULL);

    return elements;
}

GameOverMenuElements create_game_over_menu(lv_obj_t *parent, lv_obj_t *info_lbl, 
                                           lv_obj_t *results_lbl, lv_event_cb_t event_cb)
{
    GameOverMenuElements elements;

    // Hide labels if provided
    if (info_lbl != NULL)
    {
        lv_obj_add_flag(info_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    if (results_lbl != NULL)
    {
        lv_obj_add_flag(results_lbl, LV_OBJ_FLAG_HIDDEN);
    }

    // Create play again button
    elements.play_again_btn = lv_btn_create(parent);
    lv_obj_set_size(elements.play_again_btn, 300, 80);
    lv_obj_align(elements.play_again_btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(elements.play_again_btn, lv_color_hex(COLOR_BTN_GREEN), 0);
    lv_obj_set_style_bg_color(elements.play_again_btn, lv_color_hex(COLOR_BTN_GREEN_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *play_label = lv_label_create(elements.play_again_btn);
    lv_label_set_text(play_label, "Грати Знову");
    lv_obj_set_style_text_font(play_label, Font2, 0);
    lv_obj_center(play_label);

    lv_obj_add_event_cb(elements.play_again_btn, event_cb, LV_EVENT_RELEASED, (void *)0);

    // Create exit button
    elements.exit_btn = lv_btn_create(parent);
    lv_obj_set_size(elements.exit_btn, 300, 80);
    lv_obj_align(elements.exit_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(elements.exit_btn, lv_color_hex(COLOR_BTN_RED), 0);
    lv_obj_set_style_bg_color(elements.exit_btn, lv_color_hex(COLOR_BTN_RED_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *exit_label = lv_label_create(elements.exit_btn);
    lv_label_set_text(exit_label, "Вихід");
    lv_obj_set_style_text_font(exit_label, Font2, 0);
    lv_obj_center(exit_label);

    lv_obj_add_event_cb(elements.exit_btn, event_cb, LV_EVENT_RELEASED, (void *)1);

    return elements;
}

// ============================================
// === ЕКРАН ВИБОРУ ПАЦІЄНТА ===
// ============================================

// Прапорець для ігнорування RELEASED після LONG_PRESSED
static bool patient_long_press_triggered = false;

// Event handler для вибору пацієнта
static void patient_select_event_cb(lv_event_t *e)
{
    // Ігноруємо RELEASED якщо це йде після довгого натискання
    if (patient_long_press_triggered) {
        patient_long_press_triggered = false;
        Serial.println("[ПОДІЯ] patient_select_event_cb - ІГНОРОВАНО (після довгого натискання)");
        return;
    }
    
    // Захист від фантомних кліків при переході екранів
    if (isScreenTransitionActive()) {
        Serial.println("[ПОДІЯ] patient_select_event_cb - ІГНОРОВАНО (захист переходу)");
        return;
    }
    Serial.println("[ПОДІЯ] patient_select_event_cb ПОЧАТОК");
    int patient_id = (int)(intptr_t)lv_event_get_user_data(e);
    currentPatientIndex = patient_id;
    
    Serial.printf("[ПАЦІЄНТ] Обрано пацієнта %d - переходимо до головного меню\n", patient_id);
    
    // Переходимо до головного меню
    current_state = STATE_MAIN_MENU;
    state_start_time = lv_tick_get();
    last_interaction_time = lv_tick_get();
    create_main_menu();
    Serial.println("[ПОДІЯ] patient_select_event_cb КІНЕЦЬ");
}

// Асинхронний callback для відкриття статистики (щоб уникнути проблем з видаленням об'єкта під час події)
static void open_stats_async(void *user_data)
{
    Serial.println("[ASYNC] open_stats_async - СТВОРЕННЯ ЕКРАНУ СТАТИСТИКИ");
    create_patient_stats_screen();
    Serial.println("[ASYNC] open_stats_async - ЗАВЕРШЕНО");
}

// Event handler для перегляду статистики (довге натискання)
static void patient_stats_event_cb(lv_event_t *e)
{
    // Захист від фантомних кліків при переході екранів
    if (isScreenTransitionActive()) {
        Serial.println("[ПОДІЯ] patient_stats_event_cb - ІГНОРОВАНО (захист переходу)");
        return;
    }
    
    // Встановлюємо прапорець щоб ігнорувати наступний RELEASED
    patient_long_press_triggered = true;
    
    // ВАЖЛИВО: Викликаємо markScreenTransition() ОДРАЗУ, до асинхронного виклику!
    // Це захистить від фантомних кліків коли палець буде відпущений
    markScreenTransition();
    
    Serial.println("[ПОДІЯ] patient_stats_event_cb - ДОВГЕ НАТИСКАННЯ ПОЧАТОК");
    
    int patient_id = (int)(intptr_t)lv_event_get_user_data(e);
    currentPatientIndex = patient_id;
    
    Serial.printf("[ПАЦІЄНТ] Перегляд статистики пацієнта %d\n", patient_id);
    
    current_state = STATE_PATIENT_STATS;
    state_start_time = lv_tick_get();
    last_interaction_time = lv_tick_get();
    
    // Використовуємо асинхронний виклик, щоб уникнути проблем з видаленням об'єкта під час події
    lv_async_call(open_stats_async, NULL);
    
    Serial.println("[ПОДІЯ] patient_stats_event_cb - ДОВГЕ НАТИСКАННЯ КІНЕЦЬ");
}

// Event handler для повернення з екрану статистики
static void stats_back_event_cb(lv_event_t *e)
{
    // Захист від фантомних кліків
    if (isScreenTransitionActive()) return;
    Serial.println("[ПОДІЯ] stats_back_event_cb - НАТИСНУТО НАЗАД");
    current_state = STATE_PATIENT_SELECT;
    state_start_time = lv_tick_get();
    last_interaction_time = lv_tick_get();
    create_patient_select_screen();
}

// Event handler для очищення статистики
static void clear_stats_event_cb(lv_event_t *e)
{
    if (isScreenTransitionActive()) return;
    clearPatientStats(currentPatientIndex);
    create_patient_stats_screen();  // Оновлюємо екран
}

// Створення екрану вибору пацієнта
void create_patient_select_screen()
{
    Serial.printf("[НАЛАГОДЖЕННЯ] Створення екрану вибору пацієнта... (current_state=%d)\n", (int)current_state);
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    // Фон
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a2e), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bg, 0, 0);

    // Заголовок
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "ОБЕРІТЬ ПАЦІЄНТА");
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Контейнер для кнопок з прокруткою
    lv_obj_t *container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(container, SCR_W - 20, SCR_H - 80);
    lv_obj_align(container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x16213e), 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);

    // Розмір кнопки (4 колонки)
    int btn_width = (SCR_W - 80) / 4;
    int btn_height = 90;  // Збільшена висота без окремої кнопки статистики

    // Кольори для різних типів
    uint32_t guest_color = 0x9b59b6;      // Фіолетовий для гостя
    uint32_t patient_color = 0x3498db;    // Синій для пацієнтів

    // Створюємо кнопки для всіх 16 пацієнтів (0 = гість, 1-15 = пацієнти)
    for (int i = 0; i < PATIENT_COUNT; i++)
    {
        // Кнопка вибору пацієнта (коротке натискання - вибір, довге - статистика)
        lv_obj_t *btn = lv_btn_create(container);
        lv_obj_set_size(btn, btn_width - 10, btn_height);
        
        uint32_t color = (i == 0) ? guest_color : patient_color;
        lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(color - 0x222222), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, lv_color_white(), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_radius(btn, 10, 0);

        // Текст кнопки
        lv_obj_t *label = lv_label_create(btn);
        if (i == 0)
        {
            lv_label_set_text(label, "ГІСТЬ");
        }
        else
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", i);
            lv_label_set_text(label, buf);
        }
        lv_obj_set_style_text_font(label, Font3, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_center(label);

        // Коротке натискання - вибір пацієнта (при відпусканні пальця)
        lv_obj_add_event_cb(btn, patient_select_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)i);
        // Довге натискання - відкриття статистики
        lv_obj_add_event_cb(btn, patient_stats_event_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)i);
    }
}

// ============================================
// === ЕКРАН ІСТОРІЇ СЕСІЙ ТРЕНАЖЕРА ===
// ============================================

// Forward declaration
void create_patient_stats_screen();

// Обробник повернення з історії до статистики
static void history_back_event_cb(lv_event_t *e)
{
    // Захист від фантомних кліків
    if (isScreenTransitionActive()) return;
    Serial.println("[HISTORY] Повернення до статистики");
    create_patient_stats_screen();
}

// Створення екрану історії сесій
void create_session_history_screen()
{
    Serial.printf("[HISTORY] Створення екрану історії для тренажера %d\n", (int)selected_trainer);
    lv_obj_clean(lv_scr_act());
    markScreenTransition();

    PatientStats *stats = &patientStats[currentPatientIndex];
    
    // Фон
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a2e), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

    // Визначаємо назву та колір тренажера
    const char *trainer_name = "";
    uint32_t trainer_color = 0x3498db;
    TrainerHistory *history = NULL;
    
    switch (selected_trainer) {
        case TRAINER_ACCURACY:
            trainer_name = "ВЛУЧНІСТЬ";
            trainer_color = 0x3498db;
            history = &stats->accuracy_history;
            break;
        case TRAINER_REACTION:
            trainer_name = "РЕАКЦІЯ";
            trainer_color = 0x2ecc71;
            history = &stats->reaction_history;
            break;
        case TRAINER_MEMORY:
            trainer_name = "ПАМ'ЯТЬ";
            trainer_color = 0xe74c3c;
            history = &stats->memory_history;
            break;
        case TRAINER_COORDINATION:
            trainer_name = "КООРДИНАЦІЯ";
            trainer_color = 0x9b59b6;
            history = &stats->coordination_history;
            break;
    }

    // Заголовок
    lv_obj_t *title = lv_label_create(lv_scr_act());
    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), "%s - ІСТОРІЯ", trainer_name);
    lv_label_set_text(title, title_buf);
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(trainer_color), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Контейнер для списку сесій
    lv_obj_t *list_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(list_container, SCR_W - 60, SCR_H - 130);
    lv_obj_align(list_container, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0x16213e), 0);
    lv_obj_set_style_border_width(list_container, 2, 0);
    lv_obj_set_style_border_color(list_container, lv_color_hex(trainer_color), 0);
    lv_obj_set_style_pad_all(list_container, 15, 0);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list_container, LV_DIR_VER);

    if (history == NULL || history->count == 0) {
        // Немає даних
        lv_obj_t *no_data = lv_label_create(list_container);
        lv_label_set_text(no_data, "Немає записаних сесій");
        lv_obj_set_style_text_font(no_data, Font3, 0);
        lv_obj_set_style_text_color(no_data, lv_color_hex(0x888888), 0);
    } else {
        // Заголовок таблиці
        lv_obj_t *header = lv_label_create(list_container);
        if (selected_trainer == TRAINER_REACTION) {
            lv_label_set_text(header, "#   | РЕЗУЛЬТАТ (мс) | ДОДАТКОВО");
        } else if (selected_trainer == TRAINER_MEMORY) {
            lv_label_set_text(header, "#   | РІВЕНЬ | ПРАВИЛЬНИХ");
        } else {
            lv_label_set_text(header, "#   | РЕЗУЛЬТАТ | ДОДАТКОВО");
        }
        lv_obj_set_style_text_font(header, Font3, 0);
        lv_obj_set_style_text_color(header, lv_color_hex(trainer_color), 0);
        
        // Роздільник
        lv_obj_t *sep = lv_label_create(list_container);
        lv_label_set_text(sep, "-------------------------------");
        lv_obj_set_style_text_font(sep, Font3, 0);
        lv_obj_set_style_text_color(sep, lv_color_hex(0x555555), 0);

        // Показуємо записи від найновішого до найстарішого
        char line_buf[64];
        int displayed = 0;
        
        for (int i = 0; i < history->count && displayed < SESSION_HISTORY_SIZE; i++) {
            // Обчислюємо індекс (від найновішого)
            int idx = (history->next_index - 1 - i + SESSION_HISTORY_SIZE) % SESSION_HISTORY_SIZE;
            if (idx < 0) idx += SESSION_HISTORY_SIZE;
            
            SessionRecord *rec = &history->sessions[idx];
            
            lv_obj_t *row = lv_label_create(list_container);
            snprintf(line_buf, sizeof(line_buf), "%2d  | %6d | %6d", 
                displayed + 1, rec->score, rec->extra);
            lv_label_set_text(row, line_buf);
            lv_obj_set_style_text_font(row, Font3, 0);
            lv_obj_set_style_text_color(row, lv_color_white(), 0);
            
            displayed++;
        }
    }

    // Кнопка назад
    lv_obj_t *back_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_btn, 200, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLOR_BTN_BACK), 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLOR_BTN_BACK_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *back_label_txt = lv_label_create(back_btn);
    lv_label_set_text(back_label_txt, "НАЗАД");
    lv_obj_set_style_text_font(back_label_txt, Font3, 0);
    lv_obj_set_style_text_color(back_label_txt, lv_color_white(), 0);
    lv_obj_center(back_label_txt);

    lv_obj_add_event_cb(back_btn, history_back_event_cb, LV_EVENT_RELEASED, NULL);
}

// Обробник кліку по назві тренажера
static void trainer_click_event_cb(lv_event_t *e)
{
    // Захист від фантомних кліків при переході екранів
    if (isScreenTransitionActive()) {
        Serial.println("[STATS] trainer_click - ІГНОРОВАНО (захист переходу)");
        return;
    }
    TrainerType type = (TrainerType)(intptr_t)lv_event_get_user_data(e);
    selected_trainer = type;
    Serial.printf("[STATS] Клік по тренажеру: %d\n", (int)type);
    create_session_history_screen();
}

// ============================================
// === ЕКРАН СТАТИСТИКИ ПАЦІЄНТА ===
// ============================================

void create_patient_stats_screen()
{
    Serial.println("[STATS_SCR] Початок create_patient_stats_screen");
    Serial.printf("[STATS_SCR] currentPatientIndex = %d\n", currentPatientIndex);
    
    Serial.println("[STATS_SCR] Очищаємо екран...");
    lv_obj_clean(lv_scr_act());
    markScreenTransition();
    Serial.println("[STATS_SCR] Екран очищено");

    // Фон
    Serial.println("[STATS_SCR] Створюємо фон...");
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a2e), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    Serial.println("[STATS_SCR] Фон створено");

    Serial.println("[STATS_SCR] Отримуємо статистику...");
    PatientStats *stats = &patientStats[currentPatientIndex];
    Serial.println("[STATS_SCR] Статистику отримано");

    // Заголовок
    Serial.println("[STATS_SCR] Створюємо заголовок...");
    lv_obj_t *title = lv_label_create(lv_scr_act());
    Serial.println("[STATS_SCR] Label створено");
    
    char title_buf[48];
    if (currentPatientIndex == 0)
    {
        snprintf(title_buf, sizeof(title_buf), "ГІСТЬ");
    }
    else
    {
        snprintf(title_buf, sizeof(title_buf), "ПАЦІЄНТ %d", currentPatientIndex);
    }
    Serial.printf("[STATS_SCR] title_buf = %s\n", title_buf);
    
    lv_label_set_text(title, title_buf);
    Serial.println("[STATS_SCR] Текст заголовка встановлено");
    
    Serial.printf("[STATS_SCR] Font2 = %p\n", (void*)Font2);
    if (Font2 != NULL) {
        lv_obj_set_style_text_font(title, Font2, 0);
        Serial.println("[STATS_SCR] Шрифт заголовка встановлено");
    } else {
        Serial.println("[STATS_SCR] УВАГА: Font2 = NULL!");
    }
    
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    Serial.println("[STATS_SCR] Заголовок створено");

    // Контейнер для статистики
    Serial.println("[STATS_SCR] Створюємо контейнер...");
    lv_obj_t *stats_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(stats_container, SCR_W - 60, SCR_H - 100);
    lv_obj_align(stats_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(stats_container, lv_color_hex(0x16213e), 0);
    lv_obj_set_style_border_width(stats_container, 2, 0);
    lv_obj_set_style_border_color(stats_container, lv_color_hex(0x3498db), 0);
    lv_obj_set_style_pad_all(stats_container, 15, 0);
    lv_obj_set_flex_flow(stats_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(stats_container, LV_DIR_VER);
    Serial.println("[STATS_SCR] Контейнер створено");

    // Створюємо статистику окремими лейблами (для стабільності з Font3)
    Serial.println("[STATS_SCR] Формуємо статистику...");
    char line_buf[128];
    lv_obj_t *lbl;
    lv_obj_t *trainer_btn;
    lv_obj_t *trainer_lbl;
    
    // Влучність
    int accuracy_pct = (stats->accuracy_total_hits + stats->accuracy_total_misses > 0) 
        ? (stats->accuracy_total_hits * 100) / (stats->accuracy_total_hits + stats->accuracy_total_misses) : 0;
    
    // Середній час реакції
    int avg_reaction = (stats->reaction_avg_count > 0) 
        ? stats->reaction_avg_time_sum / stats->reaction_avg_count : 0;

    // --- ВЛУЧНІСТЬ --- (клікабельна кнопка)
    trainer_btn = lv_btn_create(stats_container);
    lv_obj_set_size(trainer_btn, LV_PCT(100), 30);
    lv_obj_set_style_bg_color(trainer_btn, lv_color_hex(0x3498db), 0);
    lv_obj_set_style_bg_color(trainer_btn, lv_color_hex(0x2980b9), LV_STATE_PRESSED);
    lv_obj_set_style_radius(trainer_btn, 5, 0);
    trainer_lbl = lv_label_create(trainer_btn);
    lv_label_set_text(trainer_lbl, "ВЛУЧНІСТЬ >");
    lv_obj_set_style_text_font(trainer_lbl, Font3, 0);
    lv_obj_set_style_text_color(trainer_lbl, lv_color_white(), 0);
    lv_obj_center(trainer_lbl);
    lv_obj_add_event_cb(trainer_btn, trainer_click_event_cb, LV_EVENT_RELEASED, (void*)(intptr_t)TRAINER_ACCURACY);
    
    lbl = lv_label_create(stats_container);
    snprintf(line_buf, sizeof(line_buf), "Сесій: %d  Влучень: %d  Промахів: %d", 
        stats->accuracy_sessions, stats->accuracy_total_hits, stats->accuracy_total_misses);
    lv_label_set_text(lbl, line_buf);
    lv_obj_set_style_text_font(lbl, Font3, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    
    lbl = lv_label_create(stats_container);
    snprintf(line_buf, sizeof(line_buf), "Точність: %d%%  Рекорд: %d", accuracy_pct, stats->accuracy_best_score);
    lv_label_set_text(lbl, line_buf);
    lv_obj_set_style_text_font(lbl, Font3, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

    // --- РЕАКЦІЯ --- (клікабельна кнопка)
    trainer_btn = lv_btn_create(stats_container);
    lv_obj_set_size(trainer_btn, LV_PCT(100), 30);
    lv_obj_set_style_bg_color(trainer_btn, lv_color_hex(0x2ecc71), 0);
    lv_obj_set_style_bg_color(trainer_btn, lv_color_hex(0x27ae60), LV_STATE_PRESSED);
    lv_obj_set_style_radius(trainer_btn, 5, 0);
    trainer_lbl = lv_label_create(trainer_btn);
    lv_label_set_text(trainer_lbl, "РЕАКЦІЯ >");
    lv_obj_set_style_text_font(trainer_lbl, Font3, 0);
    lv_obj_set_style_text_color(trainer_lbl, lv_color_white(), 0);
    lv_obj_center(trainer_lbl);
    lv_obj_add_event_cb(trainer_btn, trainer_click_event_cb, LV_EVENT_RELEASED, (void*)(intptr_t)TRAINER_REACTION);
    
    lbl = lv_label_create(stats_container);
    snprintf(line_buf, sizeof(line_buf), "Сесій: %d  Рекорд: %d мс  Середнє: %d мс", 
        stats->reaction_sessions, stats->reaction_best_time_ms, avg_reaction);
    lv_label_set_text(lbl, line_buf);
    lv_obj_set_style_text_font(lbl, Font3, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

    // --- ПАМ'ЯТЬ --- (клікабельна кнопка)
    trainer_btn = lv_btn_create(stats_container);
    lv_obj_set_size(trainer_btn, LV_PCT(100), 30);
    lv_obj_set_style_bg_color(trainer_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_set_style_bg_color(trainer_btn, lv_color_hex(0xc0392b), LV_STATE_PRESSED);
    lv_obj_set_style_radius(trainer_btn, 5, 0);
    trainer_lbl = lv_label_create(trainer_btn);
    lv_label_set_text(trainer_lbl, "ПАМ'ЯТЬ >");
    lv_obj_set_style_text_font(trainer_lbl, Font3, 0);
    lv_obj_set_style_text_color(trainer_lbl, lv_color_white(), 0);
    lv_obj_center(trainer_lbl);
    lv_obj_add_event_cb(trainer_btn, trainer_click_event_cb, LV_EVENT_RELEASED, (void*)(intptr_t)TRAINER_MEMORY);
    
    lbl = lv_label_create(stats_container);
    snprintf(line_buf, sizeof(line_buf), "Сесій: %d  Макс. рівень: %d  Вірних: %d", 
        stats->memory_sessions, stats->memory_best_level, stats->memory_total_correct);
    lv_label_set_text(lbl, line_buf);
    lv_obj_set_style_text_font(lbl, Font3, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

    // --- КООРДИНАЦІЯ --- (клікабельна кнопка)
    trainer_btn = lv_btn_create(stats_container);
    lv_obj_set_size(trainer_btn, LV_PCT(100), 30);
    lv_obj_set_style_bg_color(trainer_btn, lv_color_hex(0x9b59b6), 0);
    lv_obj_set_style_bg_color(trainer_btn, lv_color_hex(0x8e44ad), LV_STATE_PRESSED);
    lv_obj_set_style_radius(trainer_btn, 5, 0);
    trainer_lbl = lv_label_create(trainer_btn);
    lv_label_set_text(trainer_lbl, "КООРДИНАЦІЯ >");
    lv_obj_set_style_text_font(trainer_lbl, Font3, 0);
    lv_obj_set_style_text_color(trainer_lbl, lv_color_white(), 0);
    lv_obj_center(trainer_lbl);
    lv_obj_add_event_cb(trainer_btn, trainer_click_event_cb, LV_EVENT_RELEASED, (void*)(intptr_t)TRAINER_COORDINATION);
    
    lbl = lv_label_create(stats_container);
    snprintf(line_buf, sizeof(line_buf), "Сесій: %d  Рекорд: %d  Влучень: %d", 
        stats->coordination_sessions, stats->coordination_best_score, stats->coordination_total_hits);
    lv_label_set_text(lbl, line_buf);
    lv_obj_set_style_text_font(lbl, Font3, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    
    Serial.println("[STATS_SCR] Статистика створена");

    // Кнопка очищення статистики (у верхньому лівому куті)
    Serial.println("[STATS_SCR] Створюємо кнопку CLEAR...");
    lv_obj_t *clear_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(clear_btn, 140, 50);
    lv_obj_align(clear_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(COLOR_BTN_RED), 0);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(COLOR_BTN_RED_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "ОЧИСТИТИ");
    lv_obj_set_style_text_font(clear_label, Font3, 0);
    lv_obj_set_style_text_color(clear_label, lv_color_white(), 0);
    lv_obj_center(clear_label);

    lv_obj_add_event_cb(clear_btn, clear_stats_event_cb, LV_EVENT_RELEASED, NULL);
    Serial.println("[STATS_SCR] Кнопка CLEAR створена");

    // Кнопка назад (у верхньому правому куті)
    Serial.println("[STATS_SCR] Створюємо кнопку BACK...");
    lv_obj_t *back_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_btn, 140, 50);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLOR_BTN_BACK), 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLOR_BTN_BACK_PRESSED), LV_STATE_PRESSED);

    lv_obj_t *back_label_txt = lv_label_create(back_btn);
    lv_label_set_text(back_label_txt, "НАЗАД");
    lv_obj_set_style_text_font(back_label_txt, Font3, 0);
    lv_obj_set_style_text_color(back_label_txt, lv_color_white(), 0);
    lv_obj_center(back_label_txt);

    lv_obj_add_event_cb(back_btn, stats_back_event_cb, LV_EVENT_RELEASED, NULL);
    Serial.println("[STATS_SCR] ЕКРАН СТАТИСТИКИ ГОТОВИЙ!");
}