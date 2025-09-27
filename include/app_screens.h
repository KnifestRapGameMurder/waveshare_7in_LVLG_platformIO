/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#ifndef APP_SCREENS_H
#define APP_SCREENS_H

#include <lvgl.h>
#include <Arduino.h>

// Forward declaration for missing font definition in this scope
// NOTE: This must be defined externally for compilation to succeed.
extern const lv_font_t minecraft_ten_48;

// Application states (Defined here as they control the UI flow)
enum AppState
{
    STATE_LOADING,   // Loading screen with animation
    STATE_MAIN_MENU, // Main menu with 4 training buttons
    STATE_TRAINER_1, // Training module 1
    STATE_TRAINER_2, // Training module 2
    STATE_TRAINER_3, // Training module 3
    STATE_TRAINER_4  // Training module 4
};

// Application State Variables (Defined in main.cpp, declared here as extern)
extern AppState current_state;
extern uint32_t state_start_time;
extern uint32_t last_interaction_time;
extern int32_t SCR_W, SCR_H;

// UI Objects (Defined in app_screens.cpp, used globally)
extern lv_obj_t *menu_buttons[4];
extern lv_obj_t *back_button;

/**
 * @brief Creates the main menu screen with 4 navigation buttons.
 */
void create_main_menu();

/**
 * @brief Creates the generic trainer screen container.
 * @param trainer_id The ID of the trainer module (0-3).
 */
void create_trainer_screen(int trainer_id);

/**
 * @brief Handles touch events across the application (used by loading screen and flow control).
 * This function handles the transition from STATE_LOADING to STATE_MAIN_MENU.
 */
void app_screen_touch_cb(lv_event_t *event);

#endif // APP_SCREENS_H
