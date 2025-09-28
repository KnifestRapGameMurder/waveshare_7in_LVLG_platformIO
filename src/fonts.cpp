#include "fonts.h"

extern const lv_font_t lv_lilita_one_regular_24;
extern const lv_font_t lv_lilita_one_regular_48;
extern const lv_font_t lv_lilita_one_regular_96;

// Assign fonts to abstractions (swap files here to change fonts globally)
const lv_font_t *Font1 = &lv_lilita_one_regular_96; // Largest
const lv_font_t *Font2 = &lv_lilita_one_regular_48; // Medium
const lv_font_t *Font3 = &lv_lilita_one_regular_24; // Smallest