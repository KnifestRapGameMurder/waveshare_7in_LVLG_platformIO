#ifndef FONTS_H
#define FONTS_H

#include <lvgl.h>

// Font abstractions for easy swapping
// Font1: Largest (e.g., 48px)
// Font2: Medium (e.g., 36px)
// Font3: Smallest (e.g., 24px)
extern const lv_font_t *Font1;
extern const lv_font_t *Font2;
extern const lv_font_t *Font3;
extern const lv_font_t *Font96;

// Raw font declarations
extern const lv_font_t lv_lilita_one_regular_24;
extern const lv_font_t lv_lilita_one_regular_48;
extern const lv_font_t lv_lilita_one_regular_96;
extern const lv_font_t lv_lilita_one_regular_130;

#endif // FONTS_H