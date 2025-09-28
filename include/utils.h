#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

// Конвертує RGB float [0-1] у RGB565
inline uint16_t rgbTo565(float r, float g, float b) {
  int r5 = (int)(r * 31.0f + 0.5f);
  int g6 = (int)(g * 63.0f + 0.5f);
  int b5 = (int)(b * 31.0f + 0.5f);
  
  if (r5 < 0) r5 = 0; else if (r5 > 31) r5 = 31;
  if (g6 < 0) g6 = 0; else if (g6 > 63) g6 = 63;
  if (b5 < 0) b5 = 0; else if (b5 > 31) b5 = 31;
  
  return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

// Генерує колір wheel для анімації LED
inline RgbColor Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Читає стан кнопок з PCF8575 expander
uint16_t expanderRead();

#endif // UTILS_H
