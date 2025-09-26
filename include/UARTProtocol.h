#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <Arduino.h>

// UART Protocol Version
#define PROTOCOL_VERSION "1.0"

// Message delimiters and separators
#define MSG_DELIMITER '\n'
#define MSG_SEPARATOR ':'

// Maximum message length
#define MAX_MESSAGE_LENGTH 64

// Protocol message types
enum MessageType
{
    // System messages
    MSG_HANDSHAKE, // Initial connection
    MSG_STATUS,    // Periodic status updates
    MSG_ACK,       // Acknowledgment
    MSG_ERROR,     // Error reporting

    // Button events (Slave -> Master)
    MSG_BUTTON_PRESSED,  // Button press event
    MSG_BUTTON_RELEASED, // Button release event
    MSG_BUTTON_STATE,    // All buttons state (16-bit)

    // LED control (Master -> Slave)
    MSG_LED_SET_PIXEL,  // Set single LED color
    MSG_LED_SET_ALL,    // Set all LEDs same color
    MSG_LED_CLEAR,      // Clear all LEDs
    MSG_LED_EFFECT,     // Start LED effect
    MSG_LED_BRIGHTNESS, // Set LED brightness

    // Hall sensor events (Slave -> Master)
    MSG_HALL_DETECTED, // Hall sensor detection
    MSG_HALL_REMOVED,  // Hall sensor removal

    MSG_UNKNOWN // Unknown message type
};

// LED color definitions (standardized across boards)
enum ProtocolColor
{
    PROTO_COLOR_BLACK = 0,
    PROTO_COLOR_RED,
    PROTO_COLOR_GREEN,
    PROTO_COLOR_BLUE,
    PROTO_COLOR_WHITE,
    PROTO_COLOR_YELLOW,
    PROTO_COLOR_CYAN,
    PROTO_COLOR_MAGENTA,
    PROTO_COLOR_ORANGE
};

// LED effect types (standardized across boards)
enum ProtocolEffect
{
    PROTO_EFFECT_OFF = 0,
    PROTO_EFFECT_SOLID,
    PROTO_EFFECT_RAINBOW_WAVE,
    PROTO_EFFECT_COLOR_CYCLE,
    PROTO_EFFECT_BREATHING,
    PROTO_EFFECT_SPARKLE,
    PROTO_EFFECT_CHASE,
    PROTO_EFFECT_BOUNCE
};

// Message structure for parsing
struct ProtocolMessage
{
    MessageType type;
    uint8_t param1;  // General purpose parameter 1
    uint8_t param2;  // General purpose parameter 2
    uint16_t param3; // General purpose parameter 3 (16-bit)
    String data;     // Additional string data
    bool valid;      // Message validity flag
};

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
    String createHandshakeMessage(const String &deviceInfo);
    String createStatusMessage(unsigned long timestamp, uint16_t buttonState);
    String createAckMessage(const String &originalMessage);
    String createErrorMessage(const String &errorDescription);

    String createButtonPressedMessage(uint8_t buttonIndex);
    String createButtonReleasedMessage(uint8_t buttonIndex);
    String createButtonStateMessage(uint16_t buttonState);

    String createLEDSetPixelMessage(uint8_t ledIndex, ProtocolColor color);
    String createLEDSetAllMessage(ProtocolColor color);
    String createLEDClearMessage();
    String createLEDEffectMessage(ProtocolEffect effect);
    String createLEDBrightnessMessage(uint8_t brightness);

    String createHallDetectedMessage(unsigned long detectionCount);
    String createHallRemovedMessage();

    // Message parsing functions
    ProtocolMessage parseMessage(const String &message);
    MessageType getMessageType(const String &typeString);
    String getMessageTypeString(MessageType type);

    // Communication functions
    void sendMessage(const String &message);
    bool receiveMessage(ProtocolMessage &message);

    // Utility functions
    ProtocolColor rgbToProtocolColor(uint8_t r, uint8_t g, uint8_t b);
    void protocolColorToRGB(ProtocolColor color, uint8_t &r, uint8_t &g, uint8_t &b);
    String getProtocolColorName(ProtocolColor color);
    String getProtocolEffectName(ProtocolEffect effect);

    // Validation functions
    bool isValidButtonIndex(uint8_t index);
    bool isValidLEDIndex(uint8_t index);
    bool isValidColor(ProtocolColor color);
    bool isValidEffect(ProtocolEffect effect);
    bool isValidBrightness(uint8_t brightness);
};

// Message format examples:
// HANDSHAKE:SLAVE:16_BUTTONS_16_LEDS
// STATUS:12345:ABCD
// ACK:BUTTON_PRESSED:5
// ERROR:INVALID_LED_INDEX
// BUTTON_PRESSED:5
// BUTTON_RELEASED:5
// BUTTON_STATE:ABCD
// LED_SET_PIXEL:5:RED
// LED_SET_ALL:BLUE
// LED_CLEAR
// LED_EFFECT:RAINBOW_WAVE
// LED_BRIGHTNESS:128
// HALL_DETECTED:42
// HALL_REMOVED

#endif