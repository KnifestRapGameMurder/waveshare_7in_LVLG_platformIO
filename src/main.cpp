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

// ---------- Налаштування ----------
static const uint32_t TARGET_FPS = 60;
static const uint32_t FRAME_MS = 1000 / TARGET_FPS;

// UI об'єкти
static lv_obj_t *canvas;
static lv_color_t *canvas_buf;
static lv_obj_t *main_label;
static lv_obj_t *sub_label_1;
static lv_obj_t *sub_label_2;
static lv_obj_t *desc_label_1;
static lv_obj_t *desc_label_2;

// Параметри екрану та анімації
static int32_t SCR_W = 800, SCR_H = 480; // Розмір екрану Waveshare 7"
static lv_timer_t *animation_timer;

// Параметри орбіт для трьох кольорових точок
struct ColorDot
{
    float x, y;
    lv_color_t color;
};

static float orbit_cx = 0.0f, orbit_cy = 0.0f;
static float orbit_angle[3] = {0.0f, 0.0f, 0.0f};
static float orbit_speed[3] = {0.9f, -1.2f, 1.6f}; // rad/s
static float orbit_radius[3] = {0.0f, 0.0f, 0.0f};
static ColorDot dots[3];

// Функція для інтерполяції кольору за допомогою зворотної відстані
static lv_color_t interpolate_color_idw(int px, int py)
{
    float x = (float)px, y = (float)py;

    // Обчислення відстаней до кожної точки
    float dx0 = x - dots[0].x, dy0 = y - dots[0].y;
    float dx1 = x - dots[1].x, dy1 = y - dots[1].y;
    float dx2 = x - dots[2].x, dy2 = y - dots[2].y;

    float d0 = sqrtf(dx0 * dx0 + dy0 * dy0);
    float d1 = sqrtf(dx1 * dx1 + dy1 * dy1);
    float d2 = sqrtf(dx2 * dx2 + dy2 * dy2);

    // Перевірка на точне співпадіння з точкою
    const float EPSILON_F = 1e-4f;
    if (d0 < EPSILON_F)
        return dots[0].color;
    if (d1 < EPSILON_F)
        return dots[1].color;
    if (d2 < EPSILON_F)
        return dots[2].color;

    // Обчислення вагових коефіцієнтів (зворотна квадратична відстань)
    const float P = 2.0f;
    float w0 = 1.0f / (d0 * d0);
    float w1 = 1.0f / (d1 * d1);
    float w2 = 1.0f / (d2 * d2);

    float wsum = w0 + w1 + w2;
    w0 /= wsum;
    w1 /= wsum;
    w2 /= wsum;

    // Декодування кольорів RGB565
    uint8_t r0 = LV_COLOR_GET_R(dots[0].color) << 3;
    uint8_t g0 = LV_COLOR_GET_G(dots[0].color) << 2;
    uint8_t b0 = LV_COLOR_GET_B(dots[0].color) << 3;

    uint8_t r1 = LV_COLOR_GET_R(dots[1].color) << 3;
    uint8_t g1 = LV_COLOR_GET_G(dots[1].color) << 2;
    uint8_t b1 = LV_COLOR_GET_B(dots[1].color) << 3;

    uint8_t r2 = LV_COLOR_GET_R(dots[2].color) << 3;
    uint8_t g2 = LV_COLOR_GET_G(dots[2].color) << 2;
    uint8_t b2 = LV_COLOR_GET_B(dots[2].color) << 3;

    // Інтерполяція
    uint8_t r = (uint8_t)(w0 * r0 + w1 * r1 + w2 * r2);
    uint8_t g = (uint8_t)(w0 * g0 + w1 * g1 + w2 * g2);
    uint8_t b = (uint8_t)(w0 * b0 + w1 * b1 + w2 * b2);

    return lv_color_make(r, g, b);
}

// Функція оновлення анімації
static void animation_timer_cb(lv_timer_t *timer)
{
    static uint32_t last_time = 0;
    uint32_t now = lv_tick_get();
    float dt = (now - last_time) / 1000.0f;
    if (dt < 0.001f)
        dt = 0.016f; // Мінімальний час кроку
    last_time = now;

    // Оновлення кутів орбіт
    for (int i = 0; i < 3; ++i)
    {
        orbit_angle[i] += orbit_speed[i] * dt;
        // Нормалізація кутів
        if (orbit_angle[i] > 6.2831853f)
            orbit_angle[i] -= 6.2831853f;
        if (orbit_angle[i] < -6.2831853f)
            orbit_angle[i] += 6.2831853f;
    }

    // Оновлення позицій точок
    dots[0].x = orbit_cx + cosf(orbit_angle[0]) * orbit_radius[0];
    dots[0].y = orbit_cy + sinf(orbit_angle[0]) * orbit_radius[0];

    dots[1].x = orbit_cx + cosf(orbit_angle[1]) * orbit_radius[1];
    dots[1].y = orbit_cy + sinf(orbit_angle[1]) * orbit_radius[1];

    dots[2].x = orbit_cx + cosf(orbit_angle[2]) * orbit_radius[2];
    dots[2].y = orbit_cy + sinf(orbit_angle[2]) * orbit_radius[2];

    // Малювання фону з градієнтом - оптимізована версія
    // Малюємо тільки кожен 4-й піксель для продуктивності
    const int step = 3;
    for (int y = 0; y < SCR_H; y += step)
    {
        for (int x = 0; x < SCR_W; x += step)
        {
            lv_color_t color = interpolate_color_idw(x, y);

            // Заповнюємо область step x step
            for (int dy = 0; dy < step && (y + dy) < SCR_H; dy++)
            {
                for (int dx = 0; dx < step && (x + dx) < SCR_W; dx++)
                {
                    lv_canvas_set_px(canvas, x + dx, y + dy, color);
                }
            }
        }
    }

    // Оновлюємо canvas
    lv_obj_invalidate(canvas);
}

void setup()
{
    Serial.begin(115200);

    Serial.println("Initializing board");
    Board *board = new Board();
    board->init();
#if LVGL_PORT_AVOID_TEARING_MODEt
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
    {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    assert(board->begin());

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);

    // Отримуємо реальний розмір екрану
    SCR_W = lv_disp_get_hor_res(NULL);
    SCR_H = lv_disp_get_ver_res(NULL);

    // Ініціалізація параметрів орбіт
    orbit_cx = SCR_W * 0.5f;
    orbit_cy = SCR_H * 0.5f;
    float baseR = (SCR_W < SCR_H ? SCR_W : SCR_H) * 0.28f;
    orbit_radius[0] = baseR * 1.0f;
    orbit_radius[1] = baseR * 0.7f;
    orbit_radius[2] = baseR * 0.45f;

    // Початкові кути
    orbit_angle[0] = 0.0f;
    orbit_angle[1] = 2.0f;
    orbit_angle[2] = 4.0f;

    // Кольори точок
    dots[0].color = lv_color_make(255, 0, 0); // Червоний
    dots[1].color = lv_color_make(0, 255, 0); // Зелений
    dots[2].color = lv_color_make(0, 0, 255); // Синій

    // Створення canvas для фону
    canvas = lv_canvas_create(lv_scr_act());
    canvas_buf = (lv_color_t *)malloc(SCR_W * SCR_H * sizeof(lv_color_t));
    if (canvas_buf)
    {
        lv_canvas_set_buffer(canvas, canvas_buf, SCR_W, SCR_H, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_size(canvas, SCR_W, SCR_H);
        lv_obj_set_pos(canvas, 0, 0);
        // Заповнюємо початковим кольором
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    }

    // Створення основного заголовку "НЕЙРО"
    main_label = lv_label_create(lv_scr_act());
    lv_label_set_text(main_label, "НЕЙРО");
    lv_obj_set_style_text_font(main_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(main_label, lv_color_white(), 0);
    lv_obj_align(main_label, LV_ALIGN_CENTER, 0, -60);

    // Створення підзаголовку "БЛОК"
    sub_label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(sub_label_1, "БЛОК");
    lv_obj_set_style_text_font(sub_label_1, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(sub_label_1, lv_color_white(), 0);
    lv_obj_align_to(sub_label_1, main_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // Інформація про систему
    sub_label_2 = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(sub_label_2, "ESP32_Display_Panel(%d.%d.%d)",
                          ESP_PANEL_VERSION_MAJOR, ESP_PANEL_VERSION_MINOR, ESP_PANEL_VERSION_PATCH);
    lv_obj_set_style_text_font(sub_label_2, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sub_label_2, lv_color_white(), 0);
    lv_obj_align_to(sub_label_2, sub_label_1, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);

    // Інформація про LVGL
    desc_label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(desc_label_1, "LVGL(%d.%d.%d)",
                          LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    lv_obj_set_style_text_font(desc_label_1, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(desc_label_1, lv_color_white(), 0);
    lv_obj_align_to(desc_label_1, sub_label_2, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // Додаткові описи
    desc_label_2 = lv_label_create(lv_scr_act());
    lv_label_set_text(desc_label_2, "Тренажер когнітивних навичок");
    lv_obj_set_style_text_font(desc_label_2, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(desc_label_2, lv_color_white(), 0);
    lv_obj_align(desc_label_2, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Запуск таймера анімації
    animation_timer = lv_timer_create(animation_timer_cb, FRAME_MS, NULL);

    /* Release the mutex */
    lvgl_port_unlock();

    Serial.println("UI Created successfully");
}

void loop()
{
    // LVGL обробляє все через свій таймер, тому тут просто невелика затримка
    delay(5);
}
