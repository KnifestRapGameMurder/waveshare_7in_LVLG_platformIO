#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <Arduino.h>

// UART Protocol Version
#define PROTOCOL_VERSION "1.0"

// Message delimiters
#define MSG_DELIMITER '\n'
#define MSG_SEPARATOR ':'

// Maximum message length
#define MAX_MESSAGE_LENGTH 256

// Message types
#define MSG_TYPE_CMD "CMD"
#define MSG_TYPE_LOG "LOG"

// CMD subcommands
#define CMD_BUTTONS "BUTTONS"
#define CMD_LED "LED"
#define CMD_LEDS "LEDS"
#define CMD_REQUEST "REQUEST"
#define CMD_AUDIO "AUDIO"

#define CMD_USE_SERIAL "USE_SERIAL"

// Audio prompt IDs (for voice prompts)
// === Загальні ===
#define AUDIO_GET_READY "GET_READY"       // "Приготуйся!"
#define AUDIO_START "START"               // "Старт!"
#define AUDIO_GO "GO"                     // "Почали!"
#define AUDIO_STOP "STOP"                 // "Стоп!"
#define AUDIO_GAME_OVER "GAME_OVER"       // "Гру завершено!"

// === Результати ===
#define AUDIO_EXCELLENT "EXCELLENT"       // "Відмінно!"
#define AUDIO_GOOD "GOOD"                 // "Добре!"
#define AUDIO_CORRECT "CORRECT"           // "Правильно!"
#define AUDIO_WRONG "WRONG"               // "Неправильно!"
#define AUDIO_NEW_RECORD "NEW_RECORD"     // "Новий рекорд!"

// === Реакція ===
#define AUDIO_WAIT_LIGHT "WAIT_LIGHT"     // "Чекай світла..."
#define AUDIO_PRESS "PRESS"               // "Натискай!"
#define AUDIO_TIMEOUT "TIMEOUT"           // "Час вийшов!"
#define AUDIO_TOO_EARLY "TOO_EARLY"       // "Занадто рано!"
#define AUDIO_TOO_SLOW "TOO_SLOW"         // "Занадто повільно"

// === Пам'ять ===
#define AUDIO_REMEMBER "REMEMBER"         // "Запам'ятовуй..."
#define AUDIO_YOUR_TURN "YOUR_TURN"       // "Твоя черга!"
#define AUDIO_LEVEL_UP "LEVEL_UP"         // "Новий рівень!"

// === Влучність ===
#define AUDIO_HIT_FLASH "HIT_FLASH"       // "Влуч у спалах!"
#define AUDIO_CATCH_TARGET "CATCH_TARGET" // "Спіймай мету!"
#define AUDIO_CATCH_LINK "CATCH_LINK"     // "Спіймай зв'язку!"

// === Координація ===
#define AUDIO_REMEMBER_BTNS "REMEMBER_BTNS" // "Запам'ятай кнопки!"
#define AUDIO_PRESS_BTNS "PRESS_BTNS"       // "Натисни кнопки!"

// === Числа для зворотного відліку ===
#define AUDIO_THREE "THREE"               // "Три"
#define AUDIO_TWO "TWO"                   // "Два"
#define AUDIO_ONE "ONE"                   // "Один"

// CMD LED subcommands
#define CMD_LED_CLEAR "CLEAR"

// Protocol class for message handling
class UARTProtocol
{
private:
    HardwareSerial *serial;
    String inputBuffer;

public:
    // Constructor
    UARTProtocol(HardwareSerial *serialPort);

    // Message creation functions
    String createUseSerialMessage(uint8_t serialIndex);                        // CMD:USE_SERIAL:1
    String createButtonStateMessage(uint16_t buttonState);                     // CMD:BUTTONS:0000111100001111 (16-bit binary as string)
    String createLEDSetPixelMessage(uint8_t ledIndex, const String &hexColor); // CMD:LED:5:FF00AA
    String createLEDSetPixelsMultiMessage(const String &colorList);            // CMD:LEDS:FF0000,00FF00,...
    String createLEDClearMessage(const String &hexColor);                      // CMD:LEDS:CLEAR:FF0000
    String createAudioMessage(const String &audioId);                          // CMD:AUDIO:EXCELLENT
    String createRequestButtonsMessage();                                      // CMD:REQUEST:BUTTONS
    String createLogMessage(const String &logData);                            // LOG:logData

    // Communication functions
    void sendMessage(const String &message);
    bool receiveMessage(String &message); // Returns raw message

    // Validation functions
    bool isCMDMessage(const String &message); // Checks if message starts with CMD
    bool isLOGMessage(const String &message); // Checks if message starts with LOG

    // Parsing functions
    bool parseCMDMessage(const String &data, String &command, String &param1, String &param2);
    void handleLogMessage(const String &data); // Print to Serial

    // Utility functions
    String uint16ToBinaryString(uint16_t value);            // Convert 16-bit to 16-char binary string
    uint16_t binaryStringToUint16(const String &binaryStr); // Convert back
};

#endif