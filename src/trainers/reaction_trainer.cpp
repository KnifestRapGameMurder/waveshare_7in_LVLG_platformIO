#include "reaction_trainer.h"
#include "hardware_abstraction.h"
#include "app_screens.h"
#include "globals.h"
#include "uart_protocol.h"
#include <Arduino.h>
#include <lvgl.h>
#include <SD_MMC.h>
#include "fonts.h"

// === Game Constants ===
const int GET_READY_DURATION = 3000;      // 3 seconds
const int RESULT_DISPLAY_DURATION = 2000; // 2 seconds
const int TIMEOUT_REACTION = 5000;        // 5 seconds timeout
const int TOTAL_TT_ROUNDS = 5;            // 5 rounds for time trial
const int SURVIVAL_PRE_ROUND_MIN_DELAY = 500;
const int SURVIVAL_PRE_ROUND_MAX_DELAY = 2000;
const int ST_WRONG_PRESS_DURATION = 1500;
const int ST_GAME_OVER_MESSAGE_DURATION = 2000;
const int SURVIVAL_RESULTS_DISPLAY_DURATION = 5000;

// === Time Trial Variables ===
static TimeTrialState timeTrialState = TT_STATE_IDLE;
static int currentTTRound = 0;
static unsigned long reactionTimes[TOTAL_TT_ROUNDS];
static unsigned long timeTrialTimer = 0;
static bool waitForReaction = false;
static unsigned long reactionStart = 0;
static int targetButton = 0;

// === Survival Variables ===
static SurvivalTimeState survivalTimeState = ST_STATE_IDLE;
static unsigned long survivalGameDuration = 0;
static unsigned long survivalGameStartTime = 0;
static int survivalCorrectPresses = 0;
static int survivalTotalPresses = 0;
static unsigned long survivalRoundTimer = 0;
// Note: lastSurvivalTargetButton, currentSurvivalDurationMinutes, 
// survivalRecord2Min, survivalRecord3Min, survivalRecord4Min
// are declared as extern in globals.h and defined in globals.cpp

// === UI Elements ===
static lv_obj_t *reaction_screen = NULL;
static lv_obj_t *round_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_obj_t *results_label = NULL;

// === Forward Declarations ===
static void update_round_display();
static void check_button_presses_time_trial();
static void check_button_presses_survival();
static void display_time_trial_results();
static void display_survival_results();
static void create_time_trial_game_over_menu();
static void back_to_menu_event_handler(lv_event_t *e);
static int get_random_button_avoiding_last(int lastButton);

// Async wrappers for transitions
static void async_open_reaction_submenu(void *user_data) { create_reaction_submenu(); }
static void async_restart_time_trial(void *user_data) { set_time_trial_state(TT_STATE_GET_READY); }
static void async_restart_survival(void *user_data) { set_survival_time_state(ST_STATE_GET_READY); }


// External font
// (Preferences removed — survival records stored on SD card)

void create_reaction_trainer_screen(AppState target_mode)
{
    // Use base function to create common elements
    TrainerScreenElements elements = create_trainer_screen_base(back_to_menu_event_handler);
    
    // Assign to local static variables
    reaction_screen = elements.screen;
    round_label = elements.top_label;
    info_label = elements.info_label;
    results_label = elements.results_label;
    // back_btn is local in base function, no need to store

    // Update global app state ONLY after UI is ready
    current_state = target_mode;

    // Initialize game state based on targeted mode
    if (current_state == STATE_REACTION_SURVIVAL) {
        set_survival_time_state(ST_STATE_GET_READY);
    } else {
        set_time_trial_state(TT_STATE_GET_READY);
    }
}

void set_time_trial_state(TimeTrialState newState)
{
    Serial.printf("set_time_trial_state(%d)\n", newState);

    timeTrialState = newState;
    timeTrialTimer = lv_tick_get();

    switch (timeTrialState)
    {
    case TT_STATE_IDLE:
        strip_Clear();
        break;

    case TT_STATE_GET_READY:
        lv_label_set_text(info_label, "Приготуйся!");
        playAudioPrompt(AUDIO_GET_READY);  // Голосова підказка
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);
        currentTTRound = 0;
        waitForReaction = false;
        for (int i = 0; i < TOTAL_TT_ROUNDS; i++)
            reactionTimes[i] = 0;
        strip_Clear();
        update_round_display();
        break;

    case TT_STATE_PRE_ROUND_DELAY:
        targetButton = random(NUM_LEDS);
        timeTrialTimer = lv_tick_get() + random(500, 2000);
        lv_label_set_text(info_label, "Чекай світла...");
        playAudioPrompt(AUDIO_WAIT_LIGHT);  // Голосова підказка
        update_round_display();
        break;

    case TT_STATE_WAIT_FOR_PRESS:
        strip_SetPixelColor(targetButton, RgbColor(0, 255, 0)); // Green
        strip_Show();
        waitForReaction = true;
        reactionStart = lv_tick_get();
        lv_label_set_text(info_label, "Натискай!");
        playAudioPrompt(AUDIO_PRESS);  // Голосова підказка
        break;

    case TT_STATE_SHOW_RESULT:
        strip_Clear();
        strip_Show();
        waitForReaction = false;
        if (reactionTimes[currentTTRound] > 0 && reactionTimes[currentTTRound] <= TIMEOUT_REACTION)
        {
            char result_text[32];
            snprintf(result_text, sizeof(result_text), "Час: %lu мс", reactionTimes[currentTTRound]);
            lv_label_set_text(info_label, result_text);
            // Оцінка результату
            if (reactionTimes[currentTTRound] < 300) {
                playAudioPrompt(AUDIO_EXCELLENT);  // Відмінно!
            } else if (reactionTimes[currentTTRound] < 500) {
                playAudioPrompt(AUDIO_GOOD);  // Добре!
            }
        }
        else
        {
            lv_label_set_text(info_label, "Таймаут!");
            playAudioPrompt(AUDIO_TIMEOUT);  // Час вийшов!
        }
        timeTrialTimer = lv_tick_get() + RESULT_DISPLAY_DURATION;
        break;

    case TT_STATE_NEXT_ROUND_DELAY:
        break;

    case TT_STATE_GAME_OVER:
        // This state is no longer used - results shown directly in SHOW_RESULT
        break;

    case TT_STATE_GAME_OVER_MENU:
        // This state is no longer used
        break;

    case TT_STATE_WAIT_FOR_EXIT:
        // Just wait for menu button presses
        break;

    default:
        break;
    }
}

static void update_round_display()
{
    char round_text[32];
    snprintf(round_text, sizeof(round_text), "Раунд %d/%d", currentTTRound + 1, TOTAL_TT_ROUNDS);
    lv_label_set_text(round_label, round_text);
}

static void check_button_presses_time_trial()
{
    if (!waitForReaction)
        return;

    uint16_t current_button_state = expanderRead();

    for (int i = 0; i < NUM_LEDS; i++)
    {
        bool was_pressed = !(last_button_state & (1 << i));
        bool is_pressed = !(current_button_state & (1 << i));

        if (!was_pressed && is_pressed) // Button just pressed
        {
            waitForReaction = false;
            last_interaction_time = lv_tick_get();

            if (i == targetButton)
            {
                unsigned long rT = lv_tick_get() - reactionStart;
                Serial.printf("RT Round %d: %lu ms\n", currentTTRound, rT);
                reactionTimes[currentTTRound] = rT;
                strip_SetPixelColor(i, RgbColor(0, 255, 0)); // Green feedback
            }
            else
            {
                Serial.printf("Round %d: Wrong button!\n", currentTTRound);
                strip_SetPixelColor(i, RgbColor(255, 0, 0)); // Red feedback
                reactionTimes[currentTTRound] = 0;
            }

            Serial.printf("Round %d complete, transitioning to SHOW_RESULT\n", currentTTRound);
            set_time_trial_state(TT_STATE_SHOW_RESULT);
            break;
        }
    }

    last_button_state = current_button_state;

    if (waitForReaction && (lv_tick_get() - reactionStart > TIMEOUT_REACTION))
    {
        Serial.println("Timeout");
        waitForReaction = false;
        reactionTimes[currentTTRound] = 0;
        set_time_trial_state(TT_STATE_SHOW_RESULT);
    }
}

void run_time_trial()
{
    switch (timeTrialState)
    {
    case TT_STATE_GET_READY:
        if (lv_tick_get() - timeTrialTimer > GET_READY_DURATION)
            set_time_trial_state(TT_STATE_PRE_ROUND_DELAY);
        break;

    case TT_STATE_PRE_ROUND_DELAY:
        if (lv_tick_get() > timeTrialTimer)
            set_time_trial_state(TT_STATE_WAIT_FOR_PRESS);
        break;

    case TT_STATE_WAIT_FOR_PRESS:
        check_button_presses_time_trial();
        break;

    case TT_STATE_SHOW_RESULT:
        if (lv_tick_get() > timeTrialTimer)
        {
            currentTTRound++;
            Serial.printf("TT_STATE_SHOW_RESULT: Round completed. currentTTRound now = %d, TOTAL = %d\n", 
                         currentTTRound, TOTAL_TT_ROUNDS);
            
            if (currentTTRound < TOTAL_TT_ROUNDS)
            {
                Serial.println("TT_STATE_SHOW_RESULT: More rounds to go, transitioning to NEXT_ROUND_DELAY");
                set_time_trial_state(TT_STATE_NEXT_ROUND_DELAY);
            }
            else
            {
                Serial.println("TT_STATE_SHOW_RESULT: All rounds done! Showing results");
                display_time_trial_results();
                set_time_trial_state(TT_STATE_SHOW_RESULTS);
            }
        }
        break;

    case TT_STATE_SHOW_RESULTS:
        // Results and buttons are shown immediately in display_time_trial_results()
        break;

    case TT_STATE_NEXT_ROUND_DELAY:
        if (lv_tick_get() > timeTrialTimer)
            set_time_trial_state(TT_STATE_PRE_ROUND_DELAY);
        break;

    case TT_STATE_GAME_OVER:
        // This state is no longer used - we show results directly in SHOW_RESULT
        break;

    case TT_STATE_WAIT_FOR_EXIT:
        // Just wait for user to press menu buttons
        break;

    case TT_STATE_GAME_OVER_MENU:
        break;

    default:
        break;
    }
}

static void display_time_trial_results()
{
    Serial.println("display_time_trial_results START");
    
    strip_Clear();
    strip_Show();
    
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(round_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    unsigned long totalReactionTime = 0;
    unsigned long bestTime = UINT32_MAX;
    int validRounds = 0;
    
    for (int i = 0; i < TOTAL_TT_ROUNDS; i++)
    {
        if (reactionTimes[i] > 0 && reactionTimes[i] <= TIMEOUT_REACTION)
        {
            totalReactionTime += reactionTimes[i];
            if (reactionTimes[i] < bestTime) bestTime = reactionTimes[i];
            validRounds++;
        }
    }

    // === Оновлюємо статистику пацієнта в RAM (збереження у Flash - через delayed callback) ===
    PatientStats *stats = &patientStats[currentPatientIndex];
    if (validRounds > 0)
    {
        unsigned long avgTime = totalReactionTime / validRounds;
        stats->reaction_sessions++;
        stats->reaction_avg_time_sum += avgTime;
        stats->reaction_avg_count++;
        if (bestTime < stats->reaction_best_time_ms || stats->reaction_best_time_ms == 0)
        {
            stats->reaction_best_time_ms = bestTime;
        }
        // Записуємо в історію сесій (час, кількість раундів)
        addReactionSession(currentPatientIndex, (uint16_t)avgTime, validRounds);
        savePatientStats(currentPatientIndex); // Зберігаємо до SD
    }
    // ==========================================================================================

    char results_text[64];
    if (validRounds > 0)
    {
        unsigned long avgTime = totalReactionTime / validRounds;
        snprintf(results_text, sizeof(results_text), 
                 "%lu мс",
                 avgTime);
    }
    else
    {
        snprintf(results_text, sizeof(results_text), "---");
    }
    
    lv_obj_set_style_text_font(results_label, &lv_lilita_one_regular_96, 0);
    lv_label_set_text(results_label, results_text);
    lv_obj_set_style_text_align(results_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(results_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_update_layout(results_label);

    Serial.println("display_time_trial_results END");
}

// === SURVIVAL MODE ===

void set_survival_time_state(SurvivalTimeState newState)
{
    survivalTimeState = newState;
    survivalRoundTimer = lv_tick_get();

    switch (survivalTimeState)
    {
    case ST_STATE_IDLE:
        break;

    case ST_STATE_GET_READY:
        lv_label_set_text(info_label, "Приготуйся!");
        playAudioPrompt(AUDIO_GET_READY);  // Голосова підказка
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);
        survivalCorrectPresses = 0;
        survivalTotalPresses = 0;
        waitForReaction = false;
        survivalGameStartTime = lv_tick_get();
        lastSurvivalTargetButton = -1;
        strip_Clear();
        break;

    case ST_STATE_COUNTDOWN:
    {
        unsigned long elapsed = lv_tick_get() - survivalGameStartTime;
        unsigned long remaining = (survivalGameDuration > elapsed) ? (survivalGameDuration - elapsed) : 0;
        char countdown_text[32];
        snprintf(countdown_text, sizeof(countdown_text), "Час: %lu сек", remaining / 1000);
        lv_label_set_text(info_label, countdown_text);
    }
    break;

    case ST_STATE_START_MESSAGE:
        lv_label_set_text(info_label, "СТАРТ!");
        playAudioPrompt(AUDIO_START);  // Голосова підказка
        lv_obj_set_style_text_color(info_label, lv_color_hex(0x00FF00), 0);
        strip_Clear();
        break;

    case ST_STATE_FAST_GAMEPLAY:
    {
        targetButton = get_random_button_avoiding_last(lastSurvivalTargetButton);
        lastSurvivalTargetButton = targetButton;
        strip_SetPixelColor(targetButton, RgbColor(0, 255, 0)); // Green
        strip_Show();
        waitForReaction = true;

        // Update HUD
        unsigned long elapsed = lv_tick_get() - survivalGameStartTime;
        unsigned long remaining = (survivalGameDuration > elapsed) ? (survivalGameDuration - elapsed) : 0;
        char hud_text[64];
        snprintf(hud_text, sizeof(hud_text), "Час: %lus  Очки: %d", remaining / 1000, survivalCorrectPresses);
        lv_label_set_text(round_label, hud_text);
    }
    break;

    case ST_STATE_STOP_MESSAGE:
        lv_label_set_text(info_label, "СТОП!");
        playAudioPrompt(AUDIO_STOP);  // Голосова підказка
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        strip_Clear();
        Serial.println("ST_STATE_STOP_MESSAGE");
        break;

    case ST_STATE_PRE_ROUND_DELAY:
        targetButton = get_random_button_avoiding_last(lastSurvivalTargetButton);
        lastSurvivalTargetButton = targetButton;
        survivalRoundTimer = lv_tick_get() + random(SURVIVAL_PRE_ROUND_MIN_DELAY, SURVIVAL_PRE_ROUND_MAX_DELAY);
        lv_label_set_text(info_label, "Чекай світла...");
        break;

    case ST_STATE_WAIT_FOR_PRESS:
        strip_SetPixelColor(targetButton, RgbColor(0, 255, 0)); // Green
        strip_Show();
        waitForReaction = true;
        lv_label_set_text(info_label, "Натискай!");
        break;

    case ST_STATE_WRONG_PRESS:
        lv_label_set_text(info_label, "Неправильно!");
        playAudioPrompt(AUDIO_WRONG);  // Голосова підказка
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        break;

    case ST_STATE_GAME_OVER_TIME:
        lv_label_set_text(info_label, "Час вийшов!");
        playAudioPrompt(AUDIO_TIMEOUT);  // Голосова підказка
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        break;

    case ST_STATE_GAME_OVER_MISTAKE:
        lv_label_set_text(info_label, "ПОМИЛКА!\nГру завершено.");
        playAudioPrompt(AUDIO_GAME_OVER);  // Голосова підказка
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        break;

    case ST_STATE_SHOW_RESULTS:
        display_survival_results();
        break;

    case ST_STATE_GAME_OVER_MENU:
        // Results already displayed, no additional buttons
        break;

    default:
        break;
    }
}

static void check_button_presses_survival()
{
    if (survivalTimeState != ST_STATE_WAIT_FOR_PRESS && survivalTimeState != ST_STATE_FAST_GAMEPLAY)
        return;

    uint16_t current_button_state = expanderRead();

    for (int i = 0; i < NUM_LEDS; i++)
    {
        bool was_pressed = !(last_button_state & (1 << i));
        bool is_pressed = !(current_button_state & (1 << i));

        if (!was_pressed && is_pressed) // Button just pressed
        {
            waitForReaction = false;
            survivalTotalPresses++;
            last_interaction_time = lv_tick_get();

            if (i == targetButton)
            {
                survivalCorrectPresses++;
                strip_SetPixelColor(i, RgbColor(0, 255, 0)); // Green feedback

                if (survivalTimeState == ST_STATE_FAST_GAMEPLAY)
                {
                    // Continue immediately
                    set_survival_time_state(ST_STATE_FAST_GAMEPLAY);
                }
                else
                {
                    set_survival_time_state(ST_STATE_PRE_ROUND_DELAY);
                }
            }
            else
            {
                strip_SetPixelColor(i, RgbColor(255, 0, 0)); // Red feedback
                Serial.printf("Surv: Wrong %d\n", i);

                if (survivalTimeState == ST_STATE_FAST_GAMEPLAY)
                {
                    set_survival_time_state(ST_STATE_STOP_MESSAGE);
                }
                else
                {
                    set_survival_time_state(ST_STATE_WRONG_PRESS);
                }
            }
            break;
        }
    }

    last_button_state = current_button_state;
}

void run_survival_time_trainer()
{
    // Check for time up
    if (survivalTimeState != ST_STATE_IDLE && survivalTimeState != ST_STATE_GET_READY &&
        survivalTimeState != ST_STATE_GAME_OVER_TIME && survivalTimeState != ST_STATE_GAME_OVER_MISTAKE &&
        survivalTimeState != ST_STATE_SHOW_RESULTS && survivalTimeState != ST_STATE_GAME_OVER_MENU &&
        survivalTimeState != ST_STATE_STOP_MESSAGE)
    {
        if (lv_tick_get() - survivalGameStartTime >= survivalGameDuration)
        {
            set_survival_time_state(ST_STATE_STOP_MESSAGE);
            return;
        }
    }

    switch (survivalTimeState)
    {
    case ST_STATE_GET_READY:
        if (lv_tick_get() - survivalRoundTimer > GET_READY_DURATION)
            set_survival_time_state(ST_STATE_COUNTDOWN);
        break;

    case ST_STATE_COUNTDOWN:
        if (lv_tick_get() - survivalRoundTimer > 1000)
            set_survival_time_state(ST_STATE_START_MESSAGE);
        break;

    case ST_STATE_START_MESSAGE:
        if (lv_tick_get() - survivalRoundTimer > 1000)
            set_survival_time_state(ST_STATE_FAST_GAMEPLAY);
        break;

    case ST_STATE_FAST_GAMEPLAY:
        check_button_presses_survival();
        break;

    case ST_STATE_STOP_MESSAGE:

        // Serial.printf("%d %d", lv_tick_get(), survivalRoundTimer);
        if (lv_tick_get() - survivalRoundTimer > 1500)
            set_survival_time_state(ST_STATE_SHOW_RESULTS);
        break;

    case ST_STATE_PRE_ROUND_DELAY:
        if (lv_tick_get() > survivalRoundTimer)
            set_survival_time_state(ST_STATE_WAIT_FOR_PRESS);
        break;

    case ST_STATE_WAIT_FOR_PRESS:
        check_button_presses_survival();
        break;

    case ST_STATE_WRONG_PRESS:
        if (lv_tick_get() - survivalRoundTimer > ST_WRONG_PRESS_DURATION)
            set_survival_time_state(ST_STATE_GAME_OVER_MISTAKE);
        break;

    case ST_STATE_GAME_OVER_TIME:
    case ST_STATE_GAME_OVER_MISTAKE:
        if (lv_tick_get() - survivalRoundTimer > ST_GAME_OVER_MESSAGE_DURATION)
            set_survival_time_state(ST_STATE_SHOW_RESULTS);
        break;

    case ST_STATE_SHOW_RESULTS:
        if (lv_tick_get() - survivalRoundTimer > SURVIVAL_RESULTS_DISPLAY_DURATION)
            set_survival_time_state(ST_STATE_GAME_OVER_MENU);
        break;

    case ST_STATE_GAME_OVER_MENU:
        break;

    default:
        break;
    }

    // Serial.println("run_survival_time_trainer end");
}

static void display_survival_results()
{
    Serial.println("display_survival_results");

    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(round_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    bool newRecord = is_new_record(survivalCorrectPresses, currentSurvivalDurationMinutes);
    int currentRecord = get_survival_record(currentSurvivalDurationMinutes);

    if (newRecord)
    {
        save_survival_record(currentSurvivalDurationMinutes, survivalCorrectPresses);
    }

    // === Оновлюємо статистику пацієнта в RAM (збереження у Flash - через delayed callback) ===
    PatientStats *stats = &patientStats[currentPatientIndex];
    stats->reaction_sessions++;
    // Записуємо в історію (результат, тривалість у хвилинах)
    addReactionSession(currentPatientIndex, survivalCorrectPresses, currentSurvivalDurationMinutes);
    savePatientStats(currentPatientIndex); // Зберігаємо до SD
    // ==========================================================================================

    char results_text[64];
    if (newRecord)
    {
        snprintf(results_text, sizeof(results_text),
                 "РЕКОРД! %d",
                 survivalCorrectPresses);
        lv_obj_set_style_text_color(results_label, lv_color_hex(0x00FF00), 0);
    }
    else
    {
        snprintf(results_text, sizeof(results_text),
                 "%d / %d",
                 survivalCorrectPresses, currentRecord);
        lv_obj_set_style_text_color(results_label, lv_color_white(), 0);
    }
    
    lv_obj_set_style_text_font(results_label, &lv_lilita_one_regular_96, 0);
    lv_label_set_text(results_label, results_text);
    lv_obj_set_style_text_align(results_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(results_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_update_layout(results_label);

    Serial.println("Results displayed");
}


static void back_to_menu_event_handler(lv_event_t *e)
{
    if (isScreenTransitionActive()) return;
    Serial.println("Reaction: Back to menu");
    current_state = STATE_REACTION_SUBMENU;
    set_time_trial_state(TT_STATE_IDLE);
    set_survival_time_state(ST_STATE_IDLE);
    
    // Async transition to avoid deleting source button in callback
    markScreenTransition();
    lv_async_call(async_open_reaction_submenu, NULL);
}

// Helper function
int get_random_button_avoiding_last(int lastButton)
{
    if (NUM_LEDS <= 1)
        return 0;

    int newButton;
    do
    {
        newButton = random(NUM_LEDS);
    } while (newButton == lastButton);

    return newButton;
}

// Duration setters
void set_survival_duration_1_min()
{
    survivalGameDuration = 1 * 60 * 1000UL;
    currentSurvivalDurationMinutes = 1;
}
void set_survival_duration_2_min()
{
    survivalGameDuration = 2 * 60 * 1000UL;
    currentSurvivalDurationMinutes = 2;
}
void set_survival_duration_3_min()
{
    survivalGameDuration = 3 * 60 * 1000UL;
    currentSurvivalDurationMinutes = 3;
}

// Records functions
// Binary layout: [int32 record_2min][int32 record_3min][int32 record_4min]
#define SURVIVAL_RECORDS_PATH "/survival_records.bin"

void load_survival_records()
{
    survivalRecord2Min = 0;
    survivalRecord3Min = 0;
    survivalRecord4Min = 0;

    File f = SD_MMC.open(SURVIVAL_RECORDS_PATH, FILE_READ);
    if (f) {
        int32_t buf[3] = {0, 0, 0};
        if (f.read((uint8_t *)buf, sizeof(buf)) == sizeof(buf)) {
            survivalRecord2Min = buf[0];
            survivalRecord3Min = buf[1];
            survivalRecord4Min = buf[2];
        }
        f.close();
    }

    Serial.println("Records loaded:");
    Serial.printf("2 min: %d\n", survivalRecord2Min);
    Serial.printf("3 min: %d\n", survivalRecord3Min);
    Serial.printf("4 min: %d\n", survivalRecord4Min);
}

void save_survival_record(int duration, int score)
{
    bool changed = false;
    switch (duration)
    {
    case 2:
        if (score > survivalRecord2Min) { survivalRecord2Min = score; changed = true; Serial.printf("New record 2 min: %d\n", score); }
        break;
    case 3:
        if (score > survivalRecord3Min) { survivalRecord3Min = score; changed = true; Serial.printf("New record 3 min: %d\n", score); }
        break;
    case 4:
        if (score > survivalRecord4Min) { survivalRecord4Min = score; changed = true; Serial.printf("New record 4 min: %d\n", score); }
        break;
    }

    if (changed) {
        File f = SD_MMC.open(SURVIVAL_RECORDS_PATH, FILE_WRITE);
        if (f) {
            int32_t buf[3] = { survivalRecord2Min, survivalRecord3Min, survivalRecord4Min };
            f.write((const uint8_t *)buf, sizeof(buf));
            f.close();
        }
    }
}

int get_survival_record(int duration)
{
    switch (duration)
    {
    case 2:
        return survivalRecord2Min;
    case 3:
        return survivalRecord3Min;
    case 4:
        return survivalRecord4Min;
    default:
        return 0;
    }
}

bool is_new_record(int score, int duration)
{
    switch (duration)
    {
    case 2:
        return score > survivalRecord2Min;
    case 3:
        return score > survivalRecord3Min;
    case 4:
        return score > survivalRecord4Min;
    default:
        return false;
    }
}