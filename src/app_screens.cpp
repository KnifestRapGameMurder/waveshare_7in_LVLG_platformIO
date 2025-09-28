/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "app_screens.h"

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

// LVGL UI objects (defined here)
lv_obj_t *menu_buttons[4] = {NULL};
lv_obj_t *back_button = NULL;

lv_obj_t *debug_label = NULL;

// Event handlers
static void accuracy_difficulty_event_cb(lv_event_t *e)
{
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

    if (current_state == STATE_LOADING)
    {
        if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED)
        {
            Serial.println("[ДОТИК] Перехід від завантаження до головного меню");
            current_state = STATE_MAIN_MENU;
            state_start_time = lv_tick_get();
            last_interaction_time = lv_tick_get();
            create_main_menu();
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

    // Create 4 trainer buttons taking all screen space in 2x2 grid
    const char *trainer_names[] = {
        "ACCURACY",    // Accuracy Trainer
        "REACTION",    // Reaction Trainer
        "MEMORY",      // Memory Trainer
        "COORDINATION" // Coordination Trainer
    };

    // Different colors for each button
    uint32_t button_colors[] = {
        0xFFD700, // Gold for Accuracy
        0x00CED1, // Dark Turquoise for Reaction
        0x9370DB, // Medium Purple for Memory
        0x32CD32  // Lime Green for Coordination
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
        lv_obj_set_style_bg_color(menu_buttons[i], lv_color_hex(button_colors[i] + 0x333333), LV_STATE_PRESSED);
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

        // Add event handler
        lv_obj_add_event_cb(menu_buttons[i], menu_button_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Create trainer screen
void create_trainer_screen(int trainer_id)
{
    Serial.printf("[НАЛАГОДЖЕННЯ] Створення екрану тренажера %d...\n", trainer_id + 1);
    lv_obj_clean(lv_scr_act());

    // Create dark background
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

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
    lv_obj_set_style_text_color(content, lv_color_hex(0xcccccc), 0);
    lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);

    // Back button
    back_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_button, 200, 80);
    lv_obj_align(back_button, LV_ALIGN_BOTTOM_MID, 0, -30);

    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x666666), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(back_button, lv_color_white(), 0);
    lv_obj_set_style_border_width(back_button, 2, 0);

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_CLICKED, NULL);
}

// Create accuracy difficulty submenu
void create_accuracy_difficulty_submenu()
{
    lv_obj_clean(lv_scr_act());

    // Create dark background
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "ВИБЕРІТЬ СКЛАДНІСТЬ");
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Easy button
    lv_obj_t *easy_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(easy_btn, 300, 80);
    lv_obj_align(easy_btn, LV_ALIGN_CENTER, 0, -100);
    lv_obj_set_style_bg_color(easy_btn, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_bg_color(easy_btn, lv_color_hex(0x00AA00), LV_STATE_PRESSED);

    lv_obj_t *easy_label = lv_label_create(easy_btn);
    lv_label_set_text(easy_label, "ЛЕГКО");
    lv_obj_set_style_text_font(easy_label, Font2, 0);
    lv_obj_center(easy_label);

    lv_obj_add_event_cb(easy_btn, accuracy_difficulty_event_cb, LV_EVENT_CLICKED, (void *)0);

    // Medium button
    lv_obj_t *medium_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(medium_btn, 300, 80);
    lv_obj_align(medium_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(medium_btn, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_bg_color(medium_btn, lv_color_hex(0xAAAA00), LV_STATE_PRESSED);

    lv_obj_t *medium_label = lv_label_create(medium_btn);
    lv_label_set_text(medium_label, "СЕРЕДНЄ");
    lv_obj_set_style_text_font(medium_label, Font2, 0);
    lv_obj_center(medium_label);

    lv_obj_add_event_cb(medium_btn, accuracy_difficulty_event_cb, LV_EVENT_CLICKED, (void *)1);

    // Hard button
    lv_obj_t *hard_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(hard_btn, 300, 80);
    lv_obj_align(hard_btn, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_bg_color(hard_btn, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_color(hard_btn, lv_color_hex(0xAA0000), LV_STATE_PRESSED);

    lv_obj_t *hard_label = lv_label_create(hard_btn);
    lv_label_set_text(hard_label, "ВАЖКО");
    lv_obj_set_style_text_font(hard_label, Font2, 0);
    lv_obj_center(hard_label);

    lv_obj_add_event_cb(hard_btn, accuracy_difficulty_event_cb, LV_EVENT_CLICKED, (void *)2);

    // Back button
    back_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_button, 200, 80);
    lv_obj_align(back_button, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x666666), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(back_button, lv_color_white(), 0);
    lv_obj_set_style_border_width(back_button, 2, 0);

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_CLICKED, NULL);

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Create reaction submenu
void create_reaction_submenu()
{
    lv_obj_clean(lv_scr_act());

    // Create dark background
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

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
    lv_obj_set_style_bg_color(trial_btn, lv_color_hex(0x00CED1), 0);
    lv_obj_set_style_bg_color(trial_btn, lv_color_hex(0x008B8B), LV_STATE_PRESSED);

    lv_obj_t *trial_label = lv_label_create(trial_btn);
    lv_label_set_text(trial_label, "ЧАС РЕАКЦІЇ");
    lv_obj_set_style_text_font(trial_label, Font2, 0);
    lv_obj_center(trial_label);

    lv_obj_add_event_cb(trial_btn, reaction_mode_event_cb, LV_EVENT_CLICKED, (void *)0);

    // Survival button
    lv_obj_t *survival_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(survival_btn, 300, 80);
    lv_obj_align(survival_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(survival_btn, lv_color_hex(0xFF6347), 0);
    lv_obj_set_style_bg_color(survival_btn, lv_color_hex(0xCD5C5C), LV_STATE_PRESSED);

    lv_obj_t *survival_label = lv_label_create(survival_btn);
    lv_label_set_text(survival_label, "ВИЖИВАННЯ");
    lv_obj_set_style_text_font(survival_label, Font2, 0);
    lv_obj_center(survival_label);

    lv_obj_add_event_cb(survival_btn, reaction_mode_event_cb, LV_EVENT_CLICKED, (void *)1);

    // Back button
    back_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_button, 200, 80);
    lv_obj_align(back_button, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x666666), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(back_button, lv_color_white(), 0);
    lv_obj_set_style_border_width(back_button, 2, 0);

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_CLICKED, NULL);

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Create coordination submenu
void create_coordination_submenu()
{
    lv_obj_clean(lv_scr_act());

    // Create dark background
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "ВИБЕРІТЬ СКЛАДНІСТЬ");
    lv_obj_set_style_text_font(title, Font2, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Easy button
    lv_obj_t *easy_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(easy_btn, 300, 80);
    lv_obj_align(easy_btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(easy_btn, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_bg_color(easy_btn, lv_color_hex(0x00AA00), LV_STATE_PRESSED);

    lv_obj_t *easy_label = lv_label_create(easy_btn);
    lv_label_set_text(easy_label, "ЛЕГКО");
    lv_obj_set_style_text_font(easy_label, Font2, 0);
    lv_obj_center(easy_label);

    lv_obj_add_event_cb(easy_btn, coordination_difficulty_event_cb, LV_EVENT_CLICKED, (void *)0);

    // Hard button
    lv_obj_t *hard_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(hard_btn, 300, 80);
    lv_obj_align(hard_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(hard_btn, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_color(hard_btn, lv_color_hex(0xAA0000), LV_STATE_PRESSED);

    lv_obj_t *hard_label = lv_label_create(hard_btn);
    lv_label_set_text(hard_label, "ВАЖКО");
    lv_obj_set_style_text_font(hard_label, Font2, 0);
    lv_obj_center(hard_label);

    lv_obj_add_event_cb(hard_btn, coordination_difficulty_event_cb, LV_EVENT_CLICKED, (void *)1);

    // Back button
    back_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_button, 200, 80);
    lv_obj_align(back_button, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x666666), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(back_button, lv_color_white(), 0);
    lv_obj_set_style_border_width(back_button, 2, 0);

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_CLICKED, NULL);

    // At the end of each create_ function, replace the repeated block with:
    create_debug_label();
}

// Create reaction survival submenu
void create_reaction_survival_submenu()
{
    lv_obj_clean(lv_scr_act());

    // Create dark background
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

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
    lv_obj_set_style_bg_color(min1_btn, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_bg_color(min1_btn, lv_color_hex(0x00AA00), LV_STATE_PRESSED);

    lv_obj_t *min1_label = lv_label_create(min1_btn);
    lv_label_set_text(min1_label, "1 ХВИЛИНА");
    lv_obj_set_style_text_font(min1_label, Font2, 0);
    lv_obj_center(min1_label);

    lv_obj_add_event_cb(min1_btn, survival_duration_event_cb, LV_EVENT_CLICKED, (void *)1);

    // 2 minutes button
    lv_obj_t *min2_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(min2_btn, 300, 80);
    lv_obj_align(min2_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(min2_btn, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_bg_color(min2_btn, lv_color_hex(0xAAAA00), LV_STATE_PRESSED);

    lv_obj_t *min2_label = lv_label_create(min2_btn);
    lv_label_set_text(min2_label, "2 ХВИЛИНИ");
    lv_obj_set_style_text_font(min2_label, Font2, 0);
    lv_obj_center(min2_label);

    lv_obj_add_event_cb(min2_btn, survival_duration_event_cb, LV_EVENT_CLICKED, (void *)2);

    // 3 minutes button
    lv_obj_t *min3_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(min3_btn, 300, 80);
    lv_obj_align(min3_btn, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_bg_color(min3_btn, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_color(min3_btn, lv_color_hex(0xAA0000), LV_STATE_PRESSED);

    lv_obj_t *min3_label = lv_label_create(min3_btn);
    lv_label_set_text(min3_label, "3 ХВИЛИНИ");
    lv_obj_set_style_text_font(min3_label, Font2, 0);
    lv_obj_center(min3_label);

    lv_obj_add_event_cb(min3_btn, survival_duration_event_cb, LV_EVENT_CLICKED, (void *)3);

    // Back button
    back_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_button, 200, 80);
    lv_obj_align(back_button, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x666666), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(back_button, lv_color_white(), 0);
    lv_obj_set_style_border_width(back_button, 2, 0);

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, Font2, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_CLICKED, NULL);

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