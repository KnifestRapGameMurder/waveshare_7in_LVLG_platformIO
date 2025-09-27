#include "console.h"
#include <lvgl.h>
#include <deque>
#include <Arduino.h> // For String, millis(), etc.

// --- Module-private (file-scoped) variables ---
namespace
{
    const int MAX_LOG_MESSAGES = 15; // How many messages to keep in the log
    lv_obj_t *log_screen;
    lv_obj_t *log_label;
    lv_obj_t *status_label;
    lv_obj_t *time_label;
    std::deque<String> log_buffer;
}

void console_init()
{
    log_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(log_screen, lv_color_hex(0x000000), 0);

    status_label = lv_label_create(log_screen);
    lv_label_set_text(status_label, "Initializing...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);

    time_label = lv_label_create(log_screen);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    log_label = lv_label_create(log_screen);
    lv_label_set_text(log_label, "Console initialized.\n");
    lv_obj_set_style_text_color(log_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_width(log_label, lv_disp_get_hor_res(NULL) - 20);
    lv_label_set_long_mode(log_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(log_label, LV_ALIGN_TOP_LEFT, 10, 40);

    lv_scr_load(log_screen);
}

void console_log(const char *message)
{
    if (log_label == NULL)
        return;

    unsigned long currentTime = millis();
    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "[%02lu:%02lu:%02lu] %s",
             (currentTime / 3600000) % 24, (currentTime / 60000) % 60,
             (currentTime / 1000) % 60, message);

    if (log_buffer.size() == 0 && lv_label_get_text(log_label) != NULL)
    {
        lv_label_set_text(log_label, ""); // Clear initial text on first log
    }

    log_buffer.push_back(String(log_entry));
    if (log_buffer.size() > MAX_LOG_MESSAGES)
    {
        log_buffer.pop_front();
    }

    String full_log_text;
    for (const auto &line : log_buffer)
    {
        full_log_text += line;
        full_log_text += "\n";
    }

    lv_label_set_text(log_label, full_log_text.c_str());
}

void console_set_status(const char *status)
{
    if (status_label != NULL)
    {
        lv_label_set_text(status_label, status);
    }
}

void console_update()
{
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate >= 1000)
    {
        lastTimeUpdate = millis();
        if (time_label != NULL)
        {
            unsigned long now = millis();
            char time_str[16];
            snprintf(time_str, sizeof(time_str), "%02lu:%02lu:%02lu",
                     (now / 3600000) % 24, (now / 60000) % 60, (now / 1000) % 60);
            lv_label_set_text(time_label, time_str);
        }
    }
}