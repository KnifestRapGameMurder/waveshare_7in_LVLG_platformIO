#ifndef ACCURACY_TRAINER_H
#define ACCURACY_TRAINER_H

#include "lvgl.h"
#include "types.h"
// #include "hardware_abstraction.h"

// === ENUMS ===
enum AccuracyDifficulty
{
    ACCURACY_EASY = 1,
    ACCURACY_MEDIUM = 2,
    ACCURACY_HARD = 3
};

// === ЗОВНІШНІ ЗМІННІ ===
extern AccuracyTrainerState accuracyTrainerState;
extern AccuracyDifficulty accuracyDifficulty;
extern unsigned long accuracyTrainerTimer;
extern int accuracyTarget;
extern unsigned long accuracyRoundStartTime;
extern int correctAccuracyPresses;
extern int totalAccuracyRounds;
extern int accuracyMisses;

// Для чейзер режиму
extern int chaserPosition;
extern bool chaserDirection;
extern unsigned long lastChaserMove;
extern int accuracyChaserSpeed;

// Для нових режимів
extern int multiTargets[3];
extern int sequenceTargets[5];
extern unsigned long blinkTimer;
extern bool blinkState;
extern int correctTarget;
extern int sequenceLength;
extern int sequenceStep;
extern unsigned long sequenceTimer;

// Додаткові змінні
extern int prevAccuracyTarget;
extern unsigned long feedbackStartTime;
extern unsigned long lastChaserPatternChange;
extern int chaserSkip;

// === LVGL UI FUNCTIONS ===
void create_accuracy_trainer_screen();
void create_accuracy_difficulty_submenu();

// === GAME LOGIC FUNCTIONS ===
void set_accuracy_trainer_state(AccuracyTrainerState newState);
void run_accuracy_trainer();
void set_accuracy_difficulty(AccuracyDifficulty difficulty);

// === UI ELEMENTS (extern so they can be accessed from app_screens) ===
extern lv_obj_t *accuracy_trainer_screen;
extern lv_obj_t *accuracy_difficulty_screen;

#endif
