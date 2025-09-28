#ifndef MEMORY_TRAINER_H
#define MEMORY_TRAINER_H

#include "lvgl.h"
#include "types.h"
// #include "hardware_abstraction.h"

// === LVGL UI FUNCTIONS ===
void create_memory_trainer_screen();

// === GAME LOGIC FUNCTIONS ===
void set_memory_trainer_state(MemoryTrainerState newState);
void run_memory_trainer();

// === UI ELEMENTS (extern so they can be accessed from app_screens) ===
extern lv_obj_t *memory_trainer_screen;

#endif // MEMORY_TRAINER_H
