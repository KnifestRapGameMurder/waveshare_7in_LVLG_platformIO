#include "reaction_trainer.h"
#include "hardware_abstraction.h"
#include "app_screens.h"
#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>

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
static uint16_t last_button_state = 0xFFFF; // Start with all buttons released

// === Survival Variables ===
static SurvivalTimeState survivalTimeState = ST_STATE_IDLE;
static unsigned long survivalGameDuration = 0;
static unsigned long survivalGameStartTime = 0;
static int survivalCorrectPresses = 0;
static int survivalTotalPresses = 0;
static unsigned long survivalRoundTimer = 0;
static int lastSurvivalTargetButton = -1;
static int currentSurvivalDurationMinutes = 1;

// === Records ===
static int survivalRecord2Min = 0;
static int survivalRecord3Min = 0;
static int survivalRecord4Min = 0;

// === UI Elements ===
static lv_obj_t *reaction_screen = NULL;
static lv_obj_t *round_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_obj_t *results_label = NULL;
static lv_obj_t *play_again_btn = NULL;
static lv_obj_t *exit_btn = NULL;

// === Forward Declarations ===
static void update_round_display();
static void check_button_presses_time_trial();
static void check_button_presses_survival();
static void display_time_trial_results();
static void display_survival_results();
static void create_game_over_menu();
static void game_over_menu_event_handler(lv_event_t *e);
static void back_to_menu_event_handler(lv_event_t *e);
static int get_random_button_avoiding_last(int lastButton);

// External font and preferences
extern const lv_font_t minecraft_ten_48;
extern Preferences preferences;

void create_reaction_trainer_screen()
{
    // Clean the screen
    lv_obj_clean(lv_scr_act());

    // Create main screen container
    reaction_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(reaction_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(reaction_screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(reaction_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Create round label
    round_label = lv_label_create(reaction_screen);
    lv_obj_set_style_text_font(round_label, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(round_label, lv_color_white(), 0);
    lv_obj_align(round_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create info label
    info_label = lv_label_create(reaction_screen);
    lv_obj_set_style_text_font(info_label, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);

    // Create results label (hidden initially)
    results_label = lv_label_create(reaction_screen);
    lv_obj_set_style_text_font(results_label, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(results_label, lv_color_white(), 0);
    lv_obj_align(results_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Create back button
    lv_obj_t *back_btn = lv_btn_create(reaction_screen);
    lv_obj_set_size(back_btn, 200, 80);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x666666), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(back_btn, lv_color_white(), 0);
    lv_obj_set_style_border_width(back_btn, 2, 0);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "НАЗАД");
    lv_obj_set_style_text_font(back_label, &minecraft_ten_48, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_btn, back_to_menu_event_handler, LV_EVENT_CLICKED, NULL);

    // Initialize game state
    set_time_trial_state(TT_STATE_GET_READY);
}

void set_time_trial_state(TimeTrialState newState)
{
    timeTrialState = newState;
    timeTrialTimer = lv_tick_get();

    switch (timeTrialState)
    {
    case TT_STATE_IDLE:
        strip_Clear();
        break;

    case TT_STATE_GET_READY:
        lv_label_set_text(info_label, "Приготуйся!");
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);
        currentTTRound = 0;
        for (int i = 0; i < TOTAL_TT_ROUNDS; i++)
            reactionTimes[i] = 0;
        strip_Clear();
        update_round_display();
        break;

    case TT_STATE_PRE_ROUND_DELAY:
        targetButton = random(NUM_LEDS);
        timeTrialTimer = lv_tick_get() + random(500, 2000);
        lv_label_set_text(info_label, "Чекай світла...");
        update_round_display();
        break;

    case TT_STATE_WAIT_FOR_PRESS:
        strip_SetPixelColor(targetButton, RgbColor(0, 255, 0)); // Green
        strip_Show();
        waitForReaction = true;
        reactionStart = lv_tick_get();
        lv_label_set_text(info_label, "Натискай!");
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
        }
        else
        {
            lv_label_set_text(info_label, "Таймаут!");
        }
        timeTrialTimer = lv_tick_get() + RESULT_DISPLAY_DURATION;
        break;

    case TT_STATE_NEXT_ROUND_DELAY:
        break;

    case TT_STATE_GAME_OVER:
        lv_label_set_text(info_label, "Гра Завершена!");
        timeTrialTimer = lv_tick_get() + 3000;
        break;

    case TT_STATE_GAME_OVER_MENU:
        display_time_trial_results();
        break;

    case TT_STATE_WAIT_FOR_EXIT:
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

            if (i == targetButton)
            {
                unsigned long rT = lv_tick_get() - reactionStart;
                Serial.printf("RT: %lu\n", rT);
                reactionTimes[currentTTRound] = rT;
                strip_SetPixelColor(i, RgbColor(0, 255, 0)); // Green feedback
                strip_Show();
                delay(100);
            }
            else
            {
                Serial.println("Wrong btn!");
                strip_SetPixelColor(i, RgbColor(255, 0, 0)); // Red feedback
                strip_Show();
                delay(100);
                reactionTimes[currentTTRound] = 0;
            }

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
            if (currentTTRound < TOTAL_TT_ROUNDS)
            {
                set_time_trial_state(TT_STATE_NEXT_ROUND_DELAY);
            }
            else
            {
                set_time_trial_state(TT_STATE_GAME_OVER);
            }
        }
        break;

    case TT_STATE_NEXT_ROUND_DELAY:
        if (lv_tick_get() > timeTrialTimer)
            set_time_trial_state(TT_STATE_PRE_ROUND_DELAY);
        break;

    case TT_STATE_GAME_OVER:
        if (lv_tick_get() > timeTrialTimer)
            set_time_trial_state(TT_STATE_GAME_OVER_MENU);
        break;

    case TT_STATE_GAME_OVER_MENU:
        break;

    case TT_STATE_WAIT_FOR_EXIT:
        break;

    default:
        break;
    }
}

static void display_time_trial_results()
{
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    unsigned long totalReactionTime = 0;
    int validRounds = 0;
    for (int i = 0; i < TOTAL_TT_ROUNDS; i++)
    {
        if (reactionTimes[i] > 0 && reactionTimes[i] <= TIMEOUT_REACTION)
        {
            totalReactionTime += reactionTimes[i];
            validRounds++;
        }
    }

    char results_text[256];
    if (validRounds > 0)
    {
        snprintf(results_text, sizeof(results_text),
                 "Результати Часу Реакції:\n\nСередній час: %lu мс\nТаймаутів: %d",
                 totalReactionTime / validRounds, TOTAL_TT_ROUNDS - validRounds);
    }
    else
    {
        snprintf(results_text, sizeof(results_text),
                 "Результати Часу Реакції:\n\nНемає успішних спроб.");
    }

    lv_label_set_text(results_label, results_text);
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
        lv_obj_clear_flag(info_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);
        survivalCorrectPresses = 0;
        survivalTotalPresses = 0;
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
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        strip_Clear();
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
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        break;

    case ST_STATE_GAME_OVER_TIME:
        lv_label_set_text(info_label, "Час вийшов!");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        break;

    case ST_STATE_GAME_OVER_MISTAKE:
        lv_label_set_text(info_label, "ПОМИЛКА!\nГру завершено.");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0xFF0000), 0);
        break;

    case ST_STATE_SHOW_RESULTS:
        display_survival_results();
        break;

    case ST_STATE_GAME_OVER_MENU:
        create_game_over_menu();
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

            if (i == targetButton)
            {
                survivalCorrectPresses++;
                strip_SetPixelColor(i, RgbColor(0, 255, 0)); // Green feedback
                strip_Show();
                delay(50);
                strip_Clear();
                strip_Show();

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
                strip_Show();
                delay(100);
                strip_Clear();
                strip_Show();
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
}

static void display_survival_results()
{
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Check for new record
    bool newRecord = is_new_record(survivalCorrectPresses, currentSurvivalDurationMinutes);
    int currentRecord = get_survival_record(currentSurvivalDurationMinutes);

    // Save record if needed
    if (newRecord)
    {
        save_survival_record(currentSurvivalDurationMinutes, survivalCorrectPresses);
    }

    char results_text[512];
    int offset = 0;

    if (newRecord)
    {
        offset += snprintf(results_text + offset, sizeof(results_text) - offset, "НОВИЙ РЕКОРД!\n\n");
    }

    offset += snprintf(results_text + offset, sizeof(results_text) - offset,
                       "Результати Виживання:\n\nПравильних: %d\nВсього спроб: %d\n",
                       survivalCorrectPresses, survivalTotalPresses);

    // Calculate game time
    unsigned long gameTimeMs = lv_tick_get() - survivalGameStartTime;
    unsigned long gameTimeSec = gameTimeMs / 1000;
    offset += snprintf(results_text + offset, sizeof(results_text) - offset,
                       "Гра тривала: %lu сек\n", gameTimeSec);

    // Show current record
    int recordToShow = newRecord ? survivalCorrectPresses : currentRecord;
    offset += snprintf(results_text + offset, sizeof(results_text) - offset,
                       "Рекорд (%d хв): %d", currentSurvivalDurationMinutes, recordToShow);

    lv_label_set_text(results_label, results_text);
}

static void create_game_over_menu()
{
    // Hide other labels
    lv_obj_add_flag(info_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(results_label, LV_OBJ_FLAG_HIDDEN);

    // Create play again button
    play_again_btn = lv_btn_create(reaction_screen);
    lv_obj_set_size(play_again_btn, 300, 80);
    lv_obj_align(play_again_btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(play_again_btn, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_bg_color(play_again_btn, lv_color_hex(0x00AA00), LV_STATE_PRESSED);

    lv_obj_t *play_label = lv_label_create(play_again_btn);
    lv_label_set_text(play_label, "Грати Знову");
    lv_obj_set_style_text_font(play_label, &minecraft_ten_48, 0);
    lv_obj_center(play_label);

    lv_obj_add_event_cb(play_again_btn, game_over_menu_event_handler, LV_EVENT_CLICKED, (void *)0);

    // Create exit button
    exit_btn = lv_btn_create(reaction_screen);
    lv_obj_set_size(exit_btn, 300, 80);
    lv_obj_align(exit_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xAA0000), LV_STATE_PRESSED);

    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "Вихід");
    lv_obj_set_style_text_font(exit_label, &minecraft_ten_48, 0);
    lv_obj_center(exit_label);

    lv_obj_add_event_cb(exit_btn, game_over_menu_event_handler, LV_EVENT_CLICKED, (void *)1);
}

static void game_over_menu_event_handler(lv_event_t *e)
{
    int action = (int)(intptr_t)lv_event_get_user_data(e);

    if (action == 0) // Play again
    {
        Serial.println("Reaction Menu: Play Again");
        set_survival_time_state(ST_STATE_GET_READY);
    }
    else if (action == 1) // Exit
    {
        Serial.println("Reaction Menu: Exit");
        current_state = STATE_REACTION_SUBMENU;
        set_survival_time_state(ST_STATE_IDLE);
        create_reaction_submenu();
    }
}

static void back_to_menu_event_handler(lv_event_t *e)
{
    Serial.println("Reaction: Back to menu");
    current_state = STATE_REACTION_SUBMENU;
    set_time_trial_state(TT_STATE_IDLE);
    set_survival_time_state(ST_STATE_IDLE);
    create_reaction_submenu();
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
void load_survival_records()
{
    preferences.begin("survival", false);
    survivalRecord2Min = preferences.getInt("record_2min", 0);
    survivalRecord3Min = preferences.getInt("record_3min", 0);
    survivalRecord4Min = preferences.getInt("record_4min", 0);
    preferences.end();

    Serial.println("Records loaded:");
    Serial.printf("2 min: %d\n", survivalRecord2Min);
    Serial.printf("3 min: %d\n", survivalRecord3Min);
    Serial.printf("4 min: %d\n", survivalRecord4Min);
}

void save_survival_record(int duration, int score)
{
    preferences.begin("survival", false);

    switch (duration)
    {
    case 2:
        if (score > survivalRecord2Min)
        {
            survivalRecord2Min = score;
            preferences.putInt("record_2min", score);
            Serial.printf("New record 2 min: %d\n", score);
        }
        break;
    case 3:
        if (score > survivalRecord3Min)
        {
            survivalRecord3Min = score;
            preferences.putInt("record_3min", score);
            Serial.printf("New record 3 min: %d\n", score);
        }
        break;
    case 4:
        if (score > survivalRecord4Min)
        {
            survivalRecord4Min = score;
            preferences.putInt("record_4min", score);
            Serial.printf("New record 4 min: %d\n", score);
        }
        break;
    }

    preferences.end();
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