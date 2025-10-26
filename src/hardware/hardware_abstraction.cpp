#include "hardware_abstraction.h"
#include "uart_protocol.h"
#include <Arduino.h>

extern UARTProtocol uart_protocol;

// Cache for button states (16 buttons)
uint16_t button_state_cache = 0;

// --- Button Abstraction ---
uint16_t expanderRead()
{
    return button_state_cache;
}

void update_button_state(uint16_t newState)
{
    button_state_cache = newState;
}

void rgbColorToHex6(RgbColor color, char out[7]) // out: "RRGGBB"
{
    snprintf(out, 7, "%02X%02X%02X", color.r, color.g, color.b);
}

// --- LED Strip Abstraction ---
void strip_SetPixelColor(uint16_t n, RgbColor color)
{
    if (n < NUM_LEDS)
    {
        char hexColor[7];
        rgbColorToHex6(color, hexColor);
        uart_protocol.sendMessage(uart_protocol.createLEDSetPixelMessage(n, String(hexColor)));
    }
}

void strip_Clear()
{
    uart_protocol.sendMessage(uart_protocol.createLEDClearMessage("000000"));
}

void strip_Show()
{
    // In our UART protocol, changes are applied immediately
    // This function exists for compatibility but doesn't need to do anything
}

void strip_ClearTo(RgbColor color)
{
    char hexColor[7];
    rgbColorToHex6(color, hexColor);
    uart_protocol.sendMessage(uart_protocol.createLEDClearMessage(String(hexColor)));
}

// RGB utility functions
RgbColor RgbColor_Dim(RgbColor color, uint8_t brightness)
{
    return RgbColor((color.r * brightness) / 255,
                    (color.g * brightness) / 255,
                    (color.b * brightness) / 255);
}