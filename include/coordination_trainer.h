#ifndef COORDINATION_TRAINER_H
#define COORDINATION_TRAINER_H

#include "lvgl.h"
#include "types.h"
// #include "hardware_abstraction.h"

// === LVGL UI FUNCTIONS ===
void create_coordination_submenu();
void create_coordination_trainer_screen();

// === GAME LOGIC FUNCTIONS ===
void set_coordination_trainer_state(CoordinationTrainerState newState);
void run_coordination_trainer();
void set_coordination_mode(CoordinationSubmenuState mode);

// === UI ELEMENTS (extern so they can be accessed from app_screens) ===
extern lv_obj_t *coordination_submenu_screen;
extern lv_obj_t *coordination_trainer_screen;

#endif
