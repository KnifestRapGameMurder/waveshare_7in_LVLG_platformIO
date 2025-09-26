# ESP32-S3 Touch LCD with UART Peripheral Communication

## Project Overview

This project implements a training application on ESP32-S3 Touch LCD (Waveshare 7" RGB) with UART communication to a separate ESP32 DevKit that controls peripheral devices (LEDs, buttons, sensors).

### System Architecture
```
┌─────────────────────┐    UART     ┌─────────────────────┐
│ ESP32-S3 Touch LCD  │◄──────────►│  ESP32 DevKit       │
│ (Master/Controller) │             │  (Peripheral Driver)│
│                     │             │                     │
│ - Training UI       │             │ - LED Control       │
│ - Touch Interface   │             │ - Button Input      │
│ - Display Control   │             │ - Sensor Reading    │
│ - State Management  │             │ - Audio Output      │
└─────────────────────┘             └─────────────────────┘
```

## Hardware Setup

### ESP32-S3 Touch LCD (This Device)
- **Display**: Waveshare 7" RGB LCD (800x480)
- **Touch**: Capacitive touch controller
- **MCU**: ESP32-S3 with 8MB Flash
- **UART Pins**: GPIO43 (TX2), GPIO44 (RX2)

### ESP32 DevKit (Peripheral Driver)
- **MCU**: ESP32 DevKit v1 or similar
- **UART Pins**: GPIO16 (RX2), GPIO17 (TX2)
- **Peripherals**: LEDs, buttons, sensors, buzzer

### Physical Connections
```
ESP32-S3 Touch LCD    ←→    ESP32 DevKit
GPIO43 (TX2)          ←→    GPIO16 (RX2)
GPIO44 (RX2)          ←→    GPIO17 (TX2)  
GND                   ←→    GND
```

## Software Features

### ESP32-S3 Touch LCD Features
- ✅ Anti-tearing RGB LCD with double-buffer
- ✅ LVGL 8.4.0 GUI with custom Ukrainian fonts
- ✅ State-based training application
- ✅ Touch-based navigation
- ✅ Orbital gradient animation on loading screen
- ✅ Full-screen training menu (2x2 button grid)
- ✅ UART protocol for peripheral communication
- ✅ Ukrainian localization
- ✅ Optimized upload speed (921600 baud)

### UART Communication Protocol
- **Baud Rate**: 115200
- **Frame Format**: `[START][CMD][LEN][DATA][CRC][END]`
- **Command Categories**: LED, Button, Sensor, System, Training
- **Error Handling**: CRC validation, timeouts, retries
- **Heartbeat**: Automatic connection monitoring

## File Structure

```
├── src/
│   ├── main.cpp                    # Main application with training UI
│   ├── minecraft_ten_48.c          # 48px Ukrainian font
│   └── minecraft_ten_96.c          # 96px Ukrainian font
├── lib/
│   ├── uart_protocol/
│   │   ├── uart_protocol.h         # UART protocol header
│   │   └── uart_protocol.cpp       # UART implementation
│   └── lvgl_port/                  # LVGL porting layer
├── include/
│   ├── lv_conf.h                   # LVGL configuration
│   └── esp_*.h                     # ESP32 Display Panel configs
├── examples/
│   └── uart_example.cpp            # UART usage example
├── UART_PROTOCOL_SPEC.md           # Complete protocol specification
├── ESP32_PERIPHERAL_DRIVER_GUIDE.md # Guide for peripheral driver
└── platformio.ini                  # PlatformIO configuration
```

## Quick Start

1. **Build and Upload**:
   ```bash
   pio run --target upload
   ```

2. **Monitor Serial Output**:
   ```bash
   pio device monitor --port COM5 --baud 115200
   ```

3. **Connect Peripheral Driver**:
   - Set up ESP32 DevKit with peripheral driver project
   - Connect UART pins as specified above
   - Power on both devices

## UART Protocol Usage

### Initialize Communication
```cpp
#include "uart_protocol.h"

HardwareSerial peripheralUART(2);
UARTProtocol uart_protocol(&peripheralUART);

// Set up callbacks
uart_protocol.set_response_callback(on_response);
uart_protocol.set_error_callback(on_error);

// Initialize and connect
uart_protocol.begin(115200);
uart_protocol.connect(5000);
```

### Control LEDs
```cpp
// Basic LED control
uart_protocol.led_set(0, true, 255);     // LED 0 ON, full brightness
uart_protocol.led_set(1, false, 0);      // LED 1 OFF

// RGB LED control
uart_protocol.led_rgb(0, 255, 0, 0);     // Red
uart_protocol.led_rgb(0, 0, 255, 0);     // Green

// LED patterns
uart_protocol.led_pattern(0, 1, 100);    // Pattern 1, speed 100
```

### Read Sensors
```cpp
// Manual sensor reading
uart_protocol.sensor_read(0);            // Temperature sensor
uart_protocol.sensor_read(1);            // Light sensor

// Auto-reporting (sensor sends data automatically)
uart_protocol.sensor_auto_enable(0, 5000); // Every 5 seconds
```

### Training Control
```cpp
// Start/stop training sessions  
uart_protocol.training_start(0);         // Start trainer 1
uart_protocol.training_stop();           // Stop current training
```

## Application States

1. **STATE_LOADING**: Orbital animation with "НЕЙРО БЛОК" title
   - Touch screen to proceed to main menu
   - Automatic timeout after idle period

2. **STATE_MAIN_MENU**: 2x2 grid of training buttons
   - Touch buttons or use hardware buttons via UART
   - Each button represents a different trainer

3. **STATE_TRAINER_X**: Individual training screens
   - Specific training content (customizable)
   - Back button to return to main menu
   - LED feedback via UART

## Peripheral Driver Development

For the ESP32 DevKit peripheral driver project:

1. **Read the Guide**: `ESP32_PERIPHERAL_DRIVER_GUIDE.md`
2. **Follow Protocol**: `UART_PROTOCOL_SPEC.md` 
3. **Implement Controllers**: LED, Button, Sensor managers
4. **Test Communication**: Use provided examples

## Customization

### Add New Training Modules
1. Extend `AppState` enum with new states
2. Add buttons to main menu grid
3. Implement `create_trainer_screen()` for new content
4. Add corresponding UART commands for peripherals

### Modify UI Fonts/Colors
- Font files: `src/minecraft_ten_*.c`
- Colors: Defined in button creation functions
- Layout: Modify screen creation functions

### Extend UART Protocol
1. Add new command IDs in `uart_protocol.h`
2. Implement handlers in `uart_protocol.cpp`
3. Update protocol specification document
4. Coordinate with peripheral driver project

## Performance Optimizations

- **Upload Speed**: 921600 baud for faster development
- **Anti-tearing**: RGB double-buffer eliminates display artifacts  
- **Font Optimization**: Custom fonts with only required glyphs
- **UART Efficiency**: Binary protocol with CRC validation
- **Memory Management**: Careful buffer management for large displays

## Troubleshooting

### Display Issues
- Verify RGB double-buffer configuration
- Check bounce buffer settings for ESP32-S3
- Ensure proper pin assignments

### UART Communication
- Verify physical wiring (TX↔RX, GND)
- Check baud rate settings (115200)
- Monitor serial output for error messages
- Test with simple ping/pong commands

### Touch Not Working
- Multiple event types registered (CLICKED, PRESSED, PRESS_LOST)
- Touch events on both screen and gradient objects
- Ensure LVGL touch driver is properly initialized

## Development Tools

- **PlatformIO**: Build system and dependency management
- **ESP32 Arduino Core 3.0.2**: Latest Arduino framework
- **LVGL 8.4.0**: GUI library with anti-tearing support
- **ESP32_Display_Panel**: Hardware abstraction layer

## License

This project uses components from Espressif Systems under various open-source licenses. Custom code is provided for educational and development purposes.

---

**Last Updated**: September 2025  
**Compatible Hardware**: ESP32-S3, Waveshare 7" RGB Touch LCD  
**Framework**: Arduino/ESP-IDF via PlatformIO