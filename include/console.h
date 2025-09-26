#pragma once
#include <lvgl.h>

// Console functions
void console_add_log(const char *message);
void console_back_event_cb(lv_event_t *event);
void console_clear_event_cb(lv_event_t *event);
void console_init();