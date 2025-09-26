# UART Protocol Documentation

## Message Format
All messages follow the format: `MESSAGE_TYPE:param1:param2:...`
Messages end with newline character (`\n`)

## Message Types

### System Messages

#### HANDSHAKE
- **From Master**: `HANDSHAKE:MASTER_BOARD`
- **From Slave**: `HANDSHAKE:SLAVE:16_BUTTONS_16_LEDS`
- **Purpose**: Initial connection establishment

#### STATUS
- **From Slave**: `STATUS:12345:ABCD`
- **Purpose**: Periodic status updates (timestamp:button_state_hex)

#### ACK
- **Both directions**: `ACK:original_message`
- **Purpose**: Acknowledge received message

#### ERROR
- **Both directions**: `ERROR:error_description`
- **Purpose**: Report errors

### Button Events (Slave → Master)

#### BUTTON_PRESSED
- **Format**: `BUTTON_PRESSED:5`
- **Purpose**: Button index 5 was pressed

#### BUTTON_RELEASED
- **Format**: `BUTTON_RELEASED:5`
- **Purpose**: Button index 5 was released

#### BUTTON_STATE
- **Format**: `BUTTON_STATE:ABCD`
- **Purpose**: All 16 buttons state as hex (bit 0 = button 0, etc.)

### LED Control (Master → Slave)

#### LED_SET_PIXEL
- **Format**: `LED_SET_PIXEL:5:RED`
- **Purpose**: Set LED index 5 to red color

#### LED_SET_ALL
- **Format**: `LED_SET_ALL:BLUE`
- **Purpose**: Set all LEDs to blue color

#### LED_CLEAR
- **Format**: `LED_CLEAR`
- **Purpose**: Turn off all LEDs

#### LED_EFFECT
- **Format**: `LED_EFFECT:RAINBOW_WAVE`
- **Purpose**: Start rainbow wave effect

#### LED_BRIGHTNESS
- **Format**: `LED_BRIGHTNESS:128`
- **Purpose**: Set brightness to 50% (0-255 range)

### Hall Sensor Events (Slave → Master)

#### HALL_DETECTED
- **Format**: `HALL_DETECTED:42`
- **Purpose**: Hall sensor detected (with detection count)

#### HALL_REMOVED
- **Format**: `HALL_REMOVED`
- **Purpose**: Hall sensor no longer detected

## Available Colors
- BLACK, RED, GREEN, BLUE, WHITE
- YELLOW, CYAN, MAGENTA, ORANGE

## Available Effects
- OFF, SOLID, RAINBOW_WAVE, COLOR_CYCLE
- BREATHING, SPARKLE, CHASE, BOUNCE

## Usage Examples

### Master sending LED commands:
```cpp
// Set LED 0 to red
protocol.sendMessage(protocol.createLEDSetPixelMessage(0, PROTO_COLOR_RED));

// Start rainbow effect
protocol.sendMessage(protocol.createLEDEffectMessage(PROTO_EFFECT_RAINBOW_WAVE));

// Set brightness to 25%
protocol.sendMessage(protocol.createLEDBrightnessMessage(64));
```

### Master handling button events:
```cpp
if (message.type == MSG_BUTTON_PRESSED) {
    uint8_t buttonIndex = message.param1;
    Serial.println("Button " + String(buttonIndex) + " pressed");
    
    // Light up the pressed button
    protocol.sendMessage(protocol.createLEDSetPixelMessage(buttonIndex, PROTO_COLOR_GREEN));
}
```

### Slave sending button events:
```cpp
// In button press handler
protocol.sendMessage(protocol.createButtonPressedMessage(buttonIndex));

// In button release handler  
protocol.sendMessage(protocol.createButtonReleasedMessage(buttonIndex));
```

## Integration Steps

1. **Copy files to master project:**
   - `include/UARTProtocol.h`
   - `src/UARTProtocol.cpp`

2. **Initialize in master:**
   ```cpp
   UARTProtocol protocol(&Serial2); // Use appropriate serial port
   ```

3. **Send commands to slave:**
   ```cpp
   protocol.sendMessage(protocol.createLEDSetAllMessage(PROTO_COLOR_RED));
   ```

4. **Handle slave messages:**
   ```cpp
   ProtocolMessage message;
   if (protocol.receiveMessage(message)) {
       // Handle message based on message.type
   }
   ```

## Benefits
- **Standardized**: Same protocol works across different projects
- **Extensible**: Easy to add new message types
- **Validated**: Built-in parameter validation
- **Portable**: Self-contained, just copy the files
- **Human-readable**: Messages are text-based for easy debugging