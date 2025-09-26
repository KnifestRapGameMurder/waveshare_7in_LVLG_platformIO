#include "animation.h"
#include "app_config.h"
#include "ui_screens.h"
#include <Arduino.h>
#include <math.h>

// Animation variables
float orbit_cx = 0.0f, orbit_cy = 0.0f;
float orbit_angle[3] = {0.0f, 0.0f, 0.0f};
float orbit_speed[3] = {2.5f, -3.2f, 4.0f}; // rad/s - faster for visible movement
float orbit_radius[3] = {0.0f, 0.0f, 0.0f};
float orbit_scale_x = 2.0f; // X-axis scale multiplier
float orbit_scale_y = 1.5f; // Y-axis scale multiplier
ColorDot dots[3];
lv_timer_t *animation_timer = nullptr;

void animation_init(int32_t screen_w, int32_t screen_h)
{
    // Initialize orbit parameters
    orbit_cx = screen_w * 0.5f;
    orbit_cy = screen_h * 0.5f;
    float baseR = (screen_w < screen_h ? screen_w : screen_h) * 0.28f;
    orbit_radius[0] = baseR * 1.0f;
    orbit_radius[1] = baseR * 0.7f;
    orbit_radius[2] = baseR * 0.45f;

    // Initial angles
    orbit_angle[0] = 0.0f;
    orbit_angle[1] = 2.0f;
    orbit_angle[2] = 4.0f;

    // Color dots
    dots[0].color = lv_color_make(255, 0, 0); // Red
    dots[1].color = lv_color_make(0, 255, 0); // Green
    dots[2].color = lv_color_make(0, 0, 255); // Blue

    // Start animation timer
    animation_timer = lv_timer_create(animation_timer_cb, FRAME_MS, NULL);

    Serial.println("Animation system initialized");
}

void animation_update()
{
    // Animation is handled by timer, so this can be empty
    // or we could add manual frame updates here if needed
}

lv_color_t interpolate_color_idw_fast(int px, int py)
{
    float x = (float)px, y = (float)py;

    // Compute distances squared (avoid sqrt for performance)
    float dx0 = x - dots[0].x, dy0 = y - dots[0].y;
    float dx1 = x - dots[1].x, dy1 = y - dots[1].y;
    float dx2 = x - dots[2].x, dy2 = y - dots[2].y;

    float d0_sq = dx0 * dx0 + dy0 * dy0;
    float d1_sq = dx1 * dx1 + dy1 * dy1;
    float d2_sq = dx2 * dx2 + dy2 * dy2;

    // Early exit for close matches
    const float EPSILON_SQ = 25.0f;
    if (d0_sq < EPSILON_SQ)
        return dots[0].color;
    if (d1_sq < EPSILON_SQ)
        return dots[1].color;
    if (d2_sq < EPSILON_SQ)
        return dots[2].color;

    // Use inverse distance weighting
    float w0 = 1.0f / (d0_sq + 1.0f);
    float w1 = 1.0f / (d1_sq + 1.0f);
    float w2 = 1.0f / (d2_sq + 1.0f);

    float wsum = w0 + w1 + w2;
    w0 /= wsum;
    w1 /= wsum;
    w2 /= wsum;

    // Fast color extraction and interpolation
    uint8_t r0 = LV_COLOR_GET_R(dots[0].color) << 3;
    uint8_t g0 = LV_COLOR_GET_G(dots[0].color) << 2;
    uint8_t b0 = LV_COLOR_GET_B(dots[0].color) << 3;

    uint8_t r1 = LV_COLOR_GET_R(dots[1].color) << 3;
    uint8_t g1 = LV_COLOR_GET_G(dots[1].color) << 2;
    uint8_t b1 = LV_COLOR_GET_B(dots[1].color) << 3;

    uint8_t r2 = LV_COLOR_GET_R(dots[2].color) << 3;
    uint8_t g2 = LV_COLOR_GET_G(dots[2].color) << 2;
    uint8_t b2 = LV_COLOR_GET_B(dots[2].color) << 3;

    uint8_t r = (uint8_t)(w0 * r0 + w1 * r1 + w2 * r2);
    uint8_t g = (uint8_t)(w0 * g0 + w1 * g1 + w2 * g2);
    uint8_t b = (uint8_t)(w0 * b0 + w1 * b1 + w2 * b2);

    return lv_color_make(r, g, b);
}

void gradient_draw_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);

    // Get drawing area
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_coord_t obj_w = lv_area_get_width(&coords);
    lv_coord_t obj_h = lv_area_get_height(&coords);

    // Create gradient by drawing smaller blocks
    const int STEP = 32;

    for (lv_coord_t y = 0; y < obj_h; y += STEP)
    {
        for (lv_coord_t x = 0; x < obj_w; x += STEP)
        {
            int screen_x = coords.x1 + x + STEP / 2;
            int screen_y = coords.y1 + y + STEP / 2;

            lv_color_t color = interpolate_color_idw_fast(screen_x, screen_y);

            lv_area_t fill_area;
            fill_area.x1 = coords.x1 + x;
            fill_area.y1 = coords.y1 + y;
            fill_area.x2 = coords.x1 + x + STEP - 1;
            fill_area.y2 = coords.y1 + y + STEP - 1;

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

void animation_timer_cb(lv_timer_t *timer)
{
    static uint32_t last_time = 0;
    uint32_t now = lv_tick_get();

    // Check for idle timeout
    if (current_state == STATE_MAIN_MENU && (now - last_interaction_time) >= IDLE_TIMEOUT)
    {
        Serial.println("[TIMEOUT] Idle timeout reached - returning to loading screen");
        current_state = STATE_LOADING;
        state_start_time = now;
        create_loading_screen();
        return;
    }

    // Only run animation in loading state
    if (current_state != STATE_LOADING)
        return;

    // Proper delta time calculation
    float dt = (now - last_time) / 1000.0f;
    if (last_time == 0 || dt < 0.001f || dt > 0.1f)
        dt = 0.033f; // 30 FPS fallback
    last_time = now;

    // Update orbit angles
    for (int i = 0; i < 3; ++i)
    {
        orbit_angle[i] += orbit_speed[i] * dt;
        while (orbit_angle[i] > 6.2831853f)
            orbit_angle[i] -= 6.2831853f;
        while (orbit_angle[i] < 0.0f)
            orbit_angle[i] += 6.2831853f;
    }

    // Update dot positions
    dots[0].x = orbit_cx + cosf(orbit_angle[0]) * orbit_radius[0] * orbit_scale_x;
    dots[0].y = orbit_cy + sinf(orbit_angle[0]) * orbit_radius[0] * orbit_scale_y;

    dots[1].x = orbit_cx + cosf(orbit_angle[1]) * orbit_radius[1] * orbit_scale_x;
    dots[1].y = orbit_cy + sinf(orbit_angle[1]) * orbit_radius[1] * orbit_scale_y;

    dots[2].x = orbit_cx + cosf(orbit_angle[2]) * orbit_radius[2] * orbit_scale_x;
    dots[2].y = orbit_cy + sinf(orbit_angle[2]) * orbit_radius[2] * orbit_scale_y;

    // Trigger redraw
    if (gradient_obj != NULL)
        lv_obj_invalidate(gradient_obj);
}