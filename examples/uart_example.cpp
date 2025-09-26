/*
 * ESP32-S3 Touch LCD UART Communication Example
 *
 * This example demonstrates how to use the UART protocol to communicate
 * with an ESP32 DevKit that acts as a peripheral driver.
 *
 * Hardware Setup:
 * - ESP32-S3 GPIO43 (TX2) → ESP32 DevKit GPIO16 (RX2)
 * - ESP32-S3 GPIO44 (RX2) ← ESP32 DevKit GPIO17 (TX2)
 * - GND connections between both devices
 */

#include <Arduino.h>
#include "uart_protocol.h"

// Create UART instance
HardwareSerial peripheralUART(2);
UARTProtocol uart_protocol(&peripheralUART);

// Response callback - called when peripheral sends data
void on_peripheral_response(uint8_t cmd, uint8_t *data, uint8_t len)
{
    Serial.printf("Received response: CMD=0x%02X, LEN=%d\n", cmd, len);

    switch (cmd)
    {
    case CMD_BTN_STATE:
        if (len >= 2)
        {
            uint8_t btn_id = data[0];
            uint8_t state = data[1];
            Serial.printf("Button %d: %s\n", btn_id, state ? "PRESSED" : "RELEASED");
        }
        break;

    case CMD_SENSOR_DATA:
        if (len >= 3)
        {
            uint8_t sensor_id = data[0];
            uint8_t sensor_type = data[1];
            Serial.printf("Sensor %d (type %d) data received\n", sensor_id, sensor_type);
        }
        break;
    }
}

// Error callback - called on communication errors
void on_uart_error(uint8_t error_code, const char *message)
{
    Serial.printf("UART Error: %s (code: 0x%02X)\n", message, error_code);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("ESP32-S3 UART Communication Example");

    // Initialize UART protocol
    if (uart_protocol.begin(115200))
    {
        uart_protocol.set_response_callback(on_peripheral_response);
        uart_protocol.set_error_callback(on_uart_error);

        Serial.println("UART protocol initialized");

        // Attempt to connect to peripheral
        if (uart_protocol.connect(5000))
        {
            Serial.println("Connected to peripheral!");

            // Test commands
            uart_protocol.ping();
            uart_protocol.get_status();

            // Control LEDs
            uart_protocol.led_set(0, true, 255); // Turn on LED 0 at full brightness
            uart_protocol.led_rgb(0, 255, 0, 0); // Set RGB LED to red

            // Enable sensor auto-reporting
            uart_protocol.sensor_auto_enable(0, 2000); // Temperature every 2 seconds
        }
        else
        {
            Serial.println("Failed to connect to peripheral");
        }
    }
}

void loop()
{
    // Update UART communication (call this regularly)
    uart_protocol.update();

    // Example: Control LEDs based on time
    static uint32_t last_led_update = 0;
    static bool led_state = false;

    if (millis() - last_led_update > 2000)
    {
        if (uart_protocol.is_connected())
        {
            // Toggle LED every 2 seconds
            led_state = !led_state;
            uart_protocol.led_set(1, led_state, 128);

            // Cycle RGB colors
            static uint8_t color_index = 0;
            uint8_t colors[][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}};
            uart_protocol.led_rgb(0, colors[color_index][0], colors[color_index][1], colors[color_index][2]);
            color_index = (color_index + 1) % 4;
        }
        last_led_update = millis();
    }

    // Example: Read sensors periodically
    static uint32_t last_sensor_read = 0;
    if (millis() - last_sensor_read > 5000)
    {
        if (uart_protocol.is_connected())
        {
            uart_protocol.sensor_read(0); // Request temperature
            uart_protocol.sensor_read(1); // Request light level
        }
        last_sensor_read = millis();
    }

    delay(10);
}