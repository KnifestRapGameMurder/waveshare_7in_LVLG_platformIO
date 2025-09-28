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
#include "uart_protocol.h"
#include <Preferences.h>

// Include external components
#include "loading_screen.h"
#include "app_screens.h" // New: Includes AppState, extern vars, and screen creation functions

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// ---------- ANTI-TEARING CONFIGURATION (Based on ESP-BSP proven solutions) ----------
static const uint32_t TARGET_FPS = 30;
static const uint32_t FRAME_MS = 1000 / TARGET_FPS;

// Application states and variables (No longer static, accessible via extern in app_screens.h)
AppState current_state = STATE_LOADING;
uint32_t state_start_time = 0;
const uint32_t IDLE_TIMEOUT = 10000; // 10 seconds idle timeout
uint32_t last_interaction_time = 0;
Preferences preferences;

HardwareSerial uart_serial(2);
UARTProtocol uart_protocol(&uart_serial);

// Screen dimensions (No longer static, accessible via extern in app_screens.h)
int32_t SCR_W = 800,
        SCR_H = 480;

// Note: UI objects (menu_buttons, back_button) are now extern and defined in app_screens.cpp.

// Forward declaration for the timer callback (remains here as it controls the core loop)
static void app_timer_cb(lv_timer_t *timer);

///
static void lv_color_to_hex6(lv_color_t c, char out[7]) // out: "RRGGBB"
{
    uint32_t xrgb = lv_color_to32(c); // XRGB8888 (alpha=0xFF)
    uint8_t r = (xrgb >> 16) & 0xFF;
    uint8_t g = (xrgb >> 8) & 0xFF;
    uint8_t b = (xrgb >> 0) & 0xFF;
    snprintf(out, 7, "%02X%02X%02X", r, g, b);
}
///

/**
 * @brief Main application timer for state management and continuous updates.
 */
static void app_timer_cb(lv_timer_t *timer)
{
    static uint32_t last_time = 0;
    uint32_t now = lv_tick_get();

    // 1. Check for idle timeout (return to loading screen if no interaction)
    if (current_state == STATE_MAIN_MENU)
    {
        if ((now - last_interaction_time) >= IDLE_TIMEOUT)
        {
            Serial.println("[ТАЙМ-АУТ] Досягнуто тайм-аут бездіяльності - повернення до екрану завантаження");
            current_state = STATE_LOADING;
            state_start_time = now;
            // Uses app_screen_touch_cb from app_screens.h
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
    // 3. Run trainer logic based on current state
    else
    {
        switch (current_state)
        {
        case STATE_ACCURACY_TRAINER:
            run_accuracy_trainer();
            break;
        case STATE_REACTION_TIME_TRIAL:
            run_time_trial();
            break;
        case STATE_REACTION_SURVIVAL:
            run_survival_time_trainer();
            break;
        case STATE_MEMORY_TRAINER:
            run_memory_trainer();
            break;
        case STATE_COORDINATION_TRAINER:
            run_coordination_trainer();
            break;
        default:
            // Menu states don't need continuous updates
            break;
        }
    }

    last_time = now;
}

void setup()
{
    Serial.begin(115200, SERIAL_8N1, 44, 43);
    Serial.println("=== ESP32-S3 RGB LCD Система Без Розривів ===");

    Serial.println("Ініціалізація плати з конфігурацією без розривів...");
    Board *board = new Board();
    board->init();

    uart_serial.begin(115200, SERIAL_8N1, 44, 43);

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
