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

#endif // GLOBALS_H
