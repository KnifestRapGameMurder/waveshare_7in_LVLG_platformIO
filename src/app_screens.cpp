/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "app_screens.h"

// LVGL UI objects (defined here)
lv_obj_t *menu_buttons[4] = {NULL};
lv_obj_t *back_button = NULL;

// Button event handler for main menu
static void menu_button_event_cb(lv_event_t *event)
{
    lv_obj_t *btn = lv_event_get_target(event);
    int trainer_id = (int)(intptr_t)lv_event_get_user_data(event);

    Serial.printf("[МЕНЮ] Натиснуто кнопку %d\n", trainer_id + 1);

    // Update interaction time to reset idle timeout (uses extern var)
    last_interaction_time = lv_tick_get();

    // Switch to selected trainer (uses extern var)
    current_state = (AppState)(STATE_TRAINER_1 + trainer_id);
    state_start_time = lv_tick_get();
    create_trainer_screen(trainer_id);
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
        "ТРЕНАЖЕР 1",
        "ТРЕНАЖЕР 2",
        "ТРЕНАЖЕР 3",
        "ТРЕНАЖЕР 4"};

    // Different colors for each button
    uint32_t button_colors[] = {
        0x2E8B57, // Sea Green
        0x4169E1, // Royal Blue
        0xDC143C, // Crimson Red
        0xFF8C00  // Dark Orange
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
        lv_obj_set_style_text_font(label, &minecraft_ten_48, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_center(label);

        // Add event handler
        lv_obj_add_event_cb(menu_buttons[i], menu_button_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
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
    lv_obj_set_style_text_font(title, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Placeholder content - THIS IS WHERE YOU WILL CALL THE UNIQUE TRAINER FUNCTION LATER
    lv_obj_t *content = lv_label_create(lv_scr_act());
    lv_label_set_text(content, "Тут буде вміст тренажера (Trainer Specific Logic Goes Here)");
    lv_obj_set_style_text_font(content, &minecraft_ten_48, 0);
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
    lv_obj_set_style_text_font(back_label, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_CLICKED, NULL);
}
