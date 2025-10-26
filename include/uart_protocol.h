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

#define CMD_USE_SERIAL "USE_SERIAL"

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