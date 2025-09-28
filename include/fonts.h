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

#endif // FONTS_H