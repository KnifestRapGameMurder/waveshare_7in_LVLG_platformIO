#include "led_control.h"
#include "hardware_abstraction.h"
#include <Arduino.h>

// LED brightness constant - since we can't access constants.h, define it here
#ifndef LED_BRIGHTNESS
#define LED_BRIGHTNESS 50
#endif

// Color constants
static RgbColor black(0, 0, 0);
static RgbColor red(255, 0, 0);

void initLeds()
{
  strip_Clear();
  strip_Show();
}

void lightUpLed(int ledIndex, RgbColor color, uint8_t brightness)
{
  strip_Clear();
  if (ledIndex >= 0 && ledIndex < NUM_LEDS)
  {
    RgbColor scaledColor(
        (uint8_t)(color.r * brightness / 255),
        (uint8_t)(color.g * brightness / 255),
        (uint8_t)(color.b * brightness / 255));
    strip_SetPixelColor(ledIndex, scaledColor);
  }
  strip_Show();
}

void lightUpMultipleLeds(int *ledIndices, int count, RgbColor color)
{
  strip_Clear();
  RgbColor scaledColor(
      (uint8_t)(color.r * LED_BRIGHTNESS / 255),
      (uint8_t)(color.g * LED_BRIGHTNESS / 255),
      (uint8_t)(color.b * LED_BRIGHTNESS / 255));
  for (int i = 0; i < count; i++)
  {
    if (ledIndices[i] >= 0 && ledIndices[i] < NUM_LEDS)
    {
      strip_SetPixelColor(ledIndices[i], scaledColor);
    }
  }
  strip_Show();
}

RgbColor Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void playHitAnimation(int centerLed)
{
  // Note: In the real system, we can't use delay() for animations
  // This is a simplified version for compilation - animation logic should be timer-based
  for (int i = 0; i <= NUM_LEDS / 2; i++)
  {
    strip_Clear();
    RgbColor color = Wheel(i * (255 / (NUM_LEDS / 2)));
    strip_SetPixelColor((centerLed + i) % NUM_LEDS, color);
    strip_SetPixelColor((centerLed - i + NUM_LEDS) % NUM_LEDS, color);
    strip_Show();
    delay(30);
  }
  delay(200);
  strip_Clear();
  strip_Show();
}

void playMissAnimation(int centerLed)
{
  // Note: In the real system, we can't use delay() for animations
  // This is a simplified version for compilation - animation logic should be timer-based
  for (int i = 0; i <= NUM_LEDS / 2; i++)
  {
    strip_Clear();
    uint8_t brightness = LED_BRIGHTNESS - (i * 20);
    if (brightness > LED_BRIGHTNESS)
      brightness = 0; // Prevent underflow
    RgbColor dimRed(red.r * brightness / 255, red.g * brightness / 255, red.b * brightness / 255);
    strip_SetPixelColor((centerLed + i) % NUM_LEDS, dimRed);
    strip_SetPixelColor((centerLed - i + NUM_LEDS) % NUM_LEDS, dimRed);
    strip_Show();
    delay(30);
  }
  delay(200);
  strip_Clear();
  strip_Show();
}
