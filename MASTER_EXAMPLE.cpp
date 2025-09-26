// Master Board Example - How to use UARTProtocol
#include <Arduino.h>
#include "UARTProtocol.h"

// Create protocol instance
UARTProtocol protocol(&Serial2); // Use Serial2 for slave communication

void setup()
{
    Serial.begin(115200);  // Debug console
    Serial2.begin(115200); // Slave communication

    Serial.println("Master Board Started");

    // Send handshake to slave
    protocol.sendMessage(protocol.createHandshakeMessage("MASTER_BOARD"));
}

void loop()
{
    // Check for incoming messages from slave
    ProtocolMessage message;
    if (protocol.receiveMessage(message))
    {
        handleSlaveMessage(message);
    }

    // Example: Send LED commands to slave
    static unsigned long lastLEDCommand = 0;
    if (millis() - lastLEDCommand > 3000)
    {
        // Send different LED commands every 3 seconds
        static int commandIndex = 0;

        switch (commandIndex % 6)
        {
        case 0:
            // Set all LEDs red
            protocol.sendMessage(protocol.createLEDSetAllMessage(PROTO_COLOR_RED));
            Serial.println("Sent: Set all LEDs RED");
            break;

        case 1:
            // Set LED 0 to blue
            protocol.sendMessage(protocol.createLEDSetPixelMessage(0, PROTO_COLOR_BLUE));
            Serial.println("Sent: Set LED 0 BLUE");
            break;

        case 2:
            // Start rainbow effect
            protocol.sendMessage(protocol.createLEDEffectMessage(PROTO_EFFECT_RAINBOW_WAVE));
            Serial.println("Sent: Start RAINBOW_WAVE effect");
            break;

        case 3:
            // Set brightness to 50%
            protocol.sendMessage(protocol.createLEDBrightnessMessage(128));
            Serial.println("Sent: Set brightness to 50%");
            break;

        case 4:
            // Start chase effect
            protocol.sendMessage(protocol.createLEDEffectMessage(PROTO_EFFECT_CHASE));
            Serial.println("Sent: Start CHASE effect");
            break;

        case 5:
            // Clear all LEDs
            protocol.sendMessage(protocol.createLEDClearMessage());
            Serial.println("Sent: Clear all LEDs");
            break;
        }

        commandIndex++;
        lastLEDCommand = millis();
    }
}

void handleSlaveMessage(const ProtocolMessage &message)
{
    switch (message.type)
    {
    case MSG_HANDSHAKE:
        Serial.println("Slave connected: " + message.data);
        // Send ACK
        protocol.sendMessage(protocol.createAckMessage("HANDSHAKE"));
        break;

    case MSG_BUTTON_PRESSED:
        Serial.println("Button " + String(message.param1) + " pressed");
        // Echo: Light up the pressed button LED
        protocol.sendMessage(protocol.createLEDSetPixelMessage(message.param1, PROTO_COLOR_GREEN));
        break;

    case MSG_BUTTON_RELEASED:
        Serial.println("Button " + String(message.param1) + " released");
        // Turn off the button LED
        protocol.sendMessage(protocol.createLEDSetPixelMessage(message.param1, PROTO_COLOR_BLACK));
        break;

    case MSG_BUTTON_STATE:
        Serial.println("Button state: 0x" + String(message.param3, HEX));
        break;

    case MSG_HALL_DETECTED:
        Serial.println("Hall sensor detected (count: " + String(message.param3) + ")");
        // Flash all LEDs when hall sensor detected
        protocol.sendMessage(protocol.createLEDEffectMessage(PROTO_EFFECT_SPARKLE));
        break;

    case MSG_HALL_REMOVED:
        Serial.println("Hall sensor removed");
        protocol.sendMessage(protocol.createLEDClearMessage());
        break;

    case MSG_STATUS:
        Serial.println("Slave status - Time: " + String(message.param3) +
                       " Buttons: 0x" + String((message.param2 << 8) | message.param1, HEX));
        break;

    case MSG_ERROR:
        Serial.println("Slave error: " + message.data);
        break;

    default:
        Serial.println("Unknown message from slave");
        break;
    }
}

// Additional utility functions for master
void sendColorToAllLEDs(ProtocolColor color)
{
    protocol.sendMessage(protocol.createLEDSetAllMessage(color));
}

void sendEffectToSlave(ProtocolEffect effect)
{
    protocol.sendMessage(protocol.createLEDEffectMessage(effect));
}

void setBrightness(uint8_t brightness)
{
    protocol.sendMessage(protocol.createLEDBrightnessMessage(brightness));
}

void lightUpButton(uint8_t buttonIndex, ProtocolColor color)
{
    if (protocol.isValidButtonIndex(buttonIndex))
    {
        protocol.sendMessage(protocol.createLEDSetPixelMessage(buttonIndex, color));
    }
}