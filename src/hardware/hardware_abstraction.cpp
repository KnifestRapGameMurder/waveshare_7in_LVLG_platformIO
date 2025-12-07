#include "hardware_abstraction.h"
#include "uart_protocol.h"
#include <Arduino.h>

extern UARTProtocol uart_protocol;

// Cache for button states (16 buttons)
// Initialize with 0xFFFF = all buttons released (1 = not pressed)
uint16_t button_state_cache = 0xFFFF;

// Last button state for edge detection (shared across trainers)
uint16_t last_button_state = 0xFFFF;

// --- Button Abstraction ---
uint16_t expanderRead()
{
    return button_state_cache;
}

void update_button_state(uint16_t newState)
{
    button_state_cache = newState;
}

int get_pressed_button()
{
    uint16_t current_button_state = expanderRead();
    
    for (int i = 0; i < NUM_LEDS; i++)
    {
        bool was_pressed = !(last_button_state & (1 << i));
        bool is_pressed = !(current_button_state & (1 << i));
        
        if (!was_pressed && is_pressed) // Button just pressed (rising edge)
        {
            last_button_state = current_button_state;
            return i;
        }
    }
    
    last_button_state = current_button_state;
    return -1; // No new button press
}

void reset_button_state()
{
    last_button_state = expanderRead();
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