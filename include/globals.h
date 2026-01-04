#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"
#include "hardware_abstraction.h"

// === КОЛЬОРИ ===
extern RgbColor red;
extern RgbColor green;
extern RgbColor black;
extern RgbColor neonBlue;
extern RgbColor brightCyan;
extern RgbColor accentColor;
extern RgbColor memoryColor;
extern RgbColor coordinationColor;
extern RgbColor accuracyColor;

// === СТАН МЕНЮ ===
extern MenuState currentMenuState;
extern bool menuNeedsRedraw;

// === ЗМІННІ ЕКРАНУ ПРИВІТАННЯ ===
extern int welcomeLedIndex;
extern unsigned long welcomeLedTimer;
extern int welcomeLoopsDone;

// === АНІМАЦІЯ ЕКРАНУ ПРИВІТАННЯ ===
extern bool welcomeAnimationActive;
extern uint32_t lastWelcomeFrame;

// === СТАН КНОПОК ===
extern unsigned long lastButtonPressTime[];
extern uint16_t lastButtonStatePCF;

// === КНОПКИ МЕНЮ ===
extern Button_t reactionTrainerButton, memoryTrainerButton, coordinationTrainerButton, accuracyTrainerButton;
extern Button_t backButton, survivalModeButton, timeTrialModeButton;
extern Button_t survivalTime1MinButton, survivalTime2MinButton, survivalTime3MinButton, survivalBackButton;
extern Button_t coordinationEasyButton, coordinationHardButton, coordinationBackButton;
extern Button_t memoryPlayAgainButton, memoryExitButton;
extern Button_t coordinationPlayAgainButton, coordinationExitButton;
extern Button_t accuracyPlayAgainButton, accuracyExitButton;
extern Button_t timeTrialPlayAgainButton, timeTrialExitButton;
extern Button_t survivalTimePlayAgainButton, survivalTimeExitButton;
extern Button_t accuracyEasyButton, accuracyNormalButton, accuracyHardButton, accuracyBackFromDiffButton;
extern Button_t inGameBackButton;

// === ЗМІННІ ТРЕНАЖЕРІВ ===
extern bool roundResult;
extern bool feedbackSuccess;
extern int accuracyHits;
extern int accuracyMissed;

// === ЗМІННІ РЕКОРДІВ ===
extern int survivalRecord2Min;
extern int survivalRecord3Min;
extern int survivalRecord4Min;
extern int currentSurvivalDurationMinutes;

// === ЗМІННІ ЗАПОБІГАННЯ ПОВТОРЕНЬ ===
extern int lastSurvivalTargetButton;

// === СИСТЕМА ПАЦІЄНТІВ ===
extern int currentPatientIndex;              // Поточний вибраний пацієнт (0 = гість)
extern PatientStats patientStats[PATIENT_COUNT];  // Статистика всіх пацієнтів

// Функції роботи з пацієнтами
void initPatientSystem();
void savePatientStats(int patientIndex);
void loadPatientStats(int patientIndex);
void clearPatientStats(int patientIndex);

// Функції запису історії сесій
void addAccuracySession(int patientIndex, uint16_t score, uint16_t accuracy_pct);
void addReactionSession(int patientIndex, uint16_t time_ms, uint16_t attempts);
void addMemorySession(int patientIndex, uint16_t level, uint16_t correct);
void addCoordinationSession(int patientIndex, uint16_t score, uint16_t hits);

#endif // GLOBALS_H
