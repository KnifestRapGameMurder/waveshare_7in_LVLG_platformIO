#ifndef HARDWARE_ABSTRACTION_H
#define HARDWARE_ABSTRACTION_H

#include <stdint.h>

// Define the number of LEDs
#define NUM_LEDS 16

// Define a simple RgbColor struct for convenience
struct RgbColor
{
    uint8_t r, g, b;
    RgbColor() : r(0), g(0), b(0) {}
    RgbColor(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

    // Dim method for brightness scaling
    RgbColor Dim(uint8_t brightness) const
    {
        return RgbColor((r * brightness) / 255, (g * brightness) / 255, (b * brightness) / 255);
    }
};

// --- Button Abstraction ---
// This function will read the cached state of the buttons.
// The state is updated by incoming UART messages.
uint16_t expanderRead();

// This function will be called from main.cpp when a button state message arrives.
void update_button_state(uint16_t newState);

// --- LED Strip Abstraction ---
// These functions will send commands over UART to the slave board.
void strip_SetPixelColor(uint16_t n, RgbColor color);
void strip_Clear();
void strip_Show();
void strip_ClearTo(RgbColor color);
RgbColor strip_GetPixelColor(int pixel);

// RGB utility functions
RgbColor RgbColor_Dim(RgbColor color, uint8_t brightness);

#endif // HARDWARE_ABSTRACTION_H
