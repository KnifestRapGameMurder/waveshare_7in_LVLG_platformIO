#include "console.h"
#include "app_config.h"
#include "ui_screens.h"
#include <Arduino.h>

void console_add_log(const char *message)
{
    if (console_textarea == NULL)
        return;

    // Get current time for timestamp
    uint32_t now = millis();
    uint32_t seconds = now / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "[%02lu:%02lu:%02lu] ",
             hours % 24, minutes % 60, seconds % 60);

    // Add timestamped message to console
    const char *current_text = lv_textarea_get_text(console_textarea);
    char *new_text = (char *)malloc(strlen(current_text) + strlen(timestamp) + strlen(message) + 10);

    if (new_text)
    {
        strcpy(new_text, current_text);
        strcat(new_text, timestamp);
        strcat(new_text, message);
        strcat(new_text, "\n");

        lv_textarea_set_text(console_textarea, new_text);

        // Auto-scroll to bottom
        lv_obj_scroll_to_y(console_textarea, LV_COORD_MAX, LV_ANIM_ON);

        free(new_text);
    }
}

void console_back_event_cb(lv_event_t *event)
{
    Serial.println("[CONSOLE] Returning to main menu");
    last_interaction_time = lv_tick_get();
    current_state = STATE_MAIN_MENU;
    state_start_time = lv_tick_get();
    create_main_menu();
}

void console_clear_event_cb(lv_event_t *event)
{
    Serial.println("[CONSOLE] Clearing logs");
    if (console_textarea)
    {
        lv_textarea_set_text(console_textarea, "");
        console_add_log("Console cleared");
    }
}

void console_init()
{
    console_add_log("UART Console started");
    console_add_log("Waiting for peripheral device data...");
    console_add_log("Console system initialized");
}