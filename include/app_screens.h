/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#ifndef APP_SCREENS_H
#define APP_SCREENS_H

#include <lvgl.h>
#include <Arduino.h>
#include "accuracy_trainer.h"
#include "reaction_trainer.h"
#include "memory_trainer.h"
#include "coordination_trainer.h"
#include "fonts.h"

// Debug label for button states
extern lv_obj_t *debug_label;

/**
 * @brief Structure holding common UI elements for trainer screens.
 * Used by create_trainer_screen_base() to return pointers to created elements.
 */
struct TrainerScreenElements
{
    lv_obj_t *screen;        // Main container
    lv_obj_t *top_label;     // Top label (for round/level display)
    lv_obj_t *info_label;    // Center info label
    lv_obj_t *results_label; // Results label (hidden initially)
    lv_obj_t *back_btn;      // Back button
};

// Application states (Defined here as they control the UI flow)
enum AppState
{
    STATE_LOADING,              // Loading screen with animation
    STATE_MAIN_MENU,            // Main menu with 4 training buttons
    STATE_ACCURACY_TRAINER,     // Accuracy trainer
    STATE_REACTION_TRAINER,     // Reaction trainer (Time Trial & Survival)
    STATE_MEMORY_TRAINER,       // Memory trainer
    STATE_COORDINATION_TRAINER, // Coordination trainer

    // Sub-states for reaction trainer
    STATE_REACTION_SUBMENU,
    STATE_REACTION_TIME_TRIAL,
    STATE_REACTION_SURVIVAL,
    STATE_REACTION_SURVIVAL_SUBMENU,

    // Sub-states for accuracy trainer
    STATE_ACCURACY_DIFFICULTY_SUBMENU,

    // Sub-states for coordination trainer
    STATE_COORDINATION_SUBMENU
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
 * @brief Creates the base trainer screen with common UI elements.
 * @param back_event_cb Callback for the back button click event.
 * @return TrainerScreenElements structure with pointers to created UI elements.
 */
TrainerScreenElements create_trainer_screen_base(lv_event_cb_t back_event_cb);

/**
 * @brief Structure holding game over menu button elements.
 */
struct GameOverMenuElements
{
    lv_obj_t *play_again_btn;
    lv_obj_t *exit_btn;
};

/**
 * @brief Creates game over menu with "Play Again" and "Exit" buttons.
 * @param parent Parent container for buttons.
 * @param info_label Info label to hide (can be NULL).
 * @param results_label Results label to hide (can be NULL).
 * @param event_cb Callback for button clicks (user_data: 0=play again, 1=exit).
 * @return GameOverMenuElements structure with pointers to created buttons.
 */
GameOverMenuElements create_game_over_menu(lv_obj_t *parent, lv_obj_t *info_label, 
                                           lv_obj_t *results_label, lv_event_cb_t event_cb);

/**
 * @brief Handles touch events across the application (used by loading screen and flow control).
 * This function handles the transition from STATE_LOADING to STATE_MAIN_MENU.
 */
void app_screen_touch_cb(lv_event_t *event);

// Function to create/update debug label
void create_debug_label();

#endif // APP_SCREENS_H
