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
static const uint32_t LOADING_DURATION = 5000; // 5 seconds
static const uint32_t IDLE_TIMEOUT = 10000;    // 10 seconds idle timeout
static uint32_t last_interaction_time = 0;

// UI об'єкти for loading screen
// static lv_obj_t *loading_screen;
static lv_obj_t *gradient_obj;
static lv_obj_t *main_label;
static lv_obj_t *sub_label_1;

// UI objects for main menu
// static lv_obj_t *menu_screen;
static lv_obj_t *menu_title;
static lv_obj_t *menu_buttons[4];
static lv_obj_t *back_button;

// Forward declarations
static void create_loading_screen();
static void create_main_menu();
static void create_trainer_screen(int trainer_id);
static void screen_touch_event_cb(lv_event_t *event);

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

// #define MAX_DIST_SQ (SOME_CONSTANT_MAX_DISTANCE_EG_SCREEN_SIZE * SOME_CONSTANT_MAX_DISTANCE_EG_SCREEN_SIZE)
#define MAX_DIST 800.0f

float channel_value(float dist)
{
    // Normalize distance to [0.0, 1.0] range based on MAX_DIST
    float norm_dist = fminf(dist / MAX_DIST, 1.0f);
    // Apply non-linear scaling (squared) for smoother gradient
    return 1.0f - sqrtf(norm_dist);
}

#define SATURATION_FACTOR 2.0f

static void saturate_color_floats(float *r, float *g, float *b)
{
    float r_f = *r;
    float g_f = *g;
    float b_f = *b;

    // 1. Calculate the average luminance (gray component) in [0.0f, 1.0f]
    float avg = (r_f + g_f + b_f) / 3.0f;

    // 2. Saturate: C_new = avg + FACTOR * (C_old - avg)
    // This increases the distance of each component from the gray average.
    float r_new = fmaf(SATURATION_FACTOR, (r_f - avg), avg);
    float g_new = fmaf(SATURATION_FACTOR, (g_f - avg), avg);
    float b_new = fmaf(SATURATION_FACTOR, (b_f - avg), avg);

    // 3. Clamp the resulting values to the required [0.0f, 1.0f] range
    *r = fmaxf(0.0f, fminf(1.0f, r_new));
    *g = fmaxf(0.0f, fminf(1.0f, g_new));
    *b = fmaxf(0.0f, fminf(1.0f, b_new));
}

static lv_color_t distance_color_map(int px, int py)
{
    float x = (float)px, y = (float)py;

    // Compute actual distances
    float dx0 = x - dots[0].x, dy0 = y - dots[0].y;
    float dist0 = sqrtf(dx0 * dx0 + dy0 * dy0);

    float dx1 = x - dots[1].x, dy1 = y - dots[1].y;
    float dist1 = sqrtf(dx1 * dx1 + dy1 * dy1);

    float dx2 = x - dots[2].x, dy2 = y - dots[2].y;
    float dist2 = sqrtf(dx2 * dx2 + dy2 * dy2);

    // Apply the formula: C = 1 - (dist / MAX_DIST)
    float r_f = channel_value(dist0); // Red based on point 1
    float g_f = channel_value(dist1); // Green based on point 2
    float b_f = channel_value(dist2); // Blue based on point 3

    saturate_color_floats(&r_f, &g_f, &b_f);

    // Convert float [0.0, 1.0] to uint8_t [0, 255]
    // lv_color_make assumes 8-bit components for its *input*, though the
    // internal representation may vary (e.g., 5, 6, 5 bits).
    // We'll map to 0-255 then let lv_color_make handle the packing.
    uint8_t r = (uint8_t)(r_f * 255.0f);
    uint8_t g = (uint8_t)(g_f * 255.0f);
    uint8_t b = (uint8_t)(b_f * 255.0f);

    // If LVGL is configured for less than 8 bits per color,
    // lv_color_make will automatically truncate/shift the 8-bit input.
    return lv_color_make(r, g, b);
}

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
            // lv_color_t color = interpolate_color_idw_fast(screen_x, screen_y);
            lv_color_t color = distance_color_map(screen_x, screen_y);

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

    // Check for idle timeout (return to loading screen if no interaction for 10 seconds)
    if (current_state == STATE_MAIN_MENU && (now - last_interaction_time) >= IDLE_TIMEOUT)
    {
        Serial.println("[ТАЙМ-АУТ] Досягнуто тайм-аут бездіяльності - повернення до екрану завантаження");
        current_state = STATE_LOADING;
        state_start_time = now;
        create_loading_screen();
        return;
    }

    // Only run animation in loading state
    if (current_state != STATE_LOADING)
        return;

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
    if (gradient_obj != NULL)
        lv_obj_invalidate(gradient_obj);
}

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
} // Back button event handler
static void back_button_event_cb(lv_event_t *event)
{
    Serial.println("[НАЗАД] Натиснуто кнопку назад - повернення до головного меню");
    last_interaction_time = lv_tick_get();
    current_state = STATE_MAIN_MENU;
    state_start_time = lv_tick_get();
    create_main_menu();
}

// Screen touch event handler - switches from loading to main menu
static void screen_touch_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    Serial.printf("[ДОТИК] Отримано подію: %d, Поточний стан: %d\n", code, current_state);

    if (current_state == STATE_LOADING)
    {
        Serial.println("[ДОТИК] Перехід від завантаження до головного меню");
        current_state = STATE_MAIN_MENU;
        state_start_time = lv_tick_get();
        last_interaction_time = lv_tick_get();
        create_main_menu();
    }
    else
    {
        Serial.printf("[ДОТИК] Дотик проігноровано - не в стані завантаження (поточний: %d)\n", current_state);
    }
}

// Create loading screen with orbital animation
static void create_loading_screen()
{
    lv_obj_clean(lv_scr_act());

    // Create gradient background object
    gradient_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(gradient_obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(gradient_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(gradient_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(gradient_obj, gradient_draw_event_cb, LV_EVENT_DRAW_MAIN, NULL);

    // Make gradient object clickable for touch detection
    lv_obj_add_flag(gradient_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(gradient_obj, screen_touch_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(gradient_obj, screen_touch_event_cb, LV_EVENT_PRESSED, NULL);

    // Add main title with 96px font
    main_label = lv_label_create(lv_scr_act());
    lv_label_set_text(main_label, "НЕЙРО");
    lv_obj_set_style_text_font(main_label, &minecraft_ten_96, 0);
    lv_obj_set_style_text_color(main_label, lv_color_white(), 0);
    lv_obj_align(main_label, LV_ALIGN_CENTER, 0, -80);

    // Add subtitle with 96px font
    sub_label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(sub_label_1, "БЛОК");
    lv_obj_set_style_text_font(sub_label_1, &minecraft_ten_96, 0);
    lv_obj_set_style_text_color(sub_label_1, lv_color_white(), 0);
    lv_obj_align_to(sub_label_1, main_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    // Add touch events to screen to detect user interaction
    // Use multiple event types to ensure touch detection works
    lv_obj_add_event_cb(lv_scr_act(), screen_touch_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(lv_scr_act(), screen_touch_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(lv_scr_act(), screen_touch_event_cb, LV_EVENT_PRESS_LOST, NULL);

    Serial.println("[НАЛАГОДЖЕННЯ] Екран завантаження створено з зареєстрованими подіями дотику");
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

        // Button label with 48px font
        lv_obj_t *label = lv_label_create(menu_buttons[i]);
        lv_label_set_text(label, trainer_names[i]);
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
    Serial.println("Базується на перевіреній ESP-BSP RGB подвійний буфер конфігурації");

    Serial.println("Ініціалізація плати з конфігурацією без розривів...");
    Board *board = new Board();
    board->init();

// Configure anti-tearing RGB double-buffer mode (ESP-BSP style)
#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();

    Serial.println("Налаштування RGB подвійного буфера для усунення розривів...");
    // Enable RGB double-buffer mode: 2 frame buffers for ping-pong operation
    lcd->configFrameBufferNumber(2); // RGB double-buffer mode

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();

    // Configure bounce buffer for ESP32-S3 RGB LCD (essential for anti-tearing)
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
    {
        Serial.println("Налаштування RGB буфера відскоку для ESP32-S3...");
        // Bounce buffer size: screen_width * height_fraction (ESP-BSP recommendation)
        // This greatly reduces tearing artifacts on ESP32-S3 RGB displays
        int bounce_buffer_height = lcd->getFrameHeight() / 10; // 48 pixels for 480px height
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

    // Using custom Minecraft 96px font with Cyrillic support
    // Font includes Unicode range: ASCII (0x0020-0x007F) + Cyrillic (0x0410-0x044F)
    Serial.println("Завантаження користувацького кириличного шрифту: minecraft_ten_96");
    Serial.printf("Вказівник шрифту: %p\n", &minecraft_ten_96);
    Serial.printf("Висота лінії шрифту: %d\n", minecraft_ten_96.line_height);
    Serial.println("Готовий до відображення кириличного тексту з великим 96px шрифтом...");
    lvgl_port_lock(-1);

    // Get actual screen dimensions
    SCR_W = lv_disp_get_hor_res(NULL);
    SCR_H = lv_disp_get_ver_res(NULL);

    Serial.printf("Роздільність екрану: %dx%d\n", SCR_W, SCR_H);

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

    // Initialize app state
    current_state = STATE_LOADING;
    state_start_time = lv_tick_get();
    last_interaction_time = state_start_time;

    Serial.println("[НАЛАГОДЖЕННЯ] Додаток ініціалізовано - Запуск в стані ЗАВАНТАЖЕННЯ");
    Serial.printf("[НАЛАГОДЖЕННЯ] Події дотику: CLICKED=%d, PRESSED=%d, PRESS_LOST=%d\n",
                  LV_EVENT_CLICKED, LV_EVENT_PRESSED, LV_EVENT_PRESS_LOST);

    // Create initial loading screen
    create_loading_screen();

    // Start animation timer for state management and animation
    animation_timer = lv_timer_create(animation_timer_cb, FRAME_MS, NULL);

    lvgl_port_unlock();

    Serial.println("=== КОНФІГУРАЦІЯ БЕЗ РОЗРИВІВ ЗАВЕРШЕНА ===");
    Serial.println("RGB LCD тепер повинен відображати плавну анімацію без розривів!");
    Serial.println("Режим: RGB подвійний буфер + LVGL повне оновлення (перевірене ESP-BSP рішення)");
}

void loop()
{
    delay(5);
}
