/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "lv_conf.h"
#include <math.h>

// Include the newly created loading screen header
#include "loading_screen.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// ---------- ANTI-TEARING CONFIGURATION (Based on ESP-BSP proven solutions) ----------
static const uint32_t TARGET_FPS = 30;
static const uint32_t FRAME_MS = 1000 / TARGET_FPS;

// Application states
enum AppState
{
    STATE_LOADING,   // Loading screen with animation
    STATE_MAIN_MENU, // Main menu with 4 training buttons
    STATE_TRAINER_1, // Training module 1
    STATE_TRAINER_2, // Training module 2
    STATE_TRAINER_3, // Training module 3
    STATE_TRAINER_4  // Training module 4
};

static AppState current_state = STATE_LOADING;
static uint32_t state_start_time = 0;
// static const uint32_t LOADING_DURATION = 5000; // 5 seconds - Removed as touch is primary trigger
static const uint32_t IDLE_TIMEOUT = 10000;
// 10 seconds idle timeout
static uint32_t last_interaction_time = 0;

// Screen dimensions
static int32_t SCR_W = 800, SCR_H = 480;

// UI objects for main menu (kept here as they belong to main menu logic)
static lv_obj_t *menu_buttons[4];
static lv_obj_t *back_button;

// Forward declarations
static void create_main_menu();
static void create_trainer_screen(int trainer_id);
static void app_screen_touch_cb(lv_event_t *event);
static void app_timer_cb(lv_timer_t *timer);

// Button event handler for main menu
static void menu_button_event_cb(lv_event_t *event)
{
    lv_obj_t *btn = lv_event_get_target(event);
    int trainer_id = (int)(intptr_t)lv_event_get_user_data(event);

    Serial.printf("[МЕНЮ] Натиснуто кнопку %d\n", trainer_id + 1);

    // Update interaction time to reset idle timeout
    last_interaction_time = lv_tick_get();

    // Switch to selected trainer
    current_state = (AppState)(STATE_TRAINER_1 + trainer_id);
    state_start_time = lv_tick_get();
    create_trainer_screen(trainer_id);
}

// Back button event handler
static void back_button_event_cb(lv_event_t *event)
{
    Serial.println("[НАЗАД] Натиснуто кнопку назад - повернення до головного меню");
    last_interaction_time = lv_tick_get();
    current_state = STATE_MAIN_MENU;
    state_start_time = lv_tick_get();
    create_main_menu();
}

/**
 * @brief Handles touch events across the application.
 * Currently only used to transition from STATE_LOADING to STATE_MAIN_MENU.
 */
static void app_screen_touch_cb(lv_event_t *event)
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

/**
 * @brief Main application timer for state management and continuous updates.
 */
static void app_timer_cb(lv_timer_t *timer)
{
    static uint32_t last_time = 0;
    uint32_t now = lv_tick_get();

    // 1. Check for idle timeout (return to loading screen if no interaction)
    if (current_state == STATE_MAIN_MENU || (current_state >= STATE_TRAINER_1 && current_state <= STATE_TRAINER_4))
    {
        if ((now - last_interaction_time) >= IDLE_TIMEOUT)
        {
            Serial.println("[ТАЙМ-АУТ] Досягнуто тайм-аут бездіяльності - повернення до екрану завантаження");
            current_state = STATE_LOADING;
            state_start_time = now;
            // Now use the encapsulated function
            loading_screen_create(app_screen_touch_cb);
            return;
        }
    }

    // 2. Run animation update if in loading state
    if (current_state == STATE_LOADING)
    {
        // Calculate delta time for FPS-independent animation
        float dt = (now - last_time) / 1000.0f;
        if (last_time == 0 || dt < 0.001f || dt > 0.1f)
            dt = 0.033f; // 30 FPS fallback

        loading_screen_update_animation(dt);
    }

    last_time = now;
}

// Create main menu with 4 trainer buttons taking full screen
static void create_main_menu()
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

    // Each button takes half of screen width and height
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

        // Button label with 48px font (assuming minecraft_ten_48 is externally defined)
        lv_obj_t *label = lv_label_create(menu_buttons[i]);
        lv_label_set_text(label, trainer_names[i]);
        // Placeholder font style since the font definition wasn't in the provided code
        lv_obj_set_style_text_font(label, &minecraft_ten_48, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_center(label);

        // Add event handler
        lv_obj_add_event_cb(menu_buttons[i], menu_button_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

// Create trainer screen
static void create_trainer_screen(int trainer_id)
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

    // Placeholder content
    lv_obj_t *content = lv_label_create(lv_scr_act());
    lv_label_set_text(content, "Тут буде вміст тренажера");
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

void setup()
{
    Serial.begin(115200);
    Serial.println("=== ESP32-S3 RGB LCD Система Без Розривів ===");

    Serial.println("Ініціалізація плати з конфігурацією без розривів...");
    Board *board = new Board();
    board->init();

// Configure anti-tearing RGB double-buffer mode (ESP-BSP style)
#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();

    Serial.println("Налаштування RGB подвійного буфера для усунення розривів...");
    lcd->configFrameBufferNumber(2); // RGB double-buffer mode

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
    {
        Serial.println("Налаштування RGB буфера відскоку для ESP32-S3...");
        int bounce_buffer_height = lcd->getFrameHeight() / 10;
        int bounce_buffer_size = lcd->getFrameWidth() * bounce_buffer_height;

        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(bounce_buffer_size);

        Serial.printf("RGB буфер відскоку налаштовано: %dx%d пікселів\n",
                      lcd->getFrameWidth(), bounce_buffer_height);
    }
#endif
#endif

    assert(board->begin());
    Serial.println("Плата ініціалізована з RGB конфігурацією без розривів!");

    Serial.println("Ініціалізація LVGL з повним оновленням...");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Створення UI з градієнтом без розривів...");
    lvgl_port_lock(-1);

    // Get actual screen dimensions
    SCR_W = lv_disp_get_hor_res(NULL);
    SCR_H = lv_disp_get_ver_res(NULL);

    Serial.printf("Роздільність екрану: %dx%d\n", SCR_W, SCR_H);

    // Initialize orbit parameters using the new function
    loading_screen_init_params(SCR_W, SCR_H);

    // Initialize app state
    current_state = STATE_LOADING;
    state_start_time = lv_tick_get();
    last_interaction_time = state_start_time;

    Serial.println("[НАЛАГОДЖЕННЯ] Додаток ініціалізовано - Запуск в стані ЗАВАНТАЖЕННЯ");

    // Create initial loading screen using the new function
    loading_screen_create(app_screen_touch_cb);

    // Start application timer for state management and animation
    lv_timer_create(app_timer_cb, FRAME_MS, NULL);

    lvgl_port_unlock();

    Serial.println("=== КОНФІГУРАЦІЯ БЕЗ РОЗРИВІВ ЗАВЕРШЕНА ===");
}

void loop()
{
    delay(5);
}
