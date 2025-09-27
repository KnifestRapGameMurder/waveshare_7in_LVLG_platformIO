#include "uart_protocol.h"

// Number of LEDs expected for the multi-set command
#define NUM_LEDS 16

// Flag value in param2 indicating the color is a raw hex string in the 'data' field
#define COLOR_TYPE_HEX_STRING 0xFE

UARTProtocol::UARTProtocol(HardwareSerial *serialPort)
{
    serial = serialPort;
    inputBuffer = "";
}

// Message creation functions (Existing functions omitted for brevity unless modified)

String UARTProtocol::createHandshakeMessage(const String &deviceInfo)
{
    return "HANDSHAKE:" + deviceInfo;
}

String UARTProtocol::createStatusMessage(unsigned long timestamp, uint16_t buttonState)
{
    return "STATUS:" + String(timestamp) + ":" + String(buttonState, HEX);
}

String UARTProtocol::createAckMessage(const String &originalMessage)
{
    return "ACK:" + originalMessage;
}

String UARTProtocol::createErrorMessage(const String &errorDescription)
{
    return "ERROR:" + errorDescription;
}

String UARTProtocol::createButtonPressedMessage(uint8_t buttonIndex)
{
    return "BUTTON_PRESSED:" + String(buttonIndex);
}

String UARTProtocol::createButtonReleasedMessage(uint8_t buttonIndex)
{
    return "BUTTON_RELEASED:" + String(buttonIndex);
}

String UARTProtocol::createButtonStateMessage(uint16_t buttonState)
{
    return "BUTTON_STATE:" + String(buttonState, HEX);
}

// LED_SET_PIXEL: Overload for ProtocolColor
String UARTProtocol::createLEDSetPixelMessage(uint8_t ledIndex, ProtocolColor color)
{
    return "LED_SET_PIXEL:" + String(ledIndex) + ":" + getProtocolColorName(color);
}

// NEW: LED_SET_PIXEL: Overload for Hex Color String (e.g., "FF00AA")
String UARTProtocol::createLEDSetPixelMessage(uint8_t ledIndex, const String &hexColor)
{
    // Note: Validation (isHexColorString) is done by the user/caller for cleaner message creation,
    // but the parser will check it upon receipt.
    return "LED_SET_PIXEL:" + String(ledIndex) + ":" + hexColor;
}

// LED_SET_PIXELS_MULTI: Overload for ProtocolColor array
String UARTProtocol::createLEDSetPixelsMultiMessage(const ProtocolColor colors[NUM_LEDS])
{
    String message = "LED_SET_PIXELS_MULTI:";
    for (int i = 0; i < NUM_LEDS; i++)

    {
        message += getProtocolColorName(colors[i]);
        if (i < NUM_LEDS - 1)

        {
            message += ','; // Comma separates the 16 color names
        }
    }
    return message;
}

// NEW: LED_SET_PIXELS_MULTI: Overload for Color List String (e.g., "RED,BLUE,FF00AA,...")
String UARTProtocol::createLEDSetPixelsMultiMessage(const String &colorListString)
{
    return "LED_SET_PIXELS_MULTI:" + colorListString;
}

String UARTProtocol::createLEDSetAllMessage(ProtocolColor color)
{
    return "LED_SET_ALL:" + getProtocolColorName(color);
}

String UARTProtocol::createLEDClearMessage()
{
    return "LED_CLEAR";
}

String UARTProtocol::createLEDEffectMessage(ProtocolEffect effect)
{
    return "LED_EFFECT:" + getProtocolEffectName(effect);
}

String UARTProtocol::createLEDBrightnessMessage(uint8_t brightness)
{
    return "LED_BRIGHTNESS:" + String(brightness);
}

String UARTProtocol::createHallDetectedMessage(unsigned long detectionCount)
{
    return "HALL_DETECTED:" + String(detectionCount);
}

String UARTProtocol::createHallRemovedMessage()
{
    return "HALL_REMOVED";
}

// NEW: Helper function to check if a string is a 6-character hex color code (e.g., "FF00AA")
bool UARTProtocol::isHexColorString(const String &str)
{
    if (str.length() != 6)
        return false;

    for (int i = 0; i < 6; i++)

    {
        char c = str.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))

        {
            return false;
        }
    }
    return true;
}

// Message parsing functions
ProtocolMessage UARTProtocol::parseMessage(const String &message)
{
    ProtocolMessage msg;
    msg.valid = false;
    msg.param1 = 0;
    msg.param2 = 0;
    msg.param3 = 0;
    msg.data = "";

    if (message.length() == 0)

    {
        msg.type = MSG_UNKNOWN;
        return msg;
    }

    // Split message by separator
    int firstSep = message.indexOf(MSG_SEPARATOR);
    String typeStr = message.substring(0, firstSep >= 0 ? firstSep : message.length());

    msg.type = getMessageType(typeStr);

    if (msg.type == MSG_UNKNOWN)

    {
        return msg;
    }

    // Parse parameters based on message type
    String params = firstSep >= 0 ? message.substring(firstSep + 1) : "";

    switch (msg.type)

    {
    case MSG_HANDSHAKE:
        msg.data = params;
        msg.valid = true;
        break;

    case MSG_STATUS:

    {
        int secondSep = params.indexOf(MSG_SEPARATOR);
        if (secondSep >= 0)

        {
            // ... (omitted existing status parsing for brevity)
            msg.param3 = params.substring(0, secondSep).toInt(); // timestamp (use param3 for 32-bit)
            String hexState = params.substring(secondSep + 1);
            msg.param1 = (uint8_t)strtol(hexState.c_str(), NULL, 16);
            // button state low byte
            msg.param2 = (uint8_t)(strtol(hexState.c_str(), NULL, 16) >> 8); // button state high byte
            msg.valid = true;
        }
        break;
    }

    case MSG_BUTTON_PRESSED:
    case MSG_BUTTON_RELEASED:
        msg.param1 = params.toInt();
        msg.valid = isValidButtonIndex(msg.param1);
        break;

    case MSG_BUTTON_STATE:
        msg.param3 = strtol(params.c_str(), NULL, 16);
        msg.valid = true;
        break;

    case MSG_LED_SET_PIXEL:

    {
        int secondSep = params.indexOf(MSG_SEPARATOR);
        if (secondSep >= 0)

        {
            msg.param1 = params.substring(0, secondSep).toInt(); // LED index
            String colorStr = params.substring(secondSep + 1);
            msg.valid = isValidLEDIndex(msg.param1);

            // 1. Try to convert color name to enum
            bool colorFound = false;
            for (int i = PROTO_COLOR_BLACK; i <= PROTO_COLOR_ORANGE; i++)

            {
                if (colorStr.equals(getProtocolColorName((ProtocolColor)i)))

                {
                    msg.param2 = i; // Store enum value
                    colorFound = true;
                    break;
                }
            }

            // 2. If not a named color, check if it's a valid hex string
            if (!colorFound && isHexColorString(colorStr))

            {
                msg.data = colorStr;
                // Store hex string in data
                msg.param2 = COLOR_TYPE_HEX_STRING; // Set flag for hex mode
                colorFound = true;
            }

            msg.valid = msg.valid && colorFound;
        }
        break;
    }

    case MSG_LED_SET_PIXELS_MULTI:

    {
        // Store the raw parameter string in 'data'. This string can contain
        // comma-separated color names (e.g., RED) or hex strings (e.g., FF00AA).
        // The application must handle tokenization and individual color validation.
        msg.data = params;

        // Basic validity check: ensure the parameter string is not empty.
        if (params.length() > 0)
        {
            msg.valid = true;
        }
        else
        {
            // If empty, it's invalid unless cleared, which has its own message.
            msg.valid = false;
        }
        break;
    }

    case MSG_LED_SET_ALL:

    {
        // Convert color name to enum
        for (int i = PROTO_COLOR_BLACK; i <= PROTO_COLOR_ORANGE; i++)

        {
            if (params.equals(getProtocolColorName((ProtocolColor)i)))

            {
                msg.param1 = i;
                msg.valid = isValidColor((ProtocolColor)i);
                break;
            }
        }
        break;
    }

    case MSG_LED_CLEAR:
        msg.valid = true;
        break;

    case MSG_LED_EFFECT:

    {
        // ... (omitted existing effect parsing for brevity)
        // Convert effect name to enum
        for (int i = PROTO_EFFECT_OFF; i <= PROTO_EFFECT_BOUNCE; i++)

        {
            if (params.equals(getProtocolEffectName((ProtocolEffect)i)))

            {
                msg.param1 = i;
                msg.valid = isValidEffect((ProtocolEffect)i);
                break;
            }
        }
        break;
    }

    case MSG_LED_BRIGHTNESS:
        msg.param1 = params.toInt();
        msg.valid = isValidBrightness(msg.param1);
        break;

    case MSG_HALL_DETECTED:
        msg.param3 = params.toInt();
        msg.valid = true;
        break;

    case MSG_HALL_REMOVED:
        msg.valid = true;
        break;

    case MSG_ACK:
    case MSG_ERROR:
        msg.data = params;
        msg.valid = true;
        break;

    default:
        msg.valid = false;
        break;
    }

    return msg;
}

MessageType UARTProtocol::getMessageType(const String &typeString)
{
    if (typeString.equals("HANDSHAKE"))
        return MSG_HANDSHAKE;
    if (typeString.equals("STATUS"))
        return MSG_STATUS;
    if (typeString.equals("ACK"))
        return MSG_ACK;
    if (typeString.equals("ERROR"))
        return MSG_ERROR;
    if (typeString.equals("BUTTON_PRESSED"))
        return MSG_BUTTON_PRESSED;
    if (typeString.equals("BUTTON_RELEASED"))
        return MSG_BUTTON_RELEASED;
    if (typeString.equals("BUTTON_STATE"))
        return MSG_BUTTON_STATE;
    if (typeString.equals("LED_SET_PIXEL"))
        return MSG_LED_SET_PIXEL;
    if (typeString.equals("LED_SET_PIXELS_MULTI"))
        return MSG_LED_SET_PIXELS_MULTI;
    if (typeString.equals("LED_SET_ALL"))
        return MSG_LED_SET_ALL;
    if (typeString.equals("LED_CLEAR"))
        return MSG_LED_CLEAR;
    if (typeString.equals("LED_EFFECT"))
        return MSG_LED_EFFECT;
    if (typeString.equals("LED_BRIGHTNESS"))
        return MSG_LED_BRIGHTNESS;
    if (typeString.equals("HALL_DETECTED"))
        return MSG_HALL_DETECTED;
    if (typeString.equals("HALL_REMOVED"))
        return MSG_HALL_REMOVED;
    return MSG_UNKNOWN;
}

String UARTProtocol::getMessageTypeString(MessageType type)
{
    switch (type)

    {
    case MSG_HANDSHAKE:
        return "HANDSHAKE";
    case MSG_STATUS:
        return "STATUS";
    case MSG_ACK:
        return "ACK";
    case MSG_ERROR:
        return "ERROR";
    case MSG_BUTTON_PRESSED:
        return "BUTTON_PRESSED";
    case MSG_BUTTON_RELEASED:
        return "BUTTON_RELEASED";
    case MSG_BUTTON_STATE:
        return "BUTTON_STATE";
    case MSG_LED_SET_PIXEL:
        return "LED_SET_PIXEL";
    case MSG_LED_SET_PIXELS_MULTI:
        return "LED_SET_PIXELS_MULTI";
    case MSG_LED_SET_ALL:
        return "LED_SET_ALL";
    case MSG_LED_CLEAR:
        return "LED_CLEAR";
    case MSG_LED_EFFECT:
        return "LED_EFFECT";
    case MSG_LED_BRIGHTNESS:
        return "LED_BRIGHTNESS";
    case MSG_HALL_DETECTED:
        return "HALL_DETECTED";
    case MSG_HALL_REMOVED:
        return "HALL_REMOVED";
    default:
        return "UNKNOWN";
    }
}

// Communication functions (omitted for brevity)

void UARTProtocol::sendMessage(const String &message)
{
    if (serial)

    {
        serial->println(message);
    }
}

bool UARTProtocol::receiveMessage(ProtocolMessage &message)
{
    if (!serial || !serial->available())

    {
        return false;
    }

    while (serial->available())

    {
        char c = serial->read();

        if (c == MSG_DELIMITER)

        {
            if (inputBuffer.length() > 0)

            {
                message = parseMessage(inputBuffer);
                inputBuffer = "";
                return message.valid;
            }
        }
        else if (inputBuffer.length() < MAX_MESSAGE_LENGTH - 1)

        {
            inputBuffer += c;
        }
        else
        {
            // Buffer overflow, reset
            inputBuffer = "";
        }
    }

    return false;
}

// Utility functions (omitted for brevity)
ProtocolColor UARTProtocol::rgbToProtocolColor(uint8_t r, uint8_t g, uint8_t b)
{
    if (r == 0 && g == 0 && b == 0)
        return PROTO_COLOR_BLACK;
    if (r == 255 && g == 0 && b == 0)
        return PROTO_COLOR_RED;
    if (r == 0 && g == 255 && b == 0)
        return PROTO_COLOR_GREEN;
    if (r == 0 && g == 0 && b == 255)
        return PROTO_COLOR_BLUE;
    if (r == 255 && g == 255 && b == 255)
        return PROTO_COLOR_WHITE;
    if (r == 255 && g == 255 && b == 0)
        return PROTO_COLOR_YELLOW;
    if (r == 0 && g == 255 && b == 255)
        return PROTO_COLOR_CYAN;
    if (r == 255 && g == 0 && b == 255)
        return PROTO_COLOR_MAGENTA;
    if (r == 255 && g == 165 && b == 0)
        return PROTO_COLOR_ORANGE;
    return PROTO_COLOR_WHITE; // Default fallback
}

void UARTProtocol::protocolColorToRGB(ProtocolColor color, uint8_t &r, uint8_t &g, uint8_t &b)
{
    switch (color)

    {
    case PROTO_COLOR_BLACK:
        r = 0;
        g = 0;
        b = 0;
        break;
    case PROTO_COLOR_RED:
        r = 255;
        g = 0;
        b = 0;
        break;
    case PROTO_COLOR_GREEN:
        r = 0;
        g = 255;
        b = 0;
        break;
    case PROTO_COLOR_BLUE:
        r = 0;
        g = 0;
        b = 255;
        break;
    case PROTO_COLOR_WHITE:
        r = 255;
        g = 255;
        b = 255;
        break;
    case PROTO_COLOR_YELLOW:
        r = 255;
        g = 255;
        b = 0;
        break;
    case PROTO_COLOR_CYAN:
        r = 0;
        g = 255;
        b = 255;
        break;
    case PROTO_COLOR_MAGENTA:
        r = 255;
        g = 0;
        b = 255;
        break;
    case PROTO_COLOR_ORANGE:
        r = 255;
        g = 165;
        b = 0;
        break;
    default:
        r = 255;
        g = 255;
        b = 255;
        break;
    }
}

String UARTProtocol::getProtocolColorName(ProtocolColor color)
{
    switch (color)

    {
    case PROTO_COLOR_BLACK:
        return "BLACK";
    case PROTO_COLOR_RED:
        return "RED";
    case PROTO_COLOR_GREEN:
        return "GREEN";
    case PROTO_COLOR_BLUE:
        return "BLUE";
    case PROTO_COLOR_WHITE:
        return "WHITE";
    case PROTO_COLOR_YELLOW:
        return "YELLOW";
    case PROTO_COLOR_CYAN:
        return "CYAN";
    case PROTO_COLOR_MAGENTA:
        return "MAGENTA";
    case PROTO_COLOR_ORANGE:
        return "ORANGE";
    default:
        return "UNKNOWN";
    }
}

String UARTProtocol::getProtocolEffectName(ProtocolEffect effect)
{
    switch (effect)

    {
    case PROTO_EFFECT_OFF:
        return "OFF";
    case PROTO_EFFECT_SOLID:
        return "SOLID";
    case PROTO_EFFECT_RAINBOW_WAVE:
        return "RAINBOW_WAVE";
    case PROTO_EFFECT_COLOR_CYCLE:
        return "COLOR_CYCLE";
    case PROTO_EFFECT_BREATHING:
        return "BREATHING";
    case PROTO_EFFECT_SPARKLE:
        return "SPARKLE";
    case PROTO_EFFECT_CHASE:
        return "CHASE";
    case PROTO_EFFECT_BOUNCE:
        return "BOUNCE";
    default:
        return "UNKNOWN";
    }
}

// Validation functions (omitted for brevity)
bool UARTProtocol::isValidButtonIndex(uint8_t index)
{
    return index < 16; // 0-15
}

bool UARTProtocol::isValidLEDIndex(uint8_t index)
{
    return index < 16; // 0-15
}

bool UARTProtocol::isValidColor(ProtocolColor color)
{
    return color >= PROTO_COLOR_BLACK && color <= PROTO_COLOR_ORANGE;
}

bool UARTProtocol::isValidEffect(ProtocolEffect effect)
{
    return effect >= PROTO_EFFECT_OFF && effect <= PROTO_EFFECT_BOUNCE;
}

bool UARTProtocol::isValidBrightness(uint8_t brightness)
{
    return brightness <= 255; // 0-255
}