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

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// ---------- ANTI-TEARING CONFIGURATION (Based on ESP-BSP proven solutions) ----------
// RGB double-buffer + LVGL full-refresh mode (recommended for ESP32-S3)
// This eliminates tearing by using hardware-level double buffering

static const uint32_t TARGET_FPS = 30;
static const uint32_t FRAME_MS = 1000 / TARGET_FPS;

// UI об'єкти
static lv_obj_t *gradient_obj;
static lv_obj_t *main_label;
static lv_obj_t *sub_label_1;
static lv_obj_t *sub_label_2;
static lv_obj_t *desc_label_1;
static lv_obj_t *desc_label_2;

// Параметри екрану та анімації
static int32_t SCR_W = 800, SCR_H = 480;
static lv_timer_t *animation_timer;

// Параметри орбіт для трьох кольорових точок
struct ColorDot
{
    float x, y;
    lv_color_t color;
};

static float orbit_cx = 0.0f, orbit_cy = 0.0f;
static float orbit_angle[3] = {0.0f, 0.0f, 0.0f};
static float orbit_speed[3] = {2.5f, -3.2f, 4.0f}; // rad/s - faster for visible movement
static float orbit_radius[3] = {0.0f, 0.0f, 0.0f};

// Scale parameters for orbital simulation
static float orbit_scale_x = 2.0f; // X-axis scale multiplier (1.0 = normal, >1.0 = wider, <1.0 = narrower)
static float orbit_scale_y = 1.5f; // Y-axis scale multiplier (1.0 = normal, >1.0 = taller, <1.0 = shorter)

static ColorDot dots[3];

// Optimized color interpolation function
static lv_color_t interpolate_color_idw_fast(int px, int py)
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

    // Fast color extraction
    uint8_t r0 = LV_COLOR_GET_R(dots[0].color) << 3;
    uint8_t g0 = LV_COLOR_GET_G(dots[0].color) << 2;
    uint8_t b0 = LV_COLOR_GET_B(dots[0].color) << 3;

    uint8_t r1 = LV_COLOR_GET_R(dots[1].color) << 3;
    uint8_t g1 = LV_COLOR_GET_G(dots[1].color) << 2;
    uint8_t b1 = LV_COLOR_GET_B(dots[1].color) << 3;

    uint8_t r2 = LV_COLOR_GET_R(dots[2].color) << 3;
    uint8_t g2 = LV_COLOR_GET_G(dots[2].color) << 2;
    uint8_t b2 = LV_COLOR_GET_B(dots[2].color) << 3;

    // Fast interpolation
    uint8_t r = (uint8_t)(w0 * r0 + w1 * r1 + w2 * r2);
    uint8_t g = (uint8_t)(w0 * g0 + w1 * g1 + w2 * g2);
    uint8_t b = (uint8_t)(w0 * b0 + w1 * b1 + w2 * b2);

    return lv_color_make(r, g, b);
}

// Custom draw event callback for smooth gradient rendering
// This uses LVGL's proper rendering pipeline with anti-tearing
static void gradient_draw_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);

    // Get drawing area
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_coord_t obj_w = lv_area_get_width(&coords);
    lv_coord_t obj_h = lv_area_get_height(&coords);

    // Create gradient by drawing smaller blocks for good performance/quality balance
    const int STEP = 32; // Small blocks for smooth appearance but good performance

    for (lv_coord_t y = 0; y < obj_h; y += STEP)
    {
        for (lv_coord_t x = 0; x < obj_w; x += STEP)
        {
            // Calculate absolute screen coordinates (center of the block)
            int screen_x = coords.x1 + x + STEP / 2;
            int screen_y = coords.y1 + y + STEP / 2;

            // Get interpolated color for this position
            lv_color_t color = interpolate_color_idw_fast(screen_x, screen_y);

            // Draw filled rectangle at this position using LVGL draw functions
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

// Animation timer using proper LVGL invalidation (works with anti-tearing)
static void animation_timer_cb(lv_timer_t *timer)
{
    static uint32_t last_time = 0;
    uint32_t now = lv_tick_get();

    // Proper delta time calculation for FPS-independent animation
    float dt = (now - last_time) / 1000.0f;         // Convert ms to seconds
    if (last_time == 0 || dt < 0.001f || dt > 0.1f) // Handle first frame and extreme values
        dt = 0.033f;                                // 30 FPS fallback
    last_time = now;

    // Update orbit angles using delta time for FPS-independent movement
    for (int i = 0; i < 3; ++i)
    {
        orbit_angle[i] += orbit_speed[i] * dt;

        // Normalize angles to [0, 2π] range
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

    // Trigger redraw using LVGL's proper invalidation
    // This works correctly with RGB double-buffer anti-tearing
    lv_obj_invalidate(gradient_obj);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("=== ESP32-S3 RGB LCD Anti-Tearing Solution ===");
    Serial.println("Based on ESP-BSP proven RGB double-buffer configuration");

    Serial.println("Initializing board with anti-tearing configuration...");
    Board *board = new Board();
    board->init();

// Configure anti-tearing RGB double-buffer mode (ESP-BSP style)
#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();

    Serial.println("Configuring RGB double-buffer for anti-tearing...");
    // Enable RGB double-buffer mode: 2 frame buffers for ping-pong operation
    lcd->configFrameBufferNumber(2); // RGB double-buffer mode

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();

    // Configure bounce buffer for ESP32-S3 RGB LCD (essential for anti-tearing)
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
    {
        Serial.println("Configuring RGB bounce buffer for ESP32-S3...");
        // Bounce buffer size: screen_width * height_fraction (ESP-BSP recommendation)
        // This greatly reduces tearing artifacts on ESP32-S3 RGB displays
        int bounce_buffer_height = lcd->getFrameHeight() / 10; // 48 pixels for 480px height
        int bounce_buffer_size = lcd->getFrameWidth() * bounce_buffer_height;

        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(bounce_buffer_size);

        Serial.printf("RGB bounce buffer configured: %dx%d pixels\n",
                      lcd->getFrameWidth(), bounce_buffer_height);
    }
#endif
#endif

    assert(board->begin());
    Serial.println("Board initialized with anti-tearing RGB configuration!");

    Serial.println("Initializing LVGL with full-refresh mode...");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI with anti-tearing gradient...");

    // Using custom Minecraft 96px font with Cyrillic support
    // Font includes Unicode range: ASCII (0x0020-0x007F) + Cyrillic (0x0410-0x044F)
    Serial.println("Loading custom Cyrillic font: minecraft_ten_96");
    Serial.printf("Font pointer: %p\n", &minecraft_ten_96);
    Serial.printf("Font line height: %d\n", minecraft_ten_96.line_height);
    Serial.println("Ready to display Cyrillic text with larger 96px font...");
    lvgl_port_lock(-1);

    // Get actual screen dimensions
    SCR_W = lv_disp_get_hor_res(NULL);
    SCR_H = lv_disp_get_ver_res(NULL);

    Serial.printf("Screen resolution: %dx%d\n", SCR_W, SCR_H);

    // Initialize orbit parameters
    orbit_cx = SCR_W * 0.5f;
    orbit_cy = SCR_H * 0.5f;
    float baseR = (SCR_W < SCR_H ? SCR_W : SCR_H) * 0.28f;
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

    // Create background gradient object that uses proper LVGL drawing
    gradient_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(gradient_obj, SCR_W, SCR_H);
    lv_obj_set_pos(gradient_obj, 0, 0);
    lv_obj_set_style_border_width(gradient_obj, 0, 0);
    lv_obj_set_style_radius(gradient_obj, 0, 0);
    lv_obj_set_style_bg_opa(gradient_obj, LV_OPA_TRANSP, 0); // Transparent, we draw manually

    // Add custom draw event for gradient rendering
    lv_obj_add_event_cb(gradient_obj, gradient_draw_event_cb, LV_EVENT_DRAW_MAIN, NULL);

    // Create text labels with Cyrillic text using 96px Minecraft font
    main_label = lv_label_create(lv_scr_act());
    lv_label_set_text(main_label, "НЕЙРО");                       // Cyrillic text
    lv_obj_set_style_text_font(main_label, &minecraft_ten_96, 0); // Custom Minecraft 96px Cyrillic font
    lv_obj_set_style_text_color(main_label, lv_color_white(), 0);
    lv_obj_align(main_label, LV_ALIGN_CENTER, 0, -80); // Adjusted position for larger font

    sub_label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(sub_label_1, "БЛОК");                        // Cyrillic text
    lv_obj_set_style_text_font(sub_label_1, &minecraft_ten_96, 0); // Custom Minecraft 96px Cyrillic font
    lv_obj_set_style_text_color(sub_label_1, lv_color_white(), 0);
    lv_obj_align_to(sub_label_1, main_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0); // Adjusted spacing for larger font

    // sub_label_2 = lv_label_create(lv_scr_act());
    // lv_label_set_text_fmt(sub_label_2, "ESP32_Display_Panel(%d.%d.%d)",
    //                       ESP_PANEL_VERSION_MAJOR, ESP_PANEL_VERSION_MINOR, ESP_PANEL_VERSION_PATCH);
    // lv_obj_set_style_text_font(sub_label_2, &lv_font_montserrat_16, 0);
    // lv_obj_set_style_text_color(sub_label_2, lv_color_white(), 0);
    // lv_obj_align_to(sub_label_2, sub_label_1, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);

    // desc_label_1 = lv_label_create(lv_scr_act());
    // lv_label_set_text_fmt(desc_label_1, "LVGL(%d.%d.%d) + RGB Anti-Tearing",
    //                       LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    // lv_obj_set_style_text_font(desc_label_1, &lv_font_montserrat_16, 0);
    // lv_obj_set_style_text_color(desc_label_1, lv_color_white(), 0);
    // lv_obj_align_to(desc_label_1, sub_label_2, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // desc_label_2 = lv_label_create(lv_scr_act());
    // lv_label_set_text(desc_label_2, "RGB Double-Buffer + LVGL Full-Refresh Mode");
    // lv_obj_set_style_text_font(desc_label_2, &lv_font_montserrat_16, 0);
    // lv_obj_set_style_text_color(desc_label_2, lv_color_white(), 0);
    // lv_obj_align(desc_label_2, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Start animation timer
    animation_timer = lv_timer_create(animation_timer_cb, FRAME_MS, NULL);

    lvgl_port_unlock();

    Serial.println("=== ANTI-TEARING CONFIGURATION COMPLETE ===");
    Serial.println("RGB LCD should now display smooth animation without tearing!");
    Serial.println("Mode: RGB double-buffer + LVGL full-refresh (ESP-BSP proven solution)");
}

void loop()
{
    delay(5);
}
