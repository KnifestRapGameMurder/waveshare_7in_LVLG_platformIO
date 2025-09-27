// #include "ui_screens.h"
// #include "app_config.h"
// #include "console.h"
// #include "animation.h"
// #include <Arduino.h>

// // UI object definitions
// lv_obj_t *loading_screen = nullptr;
// lv_obj_t *gradient_obj = nullptr;
// lv_obj_t *main_label = nullptr;
// lv_obj_t *sub_label_1 = nullptr;

// lv_obj_t *menu_screen = nullptr;
// lv_obj_t *menu_title = nullptr;
// lv_obj_t *menu_buttons[4] = {nullptr};
// lv_obj_t *back_button = nullptr;

// lv_obj_t *console_screen = nullptr;
// lv_obj_t *console_title = nullptr;
// lv_obj_t *console_textarea = nullptr;
// lv_obj_t *console_back_btn = nullptr;
// lv_obj_t *console_clear_btn = nullptr;

// void create_loading_screen()
// {
//     lv_obj_clean(lv_scr_act());

//     // Create gradient background object
//     gradient_obj = lv_obj_create(lv_scr_act());
//     lv_obj_set_size(gradient_obj, LV_HOR_RES, LV_VER_RES);
//     lv_obj_align(gradient_obj, LV_ALIGN_CENTER, 0, 0);
//     lv_obj_clear_flag(gradient_obj, LV_OBJ_FLAG_SCROLLABLE);
//     lv_obj_add_event_cb(gradient_obj, gradient_draw_event_cb, LV_EVENT_DRAW_MAIN, NULL);

//     // Make gradient object clickable for touch detection
//     lv_obj_add_flag(gradient_obj, LV_OBJ_FLAG_CLICKABLE);
//     lv_obj_add_event_cb(gradient_obj, screen_touch_event_cb, LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(gradient_obj, screen_touch_event_cb, LV_EVENT_PRESSED, NULL);

//     // Add main title with 96px font
//     main_label = lv_label_create(lv_scr_act());
//     lv_label_set_text(main_label, "НЕЙРО");
//     lv_obj_set_style_text_font(main_label, &minecraft_ten_96, 0);
//     lv_obj_set_style_text_color(main_label, lv_color_white(), 0);
//     lv_obj_align(main_label, LV_ALIGN_CENTER, 0, -80);

//     // Add subtitle with 96px font
//     sub_label_1 = lv_label_create(lv_scr_act());
//     lv_label_set_text(sub_label_1, "БЛОК");
//     lv_obj_set_style_text_font(sub_label_1, &minecraft_ten_96, 0);
//     lv_obj_set_style_text_color(sub_label_1, lv_color_white(), 0);
//     lv_obj_align_to(sub_label_1, main_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

//     // Add touch events to screen
//     lv_obj_add_event_cb(lv_scr_act(), screen_touch_event_cb, LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(lv_scr_act(), screen_touch_event_cb, LV_EVENT_PRESSED, NULL);
//     lv_obj_add_event_cb(lv_scr_act(), screen_touch_event_cb, LV_EVENT_PRESS_LOST, NULL);

//     Serial.println("[DEBUG] Loading screen created with touch events registered");
// }

// void create_main_menu()
// {
//     Serial.println("[DEBUG] Creating main menu...");
//     lv_obj_clean(lv_scr_act());

//     // Create 4 trainer buttons + 1 console button
//     const char *trainer_names[] = {"TRAINER 1", "TRAINER 2", "TRAINER 3", "CONSOLE"};
//     uint32_t button_colors[] = {0x2E8B57, 0x4169E1, 0xDC143C, 0x9932CC};

//     int btn_width = SCR_W / 2;
//     int btn_height = SCR_H / 2;

//     for (int i = 0; i < 4; i++)
//     {
//         int row = i / 2;
//         int col = i % 2;

//         menu_buttons[i] = lv_btn_create(lv_scr_act());
//         lv_obj_set_size(menu_buttons[i], btn_width, btn_height);

//         int x_pos = col * btn_width;
//         int y_pos = row * btn_height;
//         lv_obj_set_pos(menu_buttons[i], x_pos, y_pos);

//         // Button styling
//         lv_obj_set_style_bg_color(menu_buttons[i], lv_color_hex(button_colors[i]), 0);
//         lv_obj_set_style_bg_color(menu_buttons[i], lv_color_hex(button_colors[i] + 0x333333), LV_STATE_PRESSED);
//         lv_obj_set_style_border_color(menu_buttons[i], lv_color_white(), 0);
//         lv_obj_set_style_border_width(menu_buttons[i], 3, 0);
//         lv_obj_set_style_radius(menu_buttons[i], 0, 0);

//         // Button label
//         lv_obj_t *label = lv_label_create(menu_buttons[i]);
//         lv_label_set_text(label, trainer_names[i]);
//         lv_obj_set_style_text_font(label, &minecraft_ten_48, 0);
//         lv_obj_set_style_text_color(label, lv_color_white(), 0);
//         lv_obj_center(label);

//         // Add event handler
//         lv_obj_add_event_cb(menu_buttons[i], menu_button_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
//     }
// }

// void create_trainer_screen(int trainer_id)
// {
//     Serial.printf("[DEBUG] Creating trainer screen %d...\n", trainer_id + 1);
//     lv_obj_clean(lv_scr_act());

//     // Create dark background
//     lv_obj_t *bg = lv_obj_create(lv_scr_act());
//     lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
//     lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
//     lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a1a), 0);
//     lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

//     // Title
//     char title_text[32];
//     snprintf(title_text, sizeof(title_text), "ТРЕНАЖЕР %d", trainer_id + 1);
//     lv_obj_t *title = lv_label_create(lv_scr_act());
//     lv_label_set_text(title, title_text);
//     lv_obj_set_style_text_font(title, &minecraft_ten_48, 0);
//     lv_obj_set_style_text_color(title, lv_color_white(), 0);
//     lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

//     // Placeholder content
//     lv_obj_t *content = lv_label_create(lv_scr_act());
//     lv_label_set_text(content, "Тут буде вміст тренажера");
//     lv_obj_set_style_text_font(content, &minecraft_ten_48, 0);
//     lv_obj_set_style_text_color(content, lv_color_hex(0xcccccc), 0);
//     lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);

//     // Back button
//     back_button = lv_btn_create(lv_scr_act());
//     lv_obj_set_size(back_button, 200, 80);
//     lv_obj_align(back_button, LV_ALIGN_BOTTOM_MID, 0, -30);

//     lv_obj_set_style_bg_color(back_button, lv_color_hex(0x444444), 0);
//     lv_obj_set_style_bg_color(back_button, lv_color_hex(0x666666), LV_STATE_PRESSED);
//     lv_obj_set_style_border_color(back_button, lv_color_white(), 0);
//     lv_obj_set_style_border_width(back_button, 2, 0);

//     lv_obj_t *back_label = lv_label_create(back_button);
//     lv_label_set_text(back_label, "НАЗАД");
//     lv_obj_set_style_text_font(back_label, &minecraft_ten_48, 0);
//     lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
//     lv_obj_center(back_label);

//     lv_obj_add_event_cb(back_button, back_button_event_cb, LV_EVENT_CLICKED, NULL);
// }

// void create_console_screen()
// {
//     Serial.println("[DEBUG] Creating console screen...");
//     lv_obj_clean(lv_scr_act());

//     // Create dark background
//     lv_obj_t *bg = lv_obj_create(lv_scr_act());
//     lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
//     lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
//     lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a1a), 0);
//     lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

//     // Title
//     console_title = lv_label_create(lv_scr_act());
//     lv_label_set_text(console_title, "UART CONSOLE");
//     lv_obj_set_style_text_font(console_title, &minecraft_ten_48, 0);
//     lv_obj_set_style_text_color(console_title, lv_color_hex(0x9932CC), 0);
//     lv_obj_align(console_title, LV_ALIGN_TOP_MID, 0, 10);

//     // Console textarea
//     console_textarea = lv_textarea_create(lv_scr_act());
//     lv_obj_set_size(console_textarea, SCR_W - 40, SCR_H - 150);
//     lv_obj_align(console_textarea, LV_ALIGN_CENTER, 0, -10);

//     // Console styling
//     lv_obj_set_style_bg_color(console_textarea, lv_color_hex(0x000000), 0);
//     lv_obj_set_style_text_color(console_textarea, lv_color_hex(0x00FF00), 0);
//     lv_obj_set_style_border_color(console_textarea, lv_color_hex(0x9932CC), 0);
//     lv_obj_set_style_border_width(console_textarea, 2, 0);
//     lv_obj_set_style_radius(console_textarea, 8, 0);

//     lv_textarea_set_text(console_textarea, "");
//     lv_obj_add_state(console_textarea, LV_STATE_DISABLED);

//     // Button container
//     lv_obj_t *btn_container = lv_obj_create(lv_scr_act());
//     lv_obj_set_size(btn_container, SCR_W - 40, 60);
//     lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
//     lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
//     lv_obj_set_style_border_opa(btn_container, LV_OPA_TRANSP, 0);
//     lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
//     lv_obj_set_style_pad_gap(btn_container, 20, 0);
//     lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

//     // Clear button
//     console_clear_btn = lv_btn_create(btn_container);
//     lv_obj_set_size(console_clear_btn, 140, 50);
//     lv_obj_set_style_bg_color(console_clear_btn, lv_color_hex(0xFF6B35), 0);
//     lv_obj_set_style_bg_color(console_clear_btn, lv_color_hex(0xFF8C69), LV_STATE_PRESSED);

//     lv_obj_t *clear_label = lv_label_create(console_clear_btn);
//     lv_label_set_text(clear_label, "CLEAR");
//     lv_obj_set_style_text_font(clear_label, &minecraft_ten_48, 0);
//     lv_obj_center(clear_label);

//     // Back button
//     console_back_btn = lv_btn_create(btn_container);
//     lv_obj_set_size(console_back_btn, 140, 50);
//     lv_obj_set_style_bg_color(console_back_btn, lv_color_hex(0x444444), 0);
//     lv_obj_set_style_bg_color(console_back_btn, lv_color_hex(0x666666), LV_STATE_PRESSED);

//     lv_obj_t *back_label = lv_label_create(console_back_btn);
//     lv_label_set_text(back_label, "BACK");
//     lv_obj_set_style_text_font(back_label, &minecraft_ten_48, 0);
//     lv_obj_center(back_label);

//     // Add event handlers
//     lv_obj_add_event_cb(console_clear_btn, console_clear_event_cb, LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(console_back_btn, console_back_event_cb, LV_EVENT_CLICKED, NULL);

//     // Initialize console
//     console_init();
// }

// void screen_touch_event_cb(lv_event_t *event)
// {
//     lv_event_code_t code = lv_event_get_code(event);
//     Serial.printf("[TOUCH] Event received: %d, Current state: %d\n", code, current_state);

//     if (current_state == STATE_LOADING)
//     {
//         Serial.println("[TOUCH] Transition from loading to main menu");
//         current_state = STATE_MAIN_MENU;
//         state_start_time = lv_tick_get();
//         last_interaction_time = lv_tick_get();
//         create_main_menu();
//     }
// }

// void menu_button_event_cb(lv_event_t *event)
// {
//     lv_obj_t *btn = lv_event_get_target(event);
//     int trainer_id = (int)(intptr_t)lv_event_get_user_data(event);

//     Serial.printf("[MENU] Button %d pressed\n", trainer_id + 1);
//     last_interaction_time = lv_tick_get();

//     if (trainer_id == 3)
//     {
//         // Console button
//         current_state = STATE_CONSOLE;
//         state_start_time = lv_tick_get();
//         create_console_screen();
//     }
//     else
//     {
//         // Regular trainer button
//         current_state = (AppState)(STATE_TRAINER_1 + trainer_id);
//         state_start_time = lv_tick_get();
//         create_trainer_screen(trainer_id);
//     }
// }

// void back_button_event_cb(lv_event_t *event)
// {
//     Serial.println("[BACK] Button pressed - returning to main menu");
//     last_interaction_time = lv_tick_get();
//     current_state = STATE_MAIN_MENU;
//     state_start_time = lv_tick_get();
//     create_main_menu();
// }