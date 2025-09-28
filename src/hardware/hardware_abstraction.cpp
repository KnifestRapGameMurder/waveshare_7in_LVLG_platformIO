#include "hardware_abstraction.h"
#include "uart_protocol.h"
#include <Arduino.h>

extern UARTProtocol uart_protocol;

// Cache for button states (16 buttons)
static uint16_t button_state_cache = 0;

// --- Button Abstraction ---
uint16_t expanderRead()
{
    return button_state_cache;
}

void update_button_state(uint16_t newState)
{
    button_state_cache = newState;
}

// --- LED Strip Abstraction ---
void strip_SetPixelColor(uint16_t n, RgbColor color)
{
    if (n < NUM_LEDS)
    {
        char hexColor[8];
        snprintf(hexColor, sizeof(hexColor), "%02X%02X%02X", color.r, color.g, color.b);
        uart_protocol.sendMessage(uart_protocol.createLEDSetPixelMessage(n, String(hexColor)));
    }
}

void strip_Clear()
{
    uart_protocol.sendMessage(uart_protocol.createLEDClearMessage());
}

void strip_Show()
{
    // In our UART protocol, changes are applied immediately
    // This function exists for compatibility but doesn't need to do anything
}

void strip_ClearTo(RgbColor color)
{
    // If the color is black, just clear the strip.
    if (color.r == 0 && color.g == 0 && color.b == 0)
    {
        uart_protocol.sendMessage(uart_protocol.createLEDClearMessage());
    }
    else
    {
        // Your protocol does not have a "set all to hex color" command.
        // For now, this function will do nothing for non-black colors.
        // The trainer logic will need to set each pixel individually.
    }
}

RgbColor strip_GetPixelColor(int pixel)
{
    // For now, return black as we don't typically need to read LED colors
    return RgbColor(0, 0, 0);
}

// RGB utility functions
RgbColor RgbColor_Dim(RgbColor color, uint8_t brightness)
{
    return RgbColor((color.r * brightness) / 255,
                    (color.g * brightness) / 255,
                    (color.b * brightness) / 255);
}