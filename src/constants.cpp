#include "constants.h"

// === АПАРАТНІ НАЛАШТУВАННЯ ===
uint8_t LED_BRIGHTNESS = 150;

// === ШРИФТИ ===
const char* baseFontName = "NotoSansRegular30";
const char* welcomeFontName = "NotoSansRegular80";
const char* fontFileName = "/NotoSansRegular30.vlw";
const char* welcomeFontFileName = "/NotoSansRegular80.vlw";

// === ТАЙМІНГИ ===
// Загальні
const unsigned long debounceDelay = 50;
const unsigned long GET_READY_DURATION = 1000;
const unsigned long ROUND_COMPLETE_DURATION = 1000;
const unsigned long GAME_OVER_MESSAGE_DURATION = 2000;

// Режим реакції на час
const int TOTAL_TT_ROUNDS = 5;
const unsigned long RESULT_DISPLAY_DURATION = 1000;
const unsigned long NEXT_ROUND_DELAY_DURATION = 400;
const unsigned long TIMEOUT_REACTION = 1500;

// Тренажер пам'яті
const int MAX_SEQUENCE_LENGTH = 8;
// Memory trainer base timings (shortened for faster, більш динамічно)
const unsigned long LED_SHOW_DURATION = 260;   // was 400
const unsigned long LED_PAUSE_DURATION = 120;  // was 200
const unsigned long INPUT_TIMEOUT = 5000;

// Тренажер координації
const unsigned long COORDINATION_TIMEOUT = 2000;
const int TOTAL_COORDINATION_ROUNDS_SINGLE = 10;
const int MAX_COORDINATION_SEQUENCE_LENGTH = 8;
const unsigned long COORD_SEQ_LED_SHOW_DURATION = 300;
const unsigned long COORD_SEQ_LED_PAUSE_DURATION = 150;
const unsigned long COORD_SEQ_INPUT_TIMEOUT = 3000;
const int MAX_MULTIPLE_TARGETS = 3;
const unsigned long MULTI_TARGET_SHOW_DURATION = 500;
const unsigned long MULTI_INPUT_TIMEOUT = 2000;

// Тренажер влучності
const int ACCURACY_MAX_LEVEL_EASY_NORMAL = 15;
const int ACCURACY_INITIAL_CHASER_DELAY = 400;
const int ACCURACY_MIN_CHASER_DELAY = 60;
const int ACCURACY_CHASER_DELAY_DECREMENT = 25;
const unsigned long ACCURACY_ROUND_COMPLETE_MSG_DURATION = 1000;
const unsigned long ACCURACY_GAME_OVER_MSG_DURATION = 2000;
const int TOTAL_ACCURACY_ROUNDS = 15;
const int MAX_ACCURACY_MISSES = 3;
const unsigned long ACCURACY_TIMEOUT = 3000;
const unsigned long ACCURACY_CHASER_TIMEOUT = 10000;
const int ACCURACY_CHASER_SPEED_EASY = 200;
const int ACCURACY_CHASER_SPEED_MEDIUM = 400;
const int ACCURACY_CHASER_SPEED_HARD = 200;
const unsigned long ROUND_DURATION = 5000;  // Тривалість раунду в мс

// Координація
const int COORDINATION_EASY_START_LEDS = 1;     // Легко: початок з 1 діода
const int COORDINATION_EASY_MAX_LEDS = 10;      // Легко: максимум 10 діодів
const int COORDINATION_HARD_START_LEDS = 5;     // Важко: початок з 5 діодів
const int COORDINATION_HARD_MAX_LEDS = 15;      // Важко: максимум 15 діодів
const unsigned long COORDINATION_INITIAL_SHOW_TIME = 3000; // Початковий час показу (3 сек)
const unsigned long COORDINATION_MIN_SHOW_TIME = 800;      // Мінімальний час показу (0.8 сек)
const unsigned long COORDINATION_TIME_DECREASE_STEP = 200; // Зменшення на 200мс за рівень

// Режим реакції на виживання
const unsigned long SURVIVAL_PRE_ROUND_MIN_DELAY = 500;
const unsigned long SURVIVAL_PRE_ROUND_MAX_DELAY = 2000;
const unsigned long SURVIVAL_REACTION_TIMEOUT = 1000;
const unsigned long SURVIVAL_RESULTS_DISPLAY_DURATION = 4000;
const unsigned long ST_WRONG_PRESS_DURATION = 1000;
const unsigned long ST_GAME_OVER_MESSAGE_DURATION = 2000;

// Екран привітання
const int WELCOME_LED_DELAY = 50;

// === АНІМАЦІЯ ЕКРАНУ ПРИВІТАННЯ ===
const uint32_t TARGET_FPS = 60;
const uint32_t FRAME_MS = 1000 / TARGET_FPS;
const uint32_t HUE_CYCLE_MS = 1000;
const float HUE_SCALE = 0.1f;
