#ifndef TFT_COMPATIBILITY_H
#define TFT_COMPATIBILITY_H

#include <Arduino.h>

// TFT color constants for compatibility
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF
#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0
#define TFT_BLUE 0x001F
#define TFT_YELLOW 0xFFE0
#define TFT_GOLD 0xFEA0
#define TFT_DARKGREY 0x7BEF

// Text datum constants
#define MC_DATUM 4
#define TC_DATUM 1

// TFT compatibility class
class TFT_eSPI_Compat
{
public:
    void fillScreen(uint16_t color) { /* Stub - LVGL handles screen drawing */ }
    void setTextColor(uint16_t color) { /* Stub */ }
    void setTextDatum(uint8_t datum) { /* Stub */ }
    void drawString(const String &text, int x, int y) { /* Stub */ }
    void fillRect(int x, int y, int w, int h, uint16_t color) { /* Stub */ }
    int width() { return 800; }  // Waveshare 7" width
    int height() { return 480; } // Waveshare 7" height
};

// Global TFT object for compatibility
extern TFT_eSPI_Compat tft;

// PCF8575 compatibility class
class PCF8575_Compat
{
public:
    bool read(uint8_t pin) { return HIGH; } // Stub - returns no press
};

// Global expander object for compatibility
extern PCF8575_Compat pcf8575;

// LED strip compatibility class
class NeoPixelBus_Compat
{
public:
    void SetPixelColor(int pixel, uint32_t color) { /* Stub */ }
    void ClearTo(uint32_t color) { /* Stub */ }
    void Show() { /* Stub */ }
    uint32_t GetPixelColor(int pixel) { return 0; }
};

// Global LED strip object for compatibility
extern NeoPixelBus_Compat strip;

// Display function stubs
void setGameFont();
void drawAccuracyHUD();
void drawButton(const void *button);
void lightUpLed(int led, uint32_t color, uint8_t brightness);

// Trainer display function stubs
void displayAccuracyResults();
void displayAccuracyGameOverMenu();
void displayCoordinationResults();
void displayCoordinationGameOverMenu();
void displayMemoryGameOverMenu();
void displayTimeTrialGameOverMenu();
void displaySurvivalResults();
void displaySurvivalGameOverMenu();

// Record functions stubs
bool isNewRecord(int score, int duration);
int getSurvivalRecord(int duration);
void saveSurvivalRecord(int duration, int score);

#endif // TFT_COMPATIBILITY_H