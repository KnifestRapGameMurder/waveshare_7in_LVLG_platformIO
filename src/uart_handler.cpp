#include "uart_handler.h"
#include "console.h"
#include "app_config.h"
#include "ui_screens.h"
#include "uart_protocol.h"
#include <Arduino.h>

// UART communication setup
HardwareSerial peripheralUART(2);
UARTProtocol *uart_protocol = nullptr;

void uart_handler_init()
{
    Serial.println("\n=== UART PROTOCOL INITIALIZATION ===");

    uart_protocol = new UARTProtocol(&peripheralUART);

    if (uart_protocol->begin(UART_BAUD_RATE))
    {
        uart_protocol->set_response_callback(uart_response_callback);
        uart_protocol->set_error_callback(uart_error_callback);

        Serial.println("UART protocol initialized on GPIO43/44");
        Serial.println("Attempting connection to peripheral driver...");

        if (uart_protocol->connect(5000))
        {
            Serial.println("✅ Successfully connected to peripheral driver!");

            // Test peripheral connection
            uart_protocol->ping();
            uart_protocol->get_status();

            // Enable automatic sensor reporting
            uart_protocol->sensor_auto_enable(0, 5000);  // Temperature sensor
            uart_protocol->sensor_auto_enable(1, 10000); // Light sensor
        }
        else
        {
            Serial.println("⚠️ Failed to connect to peripheral driver");
            Serial.println("Application will continue without external peripherals");
        }
    }
    else
    {
        Serial.println("❌ UART protocol initialization error");
    }
    Serial.println("=== UART PROTOCOL READY ===\n");
}

void uart_handler_process()
{
    // Handle simple text-based UART from slave
    handle_slave_uart_data();

    // Update UART communication (binary protocol)
    if (uart_protocol)
    {
        uart_protocol->update();
    }

    // Debug: Show UART activity in serial monitor
    static uint32_t last_debug_time = 0;
    uint32_t now = millis();
    if (now - last_debug_time > DEBUG_INTERVAL)
    {
        last_debug_time = now;
        Serial.printf("[DEBUG] UART available bytes: %d\n", peripheralUART.available());
        if (current_state == STATE_CONSOLE)
        {
            Serial.println("[DEBUG] Console window is active");
        }
    }
}

void handle_slave_uart_data()
{
    if (peripheralUART.available())
    {
        String receivedData = peripheralUART.readStringUntil('\n');
        receivedData.trim();

        if (receivedData.length() > 0)
        {
            Serial.printf("📡 Raw UART Data: %s\n", receivedData.c_str());

            // Log ALL received data to console
            char log_buffer[256];
            snprintf(log_buffer, sizeof(log_buffer), "RX: %s", receivedData.c_str());
            console_add_log(log_buffer);

            // Parse specific message formats
            if (receivedData.startsWith("SENSOR_DATA"))
            {
                // Parse: SENSOR_DATA,HALL,EVENT,COUNT,TIMESTAMP,STATE
                console_add_log(">>> Sensor data received");
            }
            else if (receivedData.startsWith("STATUS_UPDATE"))
            {
                console_add_log(">>> Status updated");
            }
            else if (receivedData == "ESP32_SLAVE_READY")
            {
                console_add_log(">>> Peripheral device ready!");
            }
            else if (receivedData == "PONG")
            {
                console_add_log(">>> Peripheral replied to PING");
            }
            else if (receivedData.startsWith("ESP32_SLAVE_MSG"))
            {
                console_add_log(">>> Slave message received");
            }
        }
    }
}

void uart_response_callback(uint8_t cmd, uint8_t *data, uint8_t len)
{
    Serial.printf("[UART CALLBACK] Response received: CMD=0x%02X, LEN=%d\n", cmd, len);
    char log_buffer[256];

    switch (cmd)
    {
    case CMD_BTN_STATE:
        if (len >= 2)
        {
            uint8_t btn_id = data[0];
            uint8_t btn_state = data[1];
            snprintf(log_buffer, sizeof(log_buffer), "Button %d: %s",
                     btn_id, btn_state == 1 ? "PRESSED" : "RELEASED");
            console_add_log(log_buffer);
        }
        break;

    case CMD_SENSOR_DATA:
        if (len >= 3)
        {
            uint8_t sensor_id = data[0];
            uint8_t sensor_type = data[1];

            switch (sensor_type)
            {
            case 0: // Temperature
                if (len >= 5)
                {
                    int16_t temp = (data[2] << 8) | data[3];
                    snprintf(log_buffer, sizeof(log_buffer), "Temperature: %d.%d°C", temp / 10, temp % 10);
                    console_add_log(log_buffer);
                }
                break;
            case 1: // Humidity
                if (len >= 4)
                {
                    uint8_t humidity = data[2];
                    snprintf(log_buffer, sizeof(log_buffer), "Humidity: %d%%", humidity);
                    console_add_log(log_buffer);
                }
                break;
            case 3: // Light
                if (len >= 5)
                {
                    uint16_t light = (data[2] << 8) | data[3];
                    snprintf(log_buffer, sizeof(log_buffer), "Light: %d lux", light);
                    console_add_log(log_buffer);
                }
                break;
            case 4: // Hall sensor
                if (len >= 4)
                {
                    uint8_t hall_state = data[2];
                    snprintf(log_buffer, sizeof(log_buffer), "Magnet: %s", hall_state ? "DETECTED" : "NOT DETECTED");
                    console_add_log(log_buffer);
                }
                break;
            }
        }
        break;

    case CMD_TRAINING_STATUS:
        if (len >= 2)
        {
            uint8_t training_id = data[0];
            uint8_t progress = data[1];
            snprintf(log_buffer, sizeof(log_buffer), "Trainer %d: %d%%", training_id + 1, progress);
            console_add_log(log_buffer);
        }
        break;

    case CMD_PONG:
        console_add_log("Connection active (PONG)");
        break;

    default:
        snprintf(log_buffer, sizeof(log_buffer), "Unknown command: 0x%02X", cmd);
        console_add_log(log_buffer);
        break;
    }
}

void uart_error_callback(uint8_t error_code, const char *message)
{
    Serial.printf("[UART ERROR] Code: 0x%02X, Message: %s\n", error_code, message);

    char log_buffer[256];
    snprintf(log_buffer, sizeof(log_buffer), "UART Error: %s (0x%02X)", message, error_code);
    console_add_log(log_buffer);
}