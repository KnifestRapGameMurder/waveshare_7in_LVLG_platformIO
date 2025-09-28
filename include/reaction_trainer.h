#ifndef REACTION_TRAINER_H
#define REACTION_TRAINER_H

#include "lvgl.h"
#include "types.h"
// #include "hardware_abstraction.h"

// === LVGL UI FUNCTIONS ===
void create_reaction_submenu();
void create_reaction_time_trial_screen();
void create_reaction_survival_submenu();
void create_reaction_survival_screen();
void create_reaction_trainer_screen();

// === GAME LOGIC FUNCTIONS ===
void set_time_trial_state(TimeTrialState newState);
void run_time_trial();
void set_survival_time_state(SurvivalTimeState newState);
void run_survival_time_trainer();

// Функції для встановлення тривалості гри
void set_survival_duration(int minutes);

// === ФУНКЦІЇ РОБОТИ З РЕКОРДАМИ ===
void load_survival_records();
void save_survival_record(int duration, int score);
int get_survival_record(int duration);
bool is_new_record(int score, int duration);

// === UI ELEMENTS (extern so they can be accessed from app_screens) ===
extern lv_obj_t *reaction_submenu_screen;
extern lv_obj_t *reaction_time_trial_screen;
extern lv_obj_t *reaction_survival_submenu_screen;
extern lv_obj_t *reaction_survival_screen;

#endif // REACTION_TRAINER_H
