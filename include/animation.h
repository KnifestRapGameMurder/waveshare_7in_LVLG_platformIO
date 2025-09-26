#pragma once
#include <lvgl.h>

// Animation structures and functions
struct ColorDot
{
    float x, y;
    lv_color_t color;
};

// Animation variables
extern float orbit_cx, orbit_cy;
extern float orbit_angle[3];
extern float orbit_speed[3];
extern float orbit_radius[3];
extern float orbit_scale_x, orbit_scale_y;
extern ColorDot dots[3];
extern lv_timer_t *animation_timer;

// Animation functions
void animation_init(int32_t screen_w, int32_t screen_h);
void animation_update();
void animation_timer_cb(lv_timer_t *timer);
lv_color_t interpolate_color_idw_fast(int px, int py);
void gradient_draw_event_cb(lv_event_t *e);