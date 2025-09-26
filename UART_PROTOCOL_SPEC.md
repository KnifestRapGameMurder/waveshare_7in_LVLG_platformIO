# ESP32 UART Communication Protocol Specification

## Overview
This document defines the communication protocol between ESP32-S3 Touch LCD (Master) and ESP32 DevKit (Peripheral Driver/Slave).

## Connection Details
- **Protocol**: UART Serial Communication
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

### Physical Wiring
```
ESP32-S3 Touch LCD    ←→    ESP32 DevKit
GPIO43 (TX2)          ←→    GPIO16 (RX2)
GPIO44 (RX2)          ←→    GPIO17 (TX2)  
GND                   ←→    GND
```

## Message Format

### Frame Structure
```
[START][CMD][LEN][DATA...][CRC][END]
```

- **START**: `0xAA` (1 byte) - Frame start marker
- **CMD**: Command ID (1 byte) - See command table below
- **LEN**: Data length (1 byte) - Length of DATA field (0-250)
- **DATA**: Command data (0-250 bytes) - Command-specific payload
- **CRC**: Checksum (1 byte) - XOR checksum of CMD+LEN+DATA
- **END**: `0x55` (1 byte) - Frame end marker

### Maximum Frame Size
- 255 bytes total (including headers)
- Maximum data payload: 250 bytes

## Command Categories

### 0x1X - LED Control Commands
| CMD  | Name | Direction | Description |
|------|------|-----------|-------------|
| 0x10 | LED_SET | Master→Slave | Set LED state (on/off/brightness) |
| 0x11 | LED_RGB | Master→Slave | Set RGB LED color |
| 0x12 | LED_PATTERN | Master→Slave | Start LED pattern/animation |
| 0x13 | LED_STATUS | Slave→Master | LED status response |

### 0x2X - Button Input Commands  
| CMD  | Name | Direction | Description |
|------|------|-----------|-------------|
| 0x20 | BTN_STATE | Slave→Master | Button state change notification |
| 0x21 | BTN_CONFIG | Master→Slave | Configure button parameters |
| 0x22 | BTN_STATUS | Master→Slave | Request all button states |

### 0x3X - Sensor Commands
| CMD  | Name | Direction | Description |
|------|------|-----------|-------------|
| 0x30 | SENSOR_READ | Master→Slave | Request sensor reading |
| 0x31 | SENSOR_DATA | Slave→Master | Sensor data response |
| 0x32 | SENSOR_CONFIG | Master→Slave | Configure sensor parameters |
| 0x33 | SENSOR_AUTO | Master→Slave | Enable/disable auto-reporting |

### 0x4X - System Commands
| CMD  | Name | Direction | Description |
|------|------|-----------|-------------|
| 0x40 | PING | Both | Connection test/heartbeat |
| 0x41 | PONG | Both | Ping response |
| 0x42 | RESET | Master→Slave | Reset peripheral controller |
| 0x43 | STATUS | Master→Slave | Request system status |
| 0x44 | ERROR | Both | Error notification |

### 0x5X - Training/Application Commands
| CMD  | Name | Direction | Description |
|------|------|-----------|-------------|
| 0x50 | TRAINING_START | Master→Slave | Start training session |
| 0x51 | TRAINING_STOP | Master→Slave | Stop training session |
| 0x52 | TRAINING_DATA | Both | Training session data |
| 0x53 | TRAINING_STATUS | Slave→Master | Training progress/status |

## Data Formats

### LED_SET (0x10)
```
Data: [LED_ID][STATE][BRIGHTNESS]
- LED_ID: 0-255 (LED identifier)
- STATE: 0=OFF, 1=ON
- BRIGHTNESS: 0-255 (PWM value)
```

### LED_RGB (0x11)
```
Data: [LED_ID][R][G][B]
- LED_ID: 0-255 (RGB LED identifier)  
- R, G, B: 0-255 (RGB color values)
```

### BTN_STATE (0x20)
```
Data: [BTN_ID][STATE][TIMESTAMP_H][TIMESTAMP_L]
- BTN_ID: 0-255 (Button identifier)
- STATE: 0=RELEASED, 1=PRESSED, 2=LONG_PRESS
- TIMESTAMP: 16-bit timestamp (ms since last reset)
```

### SENSOR_DATA (0x31)
```
Data: [SENSOR_ID][TYPE][VALUE...]
- SENSOR_ID: 0-255 (Sensor identifier)
- TYPE: 0=TEMP, 1=HUMIDITY, 2=PRESSURE, 3=LIGHT, 4=DISTANCE
- VALUE: Sensor-specific data format
```

## Error Handling

### Error Codes (0x44 command)
```
Data: [ERROR_CODE][ERROR_DATA...]
- 0x01: Invalid command
- 0x02: Invalid data length
- 0x03: CRC mismatch
- 0x04: Hardware error
- 0x05: Timeout
- 0x06: Resource busy
- 0xFF: Unknown error
```

### Timeout Rules
- Master waits 1000ms for slave response
- Slave responds within 500ms to master commands
- Heartbeat (PING) every 30 seconds during idle
- Auto-retry failed commands up to 3 times

## Implementation Notes

### CRC Calculation
```c
uint8_t calculate_crc(uint8_t cmd, uint8_t len, uint8_t* data) {
    uint8_t crc = cmd ^ len;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}
```

### Example Frame
LED_SET command to turn on LED 1 with brightness 128:
```
[0xAA][0x10][0x03][0x01][0x01][0x80][0x92][0x55]
 START CMD   LEN   LED1  ON   BRIGHT  CRC  END
```

## State Management

### Connection States
- **DISCONNECTED**: No communication established
- **CONNECTING**: Handshake in progress  
- **CONNECTED**: Normal operation
- **ERROR**: Communication error, attempting recovery

### Handshake Sequence
1. Master sends PING (0x40)
2. Slave responds with PONG (0x41) 
3. Master requests STATUS (0x43)
4. Slave sends system information
5. Connection established

## Version
- Protocol Version: 1.0
- Last Updated: September 2025
- Compatible with: ESP32, ESP32-S3, ESP32-C3