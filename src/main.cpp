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

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// External font declarations
extern "C"
{
    extern const lv_font_t minecraft_ten_48;
    extern const lv_font_t minecraft_ten_96;
}

// Forward declarations
static void create_loading_screen();
static void create_main_menu();
static void create_trainer_screen(int trainer_id);
static void create_console_screen();
static void screen_touch_event_cb(lv_event_t *event);

// UART callback functions
static void uart_response_callback(uint8_t cmd, uint8_t *data, uint8_t len);
static void uart_error_callback(uint8_t error_code, const char *message);

// Console functions
static void console_add_log(const char *message);
static void console_back_event_cb(lv_event_t *event);
static void console_clear_event_cb(lv_event_t *event);

// ---------- ANTI-TEARING CONFIGURATION (Based on ESP-BSP proven solutions) ----------
// RGB double-buffer + LVGL full-refresh mode (recommended for ESP32-S3)
// This eliminates tearing by using hardware-level double buffering

static const uint32_t TARGET_FPS = 30;
static const uint32_t FRAME_MS = 1000 / TARGET_FPS;

// ---------- UART COMMUNICATION SETUP ----------
// ESP32-S3 pins: TX2=GPIO43, RX2=GPIO44
HardwareSerial peripheralUART(2);
UARTProtocol *uart_protocol;

// Application states
enum AppState
{
    STATE_LOADING,   // Loading screen with animation
    STATE_MAIN_MENU, // Main menu with 4 training buttons
    STATE_TRAINER_1, // Training module 1
    STATE_TRAINER_2, // Training module 2
    STATE_TRAINER_3, // Training module 3
    STATE_TRAINER_4, // Training module 4
    STATE_CONSOLE    // Debug console window
};

static AppState current_state = STATE_LOADING;
static uint32_t state_start_time = 0;
static const uint32_t LOADING_DURATION = 5000; // 5 seconds
static const uint32_t IDLE_TIMEOUT = 10000;    // 10 seconds idle timeout
static uint32_t last_interaction_time = 0;

// UI об'єкти for loading screen
static lv_obj_t *loading_screen;
static lv_obj_t *gradient_obj;
static lv_obj_t *main_label;
static lv_obj_t *sub_label_1;

// UI objects for main menu
static lv_obj_t *menu_screen;
static lv_obj_t *menu_title;
static lv_obj_t *menu_buttons[4];
static lv_obj_t *back_button;

// UI objects for console window
static lv_obj_t *console_screen;
static lv_obj_t *console_title;
static lv_obj_t *console_textarea;
static lv_obj_t *console_back_btn;
static lv_obj_t *console_clear_btn;

// Forward declarations
static void create_loading_screen();
static void create_main_menu();
static void create_trainer_screen(int trainer_id);
static void screen_touch_event_cb(lv_event_t *event);

// UART callback functions
static void uart_response_callback(uint8_t cmd, uint8_t *data, uint8_t len);
static void uart_error_callback(uint8_t error_code, const char *message);

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

    // Switch to selected trainer or console
    if (trainer_id == 3)
    {
        // Console button (4th button)
        current_state = STATE_CONSOLE;
        state_start_time = lv_tick_get();
        create_console_screen();
    }
    else
    {
        // Regular trainer button
        current_state = (AppState)(STATE_TRAINER_1 + trainer_id);
        state_start_time = lv_tick_get();
        create_trainer_screen(trainer_id);

        // Send LED commands to peripheral for visual feedback
        if (uart_protocol && uart_protocol->is_connected())
        {
            // Turn on LED corresponding to selected trainer
            uart_protocol->led_set(trainer_id, true, 255);

            // Set RGB LED to match trainer color
            switch (trainer_id)
            {
            case 0:
                uart_protocol->led_rgb(0, 46, 139, 87);
                break; // Sea Green
            case 1:
                uart_protocol->led_rgb(0, 65, 105, 225);
                break; // Royal Blue
            case 2:
                uart_protocol->led_rgb(0, 220, 20, 60);
                break; // Crimson Red
            }

            // Start training session
            uart_protocol->training_start(trainer_id);
        }
    }
} // Back button event handler
static void back_button_event_cb(lv_event_t *event)
{
    Serial.println("[НАЗАД] Натиснуто кнопку назад - повернення до головного меню");
    last_interaction_time = lv_tick_get();
    current_state = STATE_MAIN_MENU;
    state_start_time = lv_tick_get();
    create_main_menu();

    // Stop training session and turn off LEDs
    if (uart_protocol && uart_protocol->is_connected())
    {
        uart_protocol->training_stop();

        // Turn off all LEDs
        for (int i = 0; i < 4; i++)
        {
            uart_protocol->led_set(i, false, 0);
        }
        uart_protocol->led_rgb(0, 0, 0, 0); // Turn off RGB LED
    }
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

    // Create 4 trainer buttons + 1 console button
    const char *trainer_names[] = {
        "TRAINER 1",
        "TRAINER 2",
        "TRAINER 3",
        "CONSOLE"};

    // Different colors for each button
    uint32_t button_colors[] = {
        0x2E8B57, // Sea Green
        0x4169E1, // Royal Blue
        0xDC143C, // Crimson Red
        0x9932CC  // Dark Orchid (Console)
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

    // Initialize UART communication with peripheral driver
    Serial.println("\n=== ІНІЦІАЛІЗАЦІЯ UART ПРОТОКОЛУ ===");
    uart_protocol = new UARTProtocol(&peripheralUART);

    if (uart_protocol->begin(115200))
    {
        uart_protocol->set_response_callback(uart_response_callback);
        uart_protocol->set_error_callback(uart_error_callback);

        Serial.println("UART протокол ініціалізовано на GPIO43/44");
        Serial.println("Спроба підключення до периферійного драйвера...");

        if (uart_protocol->connect(5000))
        {
            Serial.println("✅ Успішно підключено до периферійного драйвера!");

            // Test peripheral connection
            uart_protocol->ping();
            uart_protocol->get_status();

            // Enable automatic sensor reporting (every 5 seconds)
            uart_protocol->sensor_auto_enable(0, 5000);  // Temperature sensor
            uart_protocol->sensor_auto_enable(1, 10000); // Light sensor
        }
        else
        {
            Serial.println("⚠️ Не вдалося підключитися до периферійного драйвера");
            Serial.println("Програма продовжить роботу без зовнішніх периферійних пристроїв");
        }
    }
    else
    {
        Serial.println("❌ Помилка ініціалізації UART протоколу");
    }
    Serial.println("=== UART ПРОТОКОЛ ГОТОВИЙ ===\n");
}

// UART callback implementations
static void uart_response_callback(uint8_t cmd, uint8_t *data, uint8_t len)
{
    Serial.printf("[UART CALLBACK] Отримано відповідь: CMD=0x%02X, LEN=%d\n", cmd, len);
    char log_buffer[256];

    switch (cmd)
    {
    case CMD_BTN_STATE:
        if (len >= 2)
        {
            uint8_t btn_id = data[0];
            uint8_t btn_state = data[1];
            Serial.printf("[КНОПКА] Кнопка %d: %s\n", btn_id,
                          btn_state == 1 ? "НАТИСНУТА" : "ВІДПУЩЕНА");

            snprintf(log_buffer, sizeof(log_buffer), "Button %d: %s",
                     btn_id, btn_state == 1 ? "PRESSED" : "RELEASED");
            console_add_log(log_buffer);

            // Handle button press - could trigger menu actions
            if (btn_state == 1)
            {
                last_interaction_time = lv_tick_get();

                // Map hardware buttons to training functions
                if (current_state == STATE_MAIN_MENU && btn_id < 4)
                {
                    current_state = (AppState)(STATE_TRAINER_1 + btn_id);
                    create_trainer_screen(btn_id);
                }
            }
        }
        break;

    case CMD_SENSOR_DATA:
        if (len >= 3)
        {
            uint8_t sensor_id = data[0];
            uint8_t sensor_type = data[1];
            Serial.printf("[СЕНСОР] Сенсор %d (тип %d): дані отримані\n", sensor_id, sensor_type);

            char log_buffer[256];
            // Process sensor data based on type
            switch (sensor_type)
            {
            case 0: // Temperature
                if (len >= 5)
                {
                    int16_t temp = (data[2] << 8) | data[3];
                    Serial.printf("[ТЕМПЕРАТУРА] %d.%d°C\n", temp / 10, temp % 10);
                    snprintf(log_buffer, sizeof(log_buffer), "Temperature: %d.%d°C", temp / 10, temp % 10);
                    console_add_log(log_buffer);
                }
                break;
            case 1: // Humidity
                if (len >= 4)
                {
                    uint8_t humidity = data[2];
                    Serial.printf("[ВОЛОГІСТЬ] %d%%\n", humidity);
                    snprintf(log_buffer, sizeof(log_buffer), "Humidity: %d%%", humidity);
                    console_add_log(log_buffer);
                }
                break;
            case 3: // Light
                if (len >= 5)
                {
                    uint16_t light = (data[2] << 8) | data[3];
                    Serial.printf("[ОСВІТЛЕННЯ] %d lux\n", light);
                    snprintf(log_buffer, sizeof(log_buffer), "Light: %d lux", light);
                    console_add_log(log_buffer);
                }
                break;
            case 4: // Hall sensor (magnetic)
                if (len >= 4)
                {
                    uint8_t hall_state = data[2];
                    Serial.printf("[МАГНІТНИЙ СЕНСОР] Стан: %s\n", hall_state ? "ВИЯВЛЕНО" : "НЕ ВИЯВЛЕНО");
                    snprintf(log_buffer, sizeof(log_buffer), "Magnet: %s", hall_state ? "DETECTED" : "NOT DETECTED");
                    console_add_log(log_buffer);
                }
                break;
            }
        }
        break;

    case CMD_TRAINING_STATUS:
        if (len >= 2)
        {
            uint8_t training_id = data[0];
            uint8_t progress = data[1];
            Serial.printf("[ТРЕНУВАННЯ] Тренажер %d: прогрес %d%%\n", training_id + 1, progress);
            snprintf(log_buffer, sizeof(log_buffer), "Trainer %d: %d%%", training_id + 1, progress);
            console_add_log(log_buffer);
        }
        break;

    case CMD_PONG:
        Serial.println("[З'ЄДНАННЯ] Периферійний пристрій відповів на ping");
        console_add_log("Connection active (PONG)");
        break;

    default:
        Serial.printf("[UART] Невідома команда: 0x%02X\n", cmd);
        snprintf(log_buffer, sizeof(log_buffer), "Unknown command: 0x%02X", cmd);
        console_add_log(log_buffer);
        break;
    }
}

static void uart_error_callback(uint8_t error_code, const char *message)
{
    Serial.printf("[UART ПОМИЛКА] Код: 0x%02X, Повідомлення: %s\n", error_code, message);

    char log_buffer[256];
    snprintf(log_buffer, sizeof(log_buffer), "UART Error: %s (0x%02X)", message, error_code);
    console_add_log(log_buffer);

    // Handle different error types
    switch (error_code)
    {
    case ERR_TIMEOUT:
        Serial.println("[UART] Спроба перепідключення...");
        // Could attempt reconnection here
        break;
    case ERR_CRC_MISMATCH:
        Serial.println("[UART] Помилка контрольної суми - можливо перешкоди");
        break;
    case ERR_HARDWARE:
        Serial.println("[UART] Помилка обладнання периферії");
        break;
    }
}

// Console implementation
static void console_add_log(const char *message)
{
    if (console_textarea == NULL)
        return;

    // Get current time for timestamp
    uint32_t now = millis();
    uint32_t seconds = now / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "[%02lu:%02lu:%02lu] ",
             hours % 24, minutes % 60, seconds % 60);

    // Add timestamped message to console
    const char *current_text = lv_textarea_get_text(console_textarea);
    char *new_text = (char *)malloc(strlen(current_text) + strlen(timestamp) + strlen(message) + 10);

    if (new_text)
    {
        strcpy(new_text, current_text);
        strcat(new_text, timestamp);
        strcat(new_text, message);
        strcat(new_text, "\n");

        lv_textarea_set_text(console_textarea, new_text);

        // Auto-scroll to bottom
        lv_obj_scroll_to_y(console_textarea, LV_COORD_MAX, LV_ANIM_ON);

        free(new_text);
    }
}

static void console_back_event_cb(lv_event_t *event)
{
    Serial.println("[CONSOLE] Returning to main menu");
    last_interaction_time = lv_tick_get();
    current_state = STATE_MAIN_MENU;
    state_start_time = lv_tick_get();
    create_main_menu();
}

static void console_clear_event_cb(lv_event_t *event)
{
    Serial.println("[CONSOLE] Clearing logs");
    if (console_textarea)
    {
        lv_textarea_set_text(console_textarea, "");
        console_add_log("Console cleared");
    }
}

static void create_console_screen()
{
    Serial.println("[DEBUG] Creating console screen...");
    lv_obj_clean(lv_scr_act());

    // Create dark background
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    console_title = lv_label_create(lv_scr_act());
    lv_label_set_text(console_title, "UART CONSOLE");
    lv_obj_set_style_text_font(console_title, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(console_title, lv_color_hex(0x9932CC), 0);
    lv_obj_align(console_title, LV_ALIGN_TOP_MID, 0, 10);

    // Console textarea (main log area)
    console_textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(console_textarea, SCR_W - 40, SCR_H - 150);
    lv_obj_align(console_textarea, LV_ALIGN_CENTER, 0, -10);

    // Console styling
    lv_obj_set_style_bg_color(console_textarea, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(console_textarea, lv_color_hex(0x00FF00), 0); // Green terminal text
    lv_obj_set_style_border_color(console_textarea, lv_color_hex(0x9932CC), 0);
    lv_obj_set_style_border_width(console_textarea, 2, 0);
    lv_obj_set_style_radius(console_textarea, 8, 0);

    // Set readonly
    lv_textarea_set_text(console_textarea, "");
    lv_obj_add_state(console_textarea, LV_STATE_DISABLED);

    // Button container
    lv_obj_t *btn_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(btn_container, SCR_W - 40, 60);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(btn_container, 20, 0);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Clear button
    console_clear_btn = lv_btn_create(btn_container);
    lv_obj_set_size(console_clear_btn, 140, 50);
    lv_obj_set_style_bg_color(console_clear_btn, lv_color_hex(0xFF6B35), 0);
    lv_obj_set_style_bg_color(console_clear_btn, lv_color_hex(0xFF8C69), LV_STATE_PRESSED);

    lv_obj_t *clear_label = lv_label_create(console_clear_btn);
    lv_label_set_text(clear_label, "CLEAR");
    lv_obj_set_style_text_font(clear_label, &minecraft_ten_48, 0);
    lv_obj_center(clear_label);

    // Back button
    console_back_btn = lv_btn_create(btn_container);
    lv_obj_set_size(console_back_btn, 140, 50);
    lv_obj_set_style_bg_color(console_back_btn, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(console_back_btn, lv_color_hex(0x666666), LV_STATE_PRESSED);

    lv_obj_t *back_label = lv_label_create(console_back_btn);
    lv_label_set_text(back_label, "BACK");
    lv_obj_set_style_text_font(back_label, &minecraft_ten_48, 0);
    lv_obj_center(back_label);

    // Add event handlers
    lv_obj_add_event_cb(console_clear_btn, console_clear_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(console_back_btn, console_back_event_cb, LV_EVENT_CLICKED, NULL);

    // Add initial welcome message
    console_add_log("UART Console started");
    console_add_log("Waiting for peripheral device data...");

    if (uart_protocol && uart_protocol->is_connected())
    {
        console_add_log("Connection to peripheral active");
    }
    else
    {
        console_add_log("Peripheral device not connected");
    }
}

// Simple text-based UART parser for slave device - logs everything received
void handle_slave_uart_data()
{
    if (peripheralUART.available())
    {
        String receivedData = peripheralUART.readStringUntil('\n');
        receivedData.trim();

        if (receivedData.length() > 0)
        {
            Serial.printf("📡 Raw UART Data: %s\n", receivedData.c_str());

            // Log ALL received data to console (this is what you want to see!)
            char log_buffer[256];
            snprintf(log_buffer, sizeof(log_buffer), "RX: %s", receivedData.c_str());
            console_add_log(log_buffer);

            // Additionally parse specific message formats if needed
            if (receivedData.startsWith("SENSOR_DATA"))
            {
                // Parse: SENSOR_DATA,HALL,EVENT,COUNT,TIMESTAMP,STATE
                int firstComma = receivedData.indexOf(',');
                int secondComma = receivedData.indexOf(',', firstComma + 1);
                int thirdComma = receivedData.indexOf(',', secondComma + 1);
                int fourthComma = receivedData.indexOf(',', thirdComma + 1);
                int fifthComma = receivedData.indexOf(',', fourthComma + 1);

                if (fifthComma > 0)
                {
                    String sensorType = receivedData.substring(firstComma + 1, secondComma);
                    String event = receivedData.substring(secondComma + 1, thirdComma);
                    String count = receivedData.substring(thirdComma + 1, fourthComma);
                    String timestamp = receivedData.substring(fourthComma + 1, fifthComma);
                    String state = receivedData.substring(fifthComma + 1);

                    if (event == "DETECTED")
                    {
                        snprintf(log_buffer, sizeof(log_buffer), ">>> MAGNET DETECTED! Count: %s", count.c_str());
                        console_add_log(log_buffer);
                    }
                    else if (event == "REMOVED")
                    {
                        snprintf(log_buffer, sizeof(log_buffer), ">>> Magnet removed. Count: %s", count.c_str());
                        console_add_log(log_buffer);
                    }
                    else
                    {
                        snprintf(log_buffer, sizeof(log_buffer), ">>> %s: %s (#%s)",
                                 sensorType.c_str(), event.c_str(), count.c_str());
                        console_add_log(log_buffer);
                    }
                }
            }
            else if (receivedData.startsWith("STATUS_UPDATE"))
            {
                console_add_log(">>> Status updated");

                int pos = receivedData.indexOf("DETECTIONS=");
                if (pos > 0)
                {
                    int end = receivedData.indexOf(',', pos);
                    String detections = receivedData.substring(pos + 11, end);
                    snprintf(log_buffer, sizeof(log_buffer), "    Total detections: %s", detections.c_str());
                    console_add_log(log_buffer);
                }

                pos = receivedData.indexOf("UPTIME=");
                if (pos > 0)
                {
                    int end = receivedData.indexOf(',', pos);
                    String uptime = receivedData.substring(pos + 7, end);
                    snprintf(log_buffer, sizeof(log_buffer), "    Uptime: %s sec", uptime.c_str());
                    console_add_log(log_buffer);
                }

                pos = receivedData.indexOf("HEAP=");
                if (pos > 0)
                {
                    String heap = receivedData.substring(pos + 5);
                    snprintf(log_buffer, sizeof(log_buffer), "    Free memory: %s bytes", heap.c_str());
                    console_add_log(log_buffer);
                }
            }
            else if (receivedData == "ESP32_SLAVE_READY")
            {
                console_add_log(">>> Peripheral device ready!");
            }
            else if (receivedData == "PONG")
            {
                console_add_log(">>> Peripheral replied to PING");
            }
            else if (receivedData.startsWith("ESP32_SLAVE_MSG"))
            {
                // Parse your specific message format: ESP32_SLAVE_MSG_123_TIME_456789
                console_add_log(">>> Slave message received");
            }
        }
    }
}

void loop()
{
    // Handle simple text-based UART from slave
    handle_slave_uart_data();

    // Update UART communication (binary protocol)
    if (uart_protocol)
    {
        uart_protocol->update();
    }

    // Debug: Show UART activity in serial monitor
    static uint32_t last_debug_time = 0;
    uint32_t now = millis();
    if (now - last_debug_time > 5000) // Every 5 seconds
    {
        last_debug_time = now;
        Serial.printf("[DEBUG] UART available bytes: %d\n", peripheralUART.available());
        if (current_state == STATE_CONSOLE)
        {
            Serial.println("[DEBUG] Console window is active");
        }
    }

    delay(5);
}
