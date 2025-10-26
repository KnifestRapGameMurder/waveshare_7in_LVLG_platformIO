#include "uart_protocol.h"

UARTProtocol::UARTProtocol(HardwareSerial *serialPort)
{
    serial = serialPort;
    inputBuffer = "";
}

String UARTProtocol::createUseSerialMessage(uint8_t serialIndex)
{
    return String(MSG_TYPE_CMD) + MSG_SEPARATOR + CMD_USE_SERIAL + MSG_SEPARATOR + String(serialIndex);
}

String UARTProtocol::createButtonStateMessage(uint16_t buttonState)
{
    return String(MSG_TYPE_CMD) + MSG_SEPARATOR + CMD_BUTTONS + MSG_SEPARATOR + uint16ToBinaryString(buttonState);
}

String UARTProtocol::createLEDSetPixelMessage(uint8_t ledIndex, const String &hexColor)
{
    return String(MSG_TYPE_CMD) + MSG_SEPARATOR + CMD_LED + MSG_SEPARATOR + String(ledIndex) + MSG_SEPARATOR + hexColor;
}

String UARTProtocol::createLEDSetPixelsMultiMessage(const String &colorList)
{
    return String(MSG_TYPE_CMD) + MSG_SEPARATOR + CMD_LEDS + MSG_SEPARATOR + colorList;
}

String UARTProtocol::createLEDClearMessage(const String &hexColor)
{
    return String(MSG_TYPE_CMD) + MSG_SEPARATOR + CMD_LEDS + MSG_SEPARATOR + CMD_LED_CLEAR + MSG_SEPARATOR + hexColor;
}

String UARTProtocol::createRequestButtonsMessage()
{
    return String(MSG_TYPE_CMD) + MSG_SEPARATOR + CMD_REQUEST + MSG_SEPARATOR + CMD_BUTTONS;
}

String UARTProtocol::createLogMessage(const String &logData)
{
    return String(MSG_TYPE_LOG) + MSG_SEPARATOR + logData;
}

void UARTProtocol::sendMessage(const String &message)
{
    if (message.length() > MAX_MESSAGE_LENGTH)
    {
        Serial.println("Error: Message too long");
        return;
    }
    // serial->println(message);
    serial->print(message + MSG_DELIMITER);
    // Serial.print(message + MSG_DELIMITER);
}

bool UARTProtocol::receiveMessage(String &message)
{
    while (serial->available())
    {
        char c = serial->read();
        if (c == MSG_DELIMITER)
        {
            if (inputBuffer.length() > 0)
            {
                message = inputBuffer;
                inputBuffer = "";
                return true;
            }
        }
        else if (inputBuffer.length() < MAX_MESSAGE_LENGTH)
        {
            inputBuffer += c;
        }
    }
    // while (Serial.available())
    // {
    //     char c = Serial.read();
    //     if (c == MSG_DELIMITER)
    //     {
    //         if (inputBuffer.length() > 0)
    //         {
    //             message = inputBuffer;
    //             inputBuffer = "";
    //             return true;
    //         }
    //     }
    //     else if (inputBuffer.length() < MAX_MESSAGE_LENGTH)
    //     {
    //         inputBuffer += c;
    //     }
    // }
    return false;
}

bool UARTProtocol::isCMDMessage(const String &message)
{
    return message.startsWith(MSG_TYPE_CMD);
}

bool UARTProtocol::isLOGMessage(const String &message)
{
    return message.startsWith(MSG_TYPE_LOG);
}

bool UARTProtocol::parseCMDMessage(const String &data, String &command, String &param1, String &param2)
{
    String cleanedData = data;
    if (cleanedData.startsWith(MSG_TYPE_CMD))
    {
        cleanedData = cleanedData.substring(String(MSG_TYPE_CMD).length() + 1);
    }

    int firstColon = cleanedData.indexOf(MSG_SEPARATOR);
    if (firstColon == -1)
        return false;
    command = cleanedData.substring(0, firstColon);

    String rest = cleanedData.substring(firstColon + 1);
    int secondColon = rest.indexOf(MSG_SEPARATOR);
    if (secondColon == -1)
    {
        param1 = rest;
        param1.trim(); // Remove any trailing newline or whitespace
        param2 = "";
    }
    else
    {
        param1 = rest.substring(0, secondColon);
        param2 = rest.substring(secondColon + 1);
        param2.trim(); // Remove any trailing newline or whitespace
    }
    return true;
}

void UARTProtocol::handleLogMessage(const String &data)
{
    Serial.println("LOG: " + data);
}

String UARTProtocol::uint16ToBinaryString(uint16_t value)
{
    String binary = "";
    // Make string position directly map to button number
    // button 0 = binary[0], button 1 = binary[1], etc.
    for (int i = 0; i < 16; i++)
    {
        binary += (value & (1 << i)) ? '1' : '0';
    }
    return binary;
}

uint16_t UARTProtocol::binaryStringToUint16(const String &binaryStr)
{
    uint16_t value = 0;
    // Make string position directly map to button number
    // binaryStr[0] = button 0, binaryStr[1] = button 1, etc.
    for (int i = 0; i < 16 && i < binaryStr.length(); i++)
    {
        if (binaryStr[i] == '1')
        {
            value |= (1 << i);
        }
    }
    return value;
}
