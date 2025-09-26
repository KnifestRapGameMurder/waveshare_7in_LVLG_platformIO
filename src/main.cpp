/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"

// Application modules
#include "app_config.h"
#include "ui_screens.h"
#include "console.h"
#include "uart_handler.h"
#include "animation.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// Board instance
Board *board = nullptr;

// Global state variables definitions
AppState current_state = STATE_LOADING;
uint32_t state_start_time = 0;
uint32_t last_interaction_time = 0;
int32_t SCR_W = 800, SCR_H = 480;

void setup()
{
    Serial.begin(115200);
    Serial.println("\n========== UKRAINE TRAINING ESP32-S3 ==========");
    Serial.println("  Target: ESP32-S3 Touch LCD 7-inch");
    Serial.println("  Port: COM5");
    Serial.println("  UART Protocol: Enhanced with Console Debug");
    Serial.println("===============================================\n");

    // Initialize board and LVGL
    Serial.println("Initializing board with anti-tearing configuration...");
    board = new Board();
    board->init();
    
    if (!board->begin()) {
        Serial.println("[ERROR] Board initialization failed!");
        while (1) delay(1000);
    }
    
    Serial.println("Initializing LVGL port...");
    if (!lvgl_port_init(board->getLCD(), board->getTouch())) {
        Serial.println("[ERROR] LVGL port initialization failed!");
        while (1) delay(1000);
    }

    Serial.println("[INIT] LVGL initialized successfully with RGB double-buffer");

    // Initialize UART handler
    uart_handler_init();
    Serial.println("[INIT] UART protocol initialized on pins RX=44, TX=43");

    // Get screen dimensions and initialize animation parameters
    SCR_W = lv_disp_get_hor_res(lv_disp_get_default());
    SCR_H = lv_disp_get_ver_res(lv_disp_get_default());

    Serial.printf("[INIT] Screen resolution: %dx%d\n", SCR_W, SCR_H);

    // Initialize orbital animation system
    animation_init(SCR_W, SCR_H);

    // Initialize timestamps
    state_start_time = lv_tick_get();
    last_interaction_time = lv_tick_get();

    // Create initial loading screen
    create_loading_screen();

    Serial.println("[INIT] Application initialized successfully");
    Serial.println("[READY] Touch screen to proceed to main menu");
}

void loop()
{
    // Process LVGL tasks
    lv_timer_handler();

    // Process UART communication
    uart_handler_process();

    // Handle animation updates
    animation_update();

    // Auto-advance from loading screen after 5 seconds
    if (current_state == STATE_LOADING)
    {
        uint32_t elapsed = lv_tick_get() - state_start_time;
        if (elapsed >= LOADING_DURATION)
        {
            Serial.println("[AUTO] Loading timeout reached - switching to main menu");
            current_state = STATE_MAIN_MENU;
            state_start_time = lv_tick_get();
            last_interaction_time = lv_tick_get();
            create_main_menu();
        }
    }

    // Small delay to prevent excessive CPU usage
    delay(10);
}