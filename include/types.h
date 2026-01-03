#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// === СТАНИ ГОЛОВНОГО МЕНЮ ===
enum MenuState
{
  WELCOME_SCREEN,
  MAIN_MENU,
  REACTION_SUBMENU,
  MEMORY_TRAINER,
  REACTION_TIME_TRIAL,
  GAME_REACTION_SURVIVAL,
  COORDINATION_TRAINER,
  ACCURACY_TRAINER,
  REACTION_SURVIVAL_SUBMENU,
  COORDINATION_SUBMENU,
  ACCURACY_DIFFICULTY_SUBMENU
};

// === РІВНІ СКЛАДНОСТІ ===
enum DifficultyLevel
{
  EASY,
  NORMAL,
  HARD
};

// === СТАНИ ДЛЯ ІГРОВИХ РЕЖИМІВ ===
enum TimeTrialState
{
  TT_STATE_IDLE,
  TT_STATE_GET_READY,
  TT_STATE_PRE_ROUND_DELAY,
  TT_STATE_WAIT_FOR_PRESS,
  TT_STATE_SHOW_RESULT,
  TT_STATE_NEXT_ROUND_DELAY,
  TT_STATE_GAME_OVER,
  TT_STATE_SHOW_RESULTS,
  TT_STATE_GAME_OVER_MENU,
  TT_STATE_WAIT_FOR_EXIT
};

enum MemoryTrainerState
{
  MT_STATE_IDLE,
  MT_STATE_GET_READY,
  MT_STATE_SHOW_SEQUENCE,
  MT_STATE_WAIT_FOR_INPUT,
  MT_STATE_CHECK_INPUT,
  MT_STATE_ROUND_COMPLETE,
  MT_STATE_GAME_OVER,
  MT_STATE_GAME_OVER_MENU
};

enum CoordinationSubmenuState
{
  CS_SUBMENU_IDLE,
  CS_EASY_MODE,
  CS_HARD_MODE
};

enum CoordinationTrainerState
{
  CT_STATE_IDLE,
  CT_STATE_GET_READY,
  CT_STATE_SHOW_TARGET,
  CT_STATE_WAIT_FOR_PRESS,
  CT_STATE_CHECK_INPUT,
  CT_STATE_ROUND_COMPLETE,
  CT_STATE_GAME_OVER,
  CT_STATE_SHOW_RESULTS,
  CT_STATE_GAME_OVER_MENU
};

enum AccuracyTrainerState
{
  AT_STATE_IDLE,
  AT_STATE_GET_READY,
  AT_STATE_SHOW_TARGET,
  AT_STATE_WAIT_FOR_PRESS,
  AT_STATE_ROUND_COMPLETE,
  AT_STATE_GAME_OVER,
  AT_STATE_SHOW_RESULTS,
  AT_STATE_GAME_OVER_MENU,
  AT_STATE_FEEDBACK
};

enum SurvivalTimeState
{
  ST_STATE_IDLE,
  ST_STATE_GET_READY,
  ST_STATE_COUNTDOWN,
  ST_STATE_START_MESSAGE,
  ST_STATE_FAST_GAMEPLAY,
  ST_STATE_STOP_MESSAGE,
  ST_STATE_PRE_ROUND_DELAY,
  ST_STATE_WAIT_FOR_PRESS,
  ST_STATE_WRONG_PRESS,
  ST_STATE_GAME_OVER_TIME,
  ST_STATE_GAME_OVER_MISTAKE,
  ST_STATE_SHOW_RESULTS,
  ST_STATE_GAME_OVER_MENU
};

// === СТРУКТУРА КНОПКИ ===
struct Button_t
{
  int16_t x, y, w, h;
  String label;
  uint16_t outlineColor, fillColor, textColor;
  MenuState targetState;
  void (*action)();
};

// === КІЛЬКІСТЬ ПАЦІЄНТІВ ===
#define PATIENT_COUNT 16  // 15 пацієнтів + 1 гість (індекс 0)

// === СТАТИСТИКА ПАЦІЄНТА ===
struct PatientStats
{
  // Влучність (Accuracy)
  uint16_t accuracy_sessions;      // Кількість сесій
  uint16_t accuracy_total_hits;    // Загальна кількість влучань
  uint16_t accuracy_total_misses;  // Загальна кількість промахів
  uint16_t accuracy_best_score;    // Найкращий результат

  // Реакція (Reaction)
  uint16_t reaction_sessions;      // Кількість сесій
  uint16_t reaction_best_time_ms;  // Найкращий час реакції (мс)
  uint32_t reaction_avg_time_sum;  // Сума часів для середнього
  uint16_t reaction_avg_count;     // Кількість вимірювань

  // Пам'ять (Memory)
  uint16_t memory_sessions;        // Кількість сесій
  uint16_t memory_best_level;      // Найкращий рівень
  uint16_t memory_total_correct;   // Загальна кількість правильних

  // Координація (Coordination)
  uint16_t coordination_sessions;  // Кількість сесій
  uint16_t coordination_best_score;// Найкращий результат
  uint16_t coordination_total_hits;// Загальна кількість влучань
};

#endif // TYPES_H
