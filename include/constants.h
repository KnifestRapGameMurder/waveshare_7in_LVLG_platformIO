#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

// === ПІНИ ТА АПАРАТНІ НАЛАШТУВАННЯ ===
#define LED_PIN 25
#define NUM_LEDS 16
#define I2C_SDA_PIN 13
#define I2C_SCL_PIN 14

// === UI КОЛЬОРИ (LVGL hex format) ===
#define COLOR_BG_DARK           0x1a1a1a
#define COLOR_BTN_BACK          0x444444
#define COLOR_BTN_BACK_PRESSED  0x666666
#define COLOR_BTN_GREEN         0x00FF00
#define COLOR_BTN_GREEN_PRESSED 0x00AA00
#define COLOR_BTN_RED           0xFF0000
#define COLOR_BTN_RED_PRESSED   0xAA0000
#define COLOR_BTN_YELLOW        0xFFFF00
#define COLOR_BTN_YELLOW_PRESSED 0xAAAA00
#define COLOR_BTN_CYAN          0x00CED1
#define COLOR_BTN_CYAN_PRESSED  0x008B8B
#define COLOR_BTN_ORANGE        0xFF6347
#define COLOR_BTN_ORANGE_PRESSED 0xCD5C5C
// Main menu trainer button colors
#define COLOR_MENU_ACCURACY     0xFFD700  // Gold
#define COLOR_MENU_REACTION     0x00CED1  // Dark Turquoise
#define COLOR_MENU_MEMORY       0x9370DB  // Medium Purple
#define COLOR_MENU_COORDINATION 0x32CD32  // Lime Green
#define COLOR_MENU_PRESSED_OFFSET 0x333333
#define COLOR_TEXT_SECONDARY    0xCCCCCC
#define COLOR_TEXT_WHITE        0xFFFFFF
#define COLOR_TEXT_ERROR        0xFF0000
#define COLOR_TEXT_SUCCESS      0x00FF00

// === ЗАГАЛЬНІ НАЛАШТУВАННЯ ===
extern uint8_t LED_BRIGHTNESS;

// === ШРИФТИ ===
extern const char *baseFontName;
extern const char *welcomeFontName;
extern const char *fontFileName;
extern const char *welcomeFontFileName;

// === ТАЙМІНГИ ===
// Загальні
extern const unsigned long debounceDelay;
extern const unsigned long GET_READY_DURATION;
extern const unsigned long ROUND_COMPLETE_DURATION;
extern const unsigned long GAME_OVER_MESSAGE_DURATION;

// Режим реакції на час
extern const int TOTAL_TT_ROUNDS;
extern const unsigned long RESULT_DISPLAY_DURATION;
extern const unsigned long NEXT_ROUND_DELAY_DURATION;
extern const unsigned long TIMEOUT_REACTION;

// Тренажер пам'яті
extern const int MAX_SEQUENCE_LENGTH;
extern const unsigned long LED_SHOW_DURATION;
extern const unsigned long LED_PAUSE_DURATION;
extern const unsigned long INPUT_TIMEOUT;

// Тренажер координації
extern const unsigned long COORDINATION_TIMEOUT;
extern const int TOTAL_COORDINATION_ROUNDS_SINGLE;
extern const int MAX_COORDINATION_SEQUENCE_LENGTH;
extern const unsigned long COORD_SEQ_LED_SHOW_DURATION;
extern const unsigned long COORD_SEQ_LED_PAUSE_DURATION;
extern const unsigned long COORD_SEQ_INPUT_TIMEOUT;
extern const int MAX_MULTIPLE_TARGETS;
extern const unsigned long MULTI_TARGET_SHOW_DURATION;
extern const unsigned long MULTI_INPUT_TIMEOUT;

// Тренажер влучності
extern const int ACCURACY_MAX_LEVEL_EASY_NORMAL;
extern const int ACCURACY_INITIAL_CHASER_DELAY;
extern const int ACCURACY_MIN_CHASER_DELAY;
extern const int ACCURACY_CHASER_DELAY_DECREMENT;
extern const unsigned long ACCURACY_ROUND_COMPLETE_MSG_DURATION;
extern const unsigned long ACCURACY_GAME_OVER_MSG_DURATION;
extern const int TOTAL_ACCURACY_ROUNDS;
extern const int MAX_ACCURACY_MISSES;
extern const unsigned long ACCURACY_TIMEOUT;
extern const unsigned long ACCURACY_CHASER_TIMEOUT;
extern const int ACCURACY_CHASER_SPEED_EASY;
extern const int ACCURACY_CHASER_SPEED_MEDIUM;
extern const int ACCURACY_CHASER_SPEED_HARD;
extern const unsigned long ROUND_DURATION;

// Координація
extern const int COORDINATION_EASY_START_LEDS;
extern const int COORDINATION_EASY_MAX_LEDS;
extern const int COORDINATION_HARD_START_LEDS;
extern const int COORDINATION_HARD_MAX_LEDS;
extern const unsigned long COORDINATION_INITIAL_SHOW_TIME;
extern const unsigned long COORDINATION_MIN_SHOW_TIME;
extern const unsigned long COORDINATION_TIME_DECREASE_STEP;

// Режим реакції на виживання
extern const unsigned long SURVIVAL_PRE_ROUND_MIN_DELAY;
extern const unsigned long SURVIVAL_PRE_ROUND_MAX_DELAY;
extern const unsigned long SURVIVAL_REACTION_TIMEOUT;
extern const unsigned long SURVIVAL_RESULTS_DISPLAY_DURATION;
extern const unsigned long ST_WRONG_PRESS_DURATION;
extern const unsigned long ST_GAME_OVER_MESSAGE_DURATION;

// Екран привітання
extern const int WELCOME_LED_DELAY;

// === АНІМАЦІЯ ЕКРАНУ ПРИВІТАННЯ ===
extern const uint32_t TARGET_FPS;
extern const uint32_t FRAME_MS;
extern const uint32_t HUE_CYCLE_MS;
extern const float HUE_SCALE;

#endif // CONSTANTS_H
