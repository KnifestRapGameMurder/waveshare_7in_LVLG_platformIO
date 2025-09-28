#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "hardware_abstraction.h"
#include <Arduino.h>

// === ФУНКЦІЇ КЕРУВАННЯ LED ===
void lightUpLed(int ledIndex, RgbColor color, uint8_t brightness);
void lightUpMultipleLeds(int *ledIndices, int count, RgbColor color);
RgbColor Wheel(byte WheelPos);
void playHitAnimation(int centerLed);
void playMissAnimation(int centerLed);
void initLeds();

#endif // LED_CONTROL_H
