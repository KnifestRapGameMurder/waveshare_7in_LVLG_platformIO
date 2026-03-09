#include "globals.h"
#include "uart_protocol.h"
#include <SD_MMC.h>

// Зовнішнє посилання на UART протокол (визначений в main.cpp)
extern UARTProtocol uart_protocol;

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

// === ЗАХИСТ ВІД ФАНТОМНИХ КЛІКІВ ПРИ ПЕРЕХОДІ ЕКРАНІВ ===
uint32_t screen_transition_time = 0;
bool touch_is_active = false;           // Чи палець на екрані
bool wait_for_touch_release = false;    // Чи чекаємо відпускання пальця після переходу

bool isScreenTransitionActive()
{
    uint32_t elapsed = millis() - screen_transition_time;
    
    // Перевіряємо чи активна блокування по wait_for_touch_release
    if (wait_for_touch_release) {
        return true;
    }
    
    // Мінімальний часовий guard
    if (elapsed < 300) { // Повертаємо до 300мс для надійності
        return true;
    }
    
    return false;
}

void markScreenTransition()
{
    screen_transition_time = millis();
    // НЕ скидаємо wait_for_touch_release - він буде скинутий при відпусканні пальця
    Serial.println("[GUARD] Перехід екрану (захист 300мс + wait_for_touch_release)");
}

void markTouchPressed()
{
    touch_is_active = true;
}

void markTouchReleased()
{
    touch_is_active = false;
    // Коли палець відпущений - скидаємо прапорець очікування і оновлюємо час
    if (wait_for_touch_release) {
        wait_for_touch_release = false;
        screen_transition_time = millis();  // Починаємо відлік часового guard
        Serial.println("[GUARD] Палець відпущено - старт часового guard");
    }
}

// === АУДІО ПІДКАЗКИ ===
void playAudioPrompt(const char* audioId)
{
    // Надсилаємо команду аудіо на ESP32 DevKit
    // Формат: CMD:AUDIO:EXCELLENT
    uart_protocol.sendMessage(uart_protocol.createAudioMessage(audioId));
    Serial.printf("[AUDIO] Відправлено: %s\n", audioId);
}

uint8_t currentVolume = 70;  // За замовчуванням 70%

void setVolume(uint8_t volume)
{
    if (volume > 100) volume = 100;
    currentVolume = volume;
    uart_protocol.sendMessage(uart_protocol.createVolumeMessage(volume));
    Serial.printf("[VOLUME] Встановлено: %d\n", volume);
}

// === СИСТЕМА ПАЦІЄНТІВ ===
int currentPatientIndex = 0;  // 0 = гість
PatientStats patientStats[PATIENT_COUNT];

// Ініціалізація системи пацієнтів
void initPatientSystem()
{
    // Ініціалізуємо всі статистики нулями
    memset(patientStats, 0, sizeof(patientStats));

    // Завантажуємо збережені дані з SD-карти для всіх пацієнтів
    for (int i = 0; i < PATIENT_COUNT; i++)
    {
        loadPatientStats(i);
    }

    Serial.println("[ПАЦІЄНТИ] Система пацієнтів ініціалізована (SD-карта)");
}

// Збереження статистики пацієнта на SD-карту (бінарний файл)
void savePatientStats(int patientIndex)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;

    char path[32];
    snprintf(path, sizeof(path), "/patients/p%02d.bin", patientIndex);

    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
        Serial.printf("[ПАЦІЄНТИ] POMOILKA: не вдалось відкрити %s для запису\n", path);
        return;
    }
    f.write((const uint8_t *)&patientStats[patientIndex], sizeof(PatientStats));
    f.close();

    Serial.printf("[ПАЦІЄНТИ] Збережено статистику пацієнта %d -> %s\n", patientIndex, path);
}

// Завантаження статистики пацієнта з SD-карти
void loadPatientStats(int patientIndex)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;

    char path[32];
    snprintf(path, sizeof(path), "/patients/p%02d.bin", patientIndex);

    if (!SD_MMC.exists(path)) {
        // Файл не існує — обнуляємо
        memset(&patientStats[patientIndex], 0, sizeof(PatientStats));
        return;
    }

    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        memset(&patientStats[patientIndex], 0, sizeof(PatientStats));
        return;
    }
    size_t bytesRead = f.read((uint8_t *)&patientStats[patientIndex], sizeof(PatientStats));
    f.close();

    if (bytesRead != sizeof(PatientStats)) {
        // Неповні дані — обнуляємо
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

// Допоміжна функція для додавання запису до циклічного буфера
static void addSessionToHistory(TrainerHistory *history, uint16_t score, uint16_t extra)
{
    history->sessions[history->next_index].timestamp = millis() / 1000;
    history->sessions[history->next_index].score = score;
    history->sessions[history->next_index].extra = extra;
    
    history->next_index = (history->next_index + 1) % SESSION_HISTORY_SIZE;
    if (history->count < SESSION_HISTORY_SIZE) {
        history->count++;
    }
}

// Додавання сесії тренажера влучності
void addAccuracySession(int patientIndex, uint16_t score, uint16_t accuracy_pct)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;
    addSessionToHistory(&patientStats[patientIndex].accuracy_history, score, accuracy_pct);
}

// Додавання сесії тренажера реакції
void addReactionSession(int patientIndex, uint16_t time_ms, uint16_t attempts)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;
    addSessionToHistory(&patientStats[patientIndex].reaction_history, time_ms, attempts);
}

// Додавання сесії тренажера пам'яті
void addMemorySession(int patientIndex, uint16_t level, uint16_t correct)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;
    addSessionToHistory(&patientStats[patientIndex].memory_history, level, correct);
}

// Додавання сесії тренажера координації
void addCoordinationSession(int patientIndex, uint16_t score, uint16_t hits)
{
    if (patientIndex < 0 || patientIndex >= PATIENT_COUNT) return;
    addSessionToHistory(&patientStats[patientIndex].coordination_history, score, hits);
}