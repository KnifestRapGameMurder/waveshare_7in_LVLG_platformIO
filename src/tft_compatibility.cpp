#include "tft_compatibility.h"

// Global compatibility objects
TFT_eSPI_Compat tft;
PCF8575_Compat pcf8575;
NeoPixelBus_Compat strip;

// Display function stubs
void setGameFont()
{
    // Stub - LVGL handles fonts
}

void drawAccuracyHUD()
{
    // Stub - LVGL handles UI elements
}

void drawButton(const void *button)
{
    // Stub - LVGL handles buttons
}

void lightUpLed(int led, uint32_t color, uint8_t brightness)
{
    // Stub - hardware abstraction layer handles LEDs
}

// Trainer display function stubs
void displayAccuracyResults()
{
    // Stub - LVGL handles result display
}

void displayAccuracyGameOverMenu()
{
    // Stub - LVGL handles menus
}

void displayCoordinationResults()
{
    // Stub - LVGL handles result display
}

void displayCoordinationGameOverMenu()
{
    // Stub - LVGL handles menus
}

void displayMemoryGameOverMenu()
{
    // Stub - LVGL handles menus
}

void displayTimeTrialGameOverMenu()
{
    // Stub - LVGL handles menus
}

void displaySurvivalResults()
{
    // Stub - LVGL handles result display
}

void displaySurvivalGameOverMenu()
{
    // Stub - LVGL handles menus
}

// Record functions stubs
bool isNewRecord(int score, int duration)
{
    // Stub - no record keeping for now
    return false;
}

int getSurvivalRecord(int duration)
{
    // Stub - no record keeping for now
    return 0;
}

void saveSurvivalRecord(int duration, int score)
{
    // Stub - no record keeping for now
}