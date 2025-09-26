# ESP32 DevKit Peripheral Driver Project Setup Guide

## Project Overview
You are tasked with creating an ESP32 DevKit project that acts as a **Peripheral Driver/Controller** for various hardware components (LEDs, buttons, sensors). This ESP32 will communicate with an ESP32-S3 Touch LCD via UART protocol.

## Your Role
- **Device Type**: ESP32 DevKit (Slave/Driver)
- **Primary Function**: Control peripherals and respond to commands from Master
- **Communication**: UART Serial with ESP32-S3 Touch LCD (Master)
- **Protocol**: See `UART_PROTOCOL_SPEC.md` for complete specification

## Hardware Setup Requirements

### ESP32 DevKit Connections
```
ESP32 DevKit Pins:
- GPIO16 (RX2) ← Connect to ESP32-S3 TX2 (GPIO43)
- GPIO17 (TX2) → Connect to ESP32-S3 RX2 (GPIO44)  
- GND        ← Connect to ESP32-S3 GND

Peripheral Connections (customize as needed):
- GPIO2  → Built-in LED
- GPIO18 → External LED Strip (WS2812B/NeoPixel)
- GPIO19 → Button 1 (with pullup)
- GPIO21 → Button 2 (with pullup)
- GPIO22 → DHT22 Temperature/Humidity Sensor
- GPIO23 → Ultrasonic Distance Sensor (Trigger)
- GPIO25 → Ultrasonic Distance Sensor (Echo)
- GPIO26 → Light Sensor (Analog)
- GPIO27 → Buzzer/Speaker
```

## Project Architecture

### Core Components to Implement

1. **UART Communication Handler**
   - Parse incoming messages from Master
   - Send responses and notifications
   - Handle protocol framing and CRC

2. **Peripheral Controllers**
   - LED Controller (basic LEDs + RGB strips)
   - Button Manager (with debouncing)
   - Sensor Manager (temperature, light, distance)
   - Audio Controller (buzzer/sounds)

3. **Command Processor**
   - Route commands to appropriate handlers
   - Validate command parameters
   - Generate responses

4. **Event System**
   - Asynchronous button press notifications
   - Sensor threshold alerts
   - Auto-reporting capabilities

## Implementation Requirements

### 1. UART Communication Class
Create a robust UART handler that:
- Implements the protocol from `UART_PROTOCOL_SPEC.md`
- Handles frame parsing with START/END markers
- Validates CRC checksums
- Manages timeouts and error recovery
- Provides async callback system for received commands

### 2. Peripheral Manager Classes
Implement modular controllers for:

**LED Controller:**
- Support for basic on/off/brightness control
- RGB LED color management
- LED patterns and animations
- FastLED or NeoPixel library integration

**Button Manager:**
- Hardware debouncing
- Press/release/long-press detection
- Configurable timing parameters
- Interrupt-based or polling modes

**Sensor Manager:**
- Temperature/humidity reading (DHT22)
- Light level sensing (analog)
- Distance measurement (ultrasonic)
- Configurable auto-reporting intervals

### 3. Training Integration
Since this connects to a training application:
- Implement training session state management
- Provide real-time feedback mechanisms
- Support for training progress indicators
- Customizable training scenarios

## Development Steps

### Phase 1: Basic UART Communication
1. Set up PlatformIO project for ESP32 DevKit
2. Implement basic UART protocol handler
3. Test PING/PONG communication with Master
4. Verify CRC and framing

### Phase 2: Core Peripherals
1. Implement LED control (basic and RGB)
2. Add button input handling
3. Integrate basic sensors
4. Test individual peripheral commands

### Phase 3: Advanced Features
1. Add LED patterns and animations
2. Implement sensor auto-reporting
3. Add training session management
4. Optimize performance and reliability

### Phase 4: Integration & Testing
1. Full protocol compliance testing
2. Error handling and recovery
3. Performance optimization
4. Documentation and examples

## Code Structure Recommendations

```
src/
├── main.cpp                 // Main setup and loop
├── uart_protocol.h/cpp      // UART communication handler
├── command_processor.h/cpp  // Command routing and processing
├── peripherals/
│   ├── led_controller.h/cpp      // LED management
│   ├── button_manager.h/cpp      // Button input handling
│   ├── sensor_manager.h/cpp      // Sensor reading
│   └── audio_controller.h/cpp    // Sound/buzzer control
├── training/
│   ├── training_manager.h/cpp    // Training session logic
│   └── feedback_system.h/cpp     // User feedback mechanisms
└── utils/
    ├── crc.h/cpp            // CRC calculation utilities
    └── timer.h/cpp          // Timing and timeout utilities
```

## Key Libraries to Consider

- **HardwareSerial**: For UART communication
- **FastLED**: For addressable LED strips
- **DHT sensor library**: For temperature/humidity
- **NewPing**: For ultrasonic distance sensors
- **ArduinoJson**: For complex data serialization (if needed)
- **TaskScheduler**: For managing multiple tasks

## Testing Strategy

### Unit Testing
- Test each peripheral controller independently
- Validate UART protocol parsing
- Verify CRC calculations
- Test error handling scenarios

### Integration Testing
- Full protocol compliance with Master device
- Stress test with rapid command sequences
- Timeout and recovery testing
- Multi-peripheral simultaneous operation

## Configuration System

Implement a flexible configuration system:
- Pin mappings for different hardware setups
- Sensor calibration parameters
- Communication timeouts and retry counts
- Training scenario parameters

## Error Handling Requirements

- Robust error detection and reporting
- Graceful degradation when peripherals fail
- Communication timeout recovery
- Invalid command handling
- Hardware fault detection

## Performance Considerations

- Non-blocking peripheral operations
- Efficient UART buffer management
- Minimize latency for real-time feedback
- Optimize memory usage for complex commands
- Consider FreeRTOS tasks for parallel processing

## Documentation Requirements

Create comprehensive documentation:
- API reference for all peripheral functions
- Configuration examples
- Troubleshooting guide
- Hardware setup diagrams
- Protocol implementation notes

## Success Criteria

Your implementation should:
1. ✅ Fully comply with the UART protocol specification
2. ✅ Handle all defined command categories (LED, Button, Sensor, System, Training)
3. ✅ Provide reliable real-time peripheral control
4. ✅ Support the training application requirements
5. ✅ Include comprehensive error handling
6. ✅ Be easily configurable for different hardware setups
7. ✅ Maintain stable communication under load

## Communication with Master Project

The ESP32-S3 Touch LCD project (Master) will:
- Send commands for LED control during training
- Request sensor readings for environmental monitoring
- Receive button press notifications for user interaction
- Coordinate training session start/stop
- Monitor system health via heartbeat

Your peripheral driver should seamlessly integrate with these requirements while maintaining modularity and reliability.

---

**Protocol Specification**: Refer to `UART_PROTOCOL_SPEC.md` for complete technical details.

**Contact**: Share this guide and the protocol specification with the Master project for coordination.