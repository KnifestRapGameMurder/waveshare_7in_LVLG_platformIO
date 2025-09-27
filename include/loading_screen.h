/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // The screen transition callback defined in main.cpp
    typedef void (*ScreenTransitionCallback_t)(lv_event_t *e);

    /**
     * @brief Initializes geometry and orbit parameters for the loading screen animation.
     * @param scr_w Screen width in pixels.
     * @param scr_h Screen height in pixels.
     */
    void loading_screen_init_params(int scr_w, int scr_h);

    /**
     * @brief Creates and displays the loading screen UI.
     * It sets up the animated gradient background and the main labels.
     * @param transition_cb Callback to execute when the screen is touched (for state transition).
     */
    void loading_screen_create(ScreenTransitionCallback_t transition_cb);

    /**
     * @brief Updates the orbital animation logic and invalidates the gradient area for redraw.
     * Call this function periodically while in the loading state.
     * @param dt Delta time in seconds since the last update.
     */
    void loading_screen_update_animation(float dt);

    static lv_color_t distance_color_map(int px, int py);

#ifdef __cplusplus
}
#endif

#endif // LOADING_SCREEN_H
