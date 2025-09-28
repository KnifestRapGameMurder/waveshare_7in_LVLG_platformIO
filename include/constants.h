#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

// === ПІНИ ТА АПАРАТНІ НАЛАШТУВАННЯ ===
#define LED_PIN     25
#define NUM_LEDS    16
#define I2C_SDA_PIN 13
#define I2C_SCL_PIN 14

// === ЗАГАЛЬНІ НАЛАШТУВАННЯ ===
extern uint8_t LED_BRIGHTNESS;

// === ШРИФТИ ===
extern const char* baseFontName;
extern const char* welcomeFontName;
extern const char* fontFileName;
extern const char* welcomeFontFileName;

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
