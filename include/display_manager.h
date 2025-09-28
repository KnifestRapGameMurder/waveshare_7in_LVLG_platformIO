#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "types.h"

// === ФУНКЦІЇ ДИСПЛЕЮ ===
void setGameFont();
void drawButton(Button_t btn);
void displayWelcomeScreen();
void updateWelcomeAnimation();
void displayMainMenu();
void displayReactionSubmenu();
void displayReactionSurvivalSubmenu();
void displayCoordinationSubmenu();
void displayAccuracyDifficultySubmenu();

#endif // DISPLAY_MANAGER_H
