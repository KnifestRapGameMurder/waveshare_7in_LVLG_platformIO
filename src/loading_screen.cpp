/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "loading_screen.h"
#include <math.h>
#include <stdio.h>
#include <Arduino.h>

// Assumes these fonts are defined and linked externally (e.g., in lv_conf.h)
extern const lv_font_t minecraft_ten_96;

// ==============================================================================
// PRIVATE UI OBJECTS
// ==============================================================================
static lv_obj_t *gradient_obj = NULL;
static lv_obj_t *main_label = NULL;
static lv_obj_t *sub_label_1 = NULL;

// ==============================================================================
// PRIVATE ANIMATION PARAMETERS
// ==============================================================================

// Parameters for the three colored dots in the orbital simulation
struct ColorDot
{
    float x, y;
    lv_color_t color;
};

static float orbit_cx = 0.0f, orbit_cy = 0.0f;
static float orbit_angle[3] = {0.0f, 0.0f, 0.0f};
// rad/s - faster for visible movement
static const float orbit_speed[3] = {2.5f, -3.2f, 4.0f};
static float orbit_radius[3] = {0.0f, 0.0f, 0.0f};

// Scale parameters for orbital simulation (creates elliptical path)
static const float orbit_scale_x = 2.0f;
static const float orbit_scale_y = 1.5f;

static ColorDot dots[3];

// Maximum distance used for color mapping normalization
#define MAX_DIST 800.0f
#define SATURATION_FACTOR 3.0f

// ==============================================================================
// PRIVATE HELPER FUNCTIONS
// ==============================================================================

/**
 * @brief Maps distance from a point to a color channel value [0.0, 1.0].
 * Closer points result in a higher value (closer to 1.0).
 */
float channel_value(float dist)
{
    float max_value = 0.8f;
    // Normalize distance to [0.0, 1.0] range based on MAX_DIST
    float norm_dist = fminf(dist / MAX_DIST, max_value);
    // Apply non-linear scaling (squared) for smoother gradient
    return max_value - (norm_dist);
}

/**
 * @brief Saturates the given floating-point RGB color components.
 */
static void saturate_color_floats(float *r, float *g, float *b)
{
    float r_f = *r;
    float g_f = *g;
    float b_f = *b;

    // 1. Calculate the average luminance (gray component) in [0.0f, 1.0f]
    float avg = (r_f + g_f + b_f) / 3.0f;

    // 2. Saturate: C_new = avg + FACTOR * (C_old - avg)
    float r_new = fmaf(SATURATION_FACTOR, (r_f - avg), avg);
    float g_new = fmaf(SATURATION_FACTOR, (g_f - avg), avg);
    float b_new = fmaf(SATURATION_FACTOR, (b_f - avg), avg);

    // 3. Clamp the resulting values to the required [0.0f, 1.0f] range
    *r = fmaxf(0.0f, fminf(1.0f, r_new));
    *g = fmaxf(0.0f, fminf(1.0f, g_new));
    *b = fmaxf(0.0f, fminf(1.0f, b_new));
}

/**
 * @brief Computes a color based on the inverse distance to the three color dots.
 * This creates the smoothly blending, dynamic gradient effect.
 */
static lv_color_t distance_color_map(int px, int py)
{
    float x = (float)px, y = (float)py;

    // Compute actual distances
    float dist0 = sqrtf(powf(x - dots[0].x, 2) + powf(y - dots[0].y, 2));
    float dist1 = sqrtf(powf(x - dots[1].x, 2) + powf(y - dots[1].y, 2));
    float dist2 = sqrtf(powf(x - dots[2].x, 2) + powf(y - dots[2].y, 2));

    // Apply the formula: C = 1 - (dist / MAX_DIST)
    float r_f = channel_value(dist0); // Red based on point 0
    float g_f = channel_value(dist1); // Green based on point 1
    float b_f = channel_value(dist2); // Blue based on point 2

    saturate_color_floats(&r_f, &g_f, &b_f);

    // Convert float [0.0, 1.0] to uint8_t [0, 255]
    uint8_t r = (uint8_t)(r_f * 255.0f);
    uint8_t g = (uint8_t)(g_f * 255.0f);
    uint8_t b = (uint8_t)(b_f * 255.0f);

    return lv_color_make(r, g, b);
}

/**
 * @brief Custom draw event callback for smooth gradient rendering.
 * Draws the background using color interpolation based on animated dot positions.
 */
static void gradient_draw_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_coord_t obj_w = lv_area_get_width(&coords);
    lv_coord_t obj_h = lv_area_get_height(&coords);

    // Use a step size for drawing blocks to balance performance and visual smoothness
    const int STEP = 32;

    for (lv_coord_t y = 0; y < obj_h; y += STEP)
    {
        for (lv_coord_t x = 0; x < obj_w; x += STEP)
        {
            // Calculate absolute screen coordinates (center of the block)
            int screen_x = coords.x1 + x + STEP / 2;
            int screen_y = coords.y1 + y + STEP / 2;

            // Get interpolated color for this position
            lv_color_t color = distance_color_map(screen_x, screen_y);

            // Define the rectangle area to fill
            lv_area_t fill_area;
            fill_area.x1 = coords.x1 + x;
            fill_area.y1 = coords.y1 + y;
            fill_area.x2 = coords.x1 + x + STEP - 1;
            fill_area.y2 = coords.y1 + y + STEP - 1;

            // Ensure we don't draw outside object bounds
            if (fill_area.x2 > coords.x2)
                fill_area.x2 = coords.x2;
            if (fill_area.y2 > coords.y2)
                fill_area.y2 = coords.y2;

            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.bg_color = color;
            rect_dsc.bg_opa = LV_OPA_COVER;
            rect_dsc.border_width = 0;

            lv_draw_rect(draw_ctx, &rect_dsc, &fill_area);
        }
    }
}

// ==============================================================================
// PUBLIC INTERFACE IMPLEMENTATION
// ==============================================================================

void loading_screen_init_params(int scr_w, int scr_h)
{
    // Initialize orbit parameters
    orbit_cx = scr_w * 0.5f;
    orbit_cy = scr_h * 0.5f;
    float baseR = (scr_w < scr_h ? scr_w : scr_h) * 0.28f;
    orbit_radius[0] = baseR * 1.0f;
    orbit_radius[1] = baseR * 0.7f;
    orbit_radius[2] = baseR * 0.45f;

    // Initial angles
    orbit_angle[0] = 0.0f;
    orbit_angle[1] = 2.0f;
    orbit_angle[2] = 4.0f;

    // Color dots (Red, Green, Blue)
    dots[0].color = lv_color_make(255, 0, 0);
    dots[1].color = lv_color_make(0, 255, 0);
    dots[2].color = lv_color_make(0, 0, 255);

    Serial.println("[LOADING_SCR] Параметри анімації ініціалізовано.");
}

void loading_screen_create(ScreenTransitionCallback_t transition_cb)
{
    // Clean the current screen
    lv_obj_clean(lv_scr_act());

    // Create gradient background object
    gradient_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(gradient_obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(gradient_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(gradient_obj, LV_OBJ_FLAG_SCROLLABLE);
    // Attach custom draw callback
    lv_obj_add_event_cb(gradient_obj, gradient_draw_event_cb, LV_EVENT_DRAW_MAIN, NULL);

    // Make gradient object clickable for touch detection
    lv_obj_add_flag(gradient_obj, LV_OBJ_FLAG_CLICKABLE);
    // Attach the transition callback from main.cpp
    lv_obj_add_event_cb(gradient_obj, transition_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(gradient_obj, transition_cb, LV_EVENT_PRESSED, NULL);
    // Also attach to the screen itself as a fallback
    lv_obj_add_event_cb(lv_scr_act(), transition_cb, LV_EVENT_CLICKED, NULL);

    // Add main title
    main_label = lv_label_create(lv_scr_act());
    lv_label_set_text(main_label, "НЕЙРО");
    lv_obj_set_style_text_font(main_label, &minecraft_ten_96, 0);
    lv_obj_set_style_text_color(main_label, lv_color_white(), 0);
    lv_obj_align(main_label, LV_ALIGN_CENTER, 0, -80);

    // Add subtitle
    sub_label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(sub_label_1, "БЛОК");
    lv_obj_set_style_text_font(sub_label_1, &minecraft_ten_96, 0);
    lv_obj_set_style_text_color(sub_label_1, lv_color_white(), 0);
    lv_obj_align_to(sub_label_1, main_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    Serial.println("[LOADING_SCR] Екран завантаження UI створено.");
}

void loading_screen_update_animation(float dt)
{
    // Update orbit angles using delta time for FPS-independent movement
    for (int i = 0; i < 3; ++i)
    {
        orbit_angle[i] += orbit_speed[i] * dt;

        // Normalize angles to [0, 2π] range (6.2831853f is 2*PI)
        while (orbit_angle[i] > 6.2831853f)
            orbit_angle[i] -= 6.2831853f;
        while (orbit_angle[i] < 0.0f)
            orbit_angle[i] += 6.2831853f;
    }

    // Update dot positions with independent X/Y scaling
    dots[0].x = orbit_cx + cosf(orbit_angle[0]) * orbit_radius[0] * orbit_scale_x;
    dots[0].y = orbit_cy + sinf(orbit_angle[0]) * orbit_radius[0] * orbit_scale_y;

    dots[1].x = orbit_cx + cosf(orbit_angle[1]) * orbit_radius[1] * orbit_scale_x;
    dots[1].y = orbit_cy + sinf(orbit_angle[1]) * orbit_radius[1] * orbit_scale_y;

    dots[2].x = orbit_cx + cosf(orbit_angle[2]) * orbit_radius[2] * orbit_scale_x;
    dots[2].y = orbit_cy + sinf(orbit_angle[2]) * orbit_radius[2] * orbit_scale_y;

    // Trigger redraw for the gradient object
    if (gradient_obj != NULL)
        lv_obj_invalidate(gradient_obj);
}
