#include "globals.h"
#include <Preferences.h>

// === КОЛЬОРИ ===
RgbColor red = {255, 0, 0};
RgbColor green = {0, 255, 0};
RgbColor black = {0, 0, 0};
RgbColor neonBlue = {0, 255, 255};
RgbColor brightCyan = {0, 255, 255};
RgbColor accentColor = {255, 128, 0};
RgbColor memoryColor = {255, 255, 0};
RgbColor coordinationColor = {128, 0, 255};
RgbColor accuracyColor = {0, 255, 0};

// === СТАН МЕНЮ ===
MenuState currentMenuState = MAIN_MENU;
bool menuNeedsRedraw = true;

// === ЗМІННІ ЕКРАНУ ПРИВІТАННЯ ===
int welcomeLedIndex = 0;
unsigned long welcomeLedTimer = 0;
int welcomeLoopsDone = 0;

// === АНІМАЦІЯ ЕКРАНУ ПРИВІТАННЯ ===
bool welcomeAnimationActive = false;
uint32_t lastWelcomeFrame = 0;

// === СТАН КНОПОК ===
unsigned long lastButtonPressTime[16] = {0};
uint16_t lastButtonStatePCF = 0;

// === КНОПКИ МЕНЮ ===
Button_t reactionTrainerButton, memoryTrainerButton, coordinationTrainerButton, accuracyTrainerButton;
Button_t backButton, survivalModeButton, timeTrialModeButton;
Button_t survivalTime1MinButton, survivalTime2MinButton, survivalTime3MinButton, survivalBackButton;
Button_t coordinationEasyButton, coordinationHardButton, coordinationBackButton;
Button_t memoryPlayAgainButton, memoryExitButton;
Button_t coordinationPlayAgainButton, coordinationExitButton;
Button_t accuracyPlayAgainButton, accuracyExitButton;
Button_t timeTrialPlayAgainButton, timeTrialExitButton;
Button_t survivalTimePlayAgainButton, survivalTimeExitButton;
Button_t accuracyEasyButton, accuracyNormalButton, accuracyHardButton, accuracyBackFromDiffButton;
Button_t inGameBackButton;

// === ЗМІННІ ТРЕНАЖЕРІВ ===
bool roundResult = false;
bool feedbackSuccess = false;
int accuracyHits = 0;
int accuracyMissed = 0;

// === ЗМІННІ РЕКОРДІВ ===
int survivalRecord2Min = 0;
int survivalRecord3Min = 0;
int survivalRecord4Min = 0;
int currentSurvivalDurationMinutes = 0;

// === ЗМІННІ ЗАПОБІГАННЯ ПОВТОРЕНЬ ===
int lastSurvivalTargetButton = -1;

// === СИСТЕМА ПАЦІЄНТІВ ===
int currentPatientIndex = 0;  // 0 = гість
PatientStats patientStats[PATIENT_COUNT];

// Ініціалізація системи пацієнтів
void initPatientSystem()
{
    // Ініціалізуємо всі статистики нулями
    memset(patientStats, 0, sizeof(patientStats));
    
    // Завантажуємо збережені дані для всіх пацієнтів
    Preferences prefs;
    prefs.begin("patients", true);  // Тільки читання
    for (int i = 0; i < PATIENT_COUNT; i++)
    {
        char key[16];
        snprintf(key, sizeof(key), "p%d", i);
        prefs.getBytes(key, &patientStats[i], sizeof(PatientStats));
    }
    prefs.end();
    
    Serial.println("[ПАЦІЄНТИ] Система пацієнтів ініціалізована");
}

// Збереження статистики пацієнта у флеш
void savePatientStats(int patientIndex)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;
    
    char key[16];
    snprintf(key, sizeof(key), "p%d", patientIndex);
    
    Preferences prefs;
    prefs.begin("patients", false);
    prefs.putBytes(key, &patientStats[patientIndex], sizeof(PatientStats));
    prefs.end();
    
    Serial.printf("[ПАЦІЄНТИ] Збережено статистику пацієнта %d\n", patientIndex);
}

// Завантаження статистики пацієнта з флеш
void loadPatientStats(int patientIndex)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;
    
    char key[16];
    snprintf(key, sizeof(key), "p%d", patientIndex);
    
    Preferences prefs;
    prefs.begin("patients", true);
    size_t len = prefs.getBytes(key, &patientStats[patientIndex], sizeof(PatientStats));
    prefs.end();
    
    if (len == 0)
    {
        // Немає збережених даних - обнулюємо
        memset(&patientStats[patientIndex], 0, sizeof(PatientStats));
    }
}

// Очищення статистики пацієнта
void clearPatientStats(int patientIndex)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;
    
    memset(&patientStats[patientIndex], 0, sizeof(PatientStats));
    savePatientStats(patientIndex);
    
    Serial.printf("[ПАЦІЄНТИ] Очищено статистику пацієнта %d\n", patientIndex);
}