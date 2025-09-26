#pragma once
#include <lvgl.h>

// UI object declarations for different screens
extern lv_obj_t *loading_screen;
extern lv_obj_t *gradient_obj;
extern lv_obj_t *main_label;
extern lv_obj_t *sub_label_1;

extern lv_obj_t *menu_screen;
extern lv_obj_t *menu_title;
extern lv_obj_t *menu_buttons[4];
extern lv_obj_t *back_button;

extern lv_obj_t *console_screen;
extern lv_obj_t *console_title;
extern lv_obj_t *console_textarea;
extern lv_obj_t *console_back_btn;
extern lv_obj_t *console_clear_btn;

// External font declarations
extern "C"
{
    extern const lv_font_t minecraft_ten_48;
    extern const lv_font_t minecraft_ten_96;
}

// UI creation functions
void create_loading_screen();
void create_main_menu();
void create_trainer_screen(int trainer_id);
void create_console_screen();

// Event handlers
void screen_touch_event_cb(lv_event_t *event);
void menu_button_event_cb(lv_event_t *event);
void back_button_event_cb(lv_event_t *event);