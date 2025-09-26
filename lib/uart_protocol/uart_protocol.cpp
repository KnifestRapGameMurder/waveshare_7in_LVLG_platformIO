#include "uart_protocol.h"

UARTProtocol::UARTProtocol(HardwareSerial *serial)
{
    uart = serial;
    state = DISCONNECTED;
    response_callback = nullptr;
    error_callback = nullptr;
    last_heartbeat = 0;
    last_command_time = 0;
    rx_index = 0;
    frame_in_progress = false;
}

bool UARTProtocol::begin(uint32_t baud_rate)
{
    uart->begin(baud_rate, SERIAL_8N1, 44, 43); // RX=GPIO44, TX=GPIO43 for ESP32-S3
    uart->setTimeout(100);

    reset_rx_buffer();
    state = DISCONNECTED;

    Serial.println("[UART] Protocol initialized");
    return true;
}

void UARTProtocol::set_response_callback(ResponseCallback callback)
{
    response_callback = callback;
}

void UARTProtocol::set_error_callback(ErrorCallback callback)
{
    error_callback = callback;
}

uint8_t UARTProtocol::calculate_crc(uint8_t cmd, uint8_t len, uint8_t *data)
{
    uint8_t crc = cmd ^ len;
    for (int i = 0; i < len; i++)
    {
        crc ^= data[i];
    }
    return crc;
}

bool UARTProtocol::send_command(uint8_t cmd, uint8_t *data, uint8_t len)
{
    if (len > UART_MAX_DATA_LEN)
    {
        send_error(ERR_INVALID_LEN, "Data too long");
        return false;
    }

    // Build frame
    UARTFrame frame;
    frame.start = UART_START_MARKER;
    frame.cmd = cmd;
    frame.len = len;

    if (data && len > 0)
    {
        memcpy(frame.data, data, len);
    }

    frame.crc = calculate_crc(cmd, len, frame.data);
    frame.end = UART_END_MARKER;

    // Send frame
    uart->write(frame.start);
    uart->write(frame.cmd);
    uart->write(frame.len);

    if (len > 0)
    {
        uart->write(frame.data, len);
    }

    uart->write(frame.crc);
    uart->write(frame.end);

    last_command_time = millis();

    Serial.printf("[UART] Sent command 0x%02X, len=%d\n", cmd, len);
    return true;
}

bool UARTProtocol::send_command_with_response(uint8_t cmd, uint8_t *data, uint8_t len,
                                              uint8_t *response_data, uint8_t *response_len,
                                              uint32_t timeout_ms)
{
    if (!send_command(cmd, data, len))
    {
        return false;
    }

    // Wait for response
    uint32_t start_time = millis();
    bool response_received = false;

    while (millis() - start_time < timeout_ms && !response_received)
    {
        update();

        // Check if we received a response
        // This is a simplified implementation - in practice you'd want to match command IDs
        if (rx_index > 0)
        {
            response_received = true;
        }

        delay(1);
    }

    if (!response_received)
    {
        send_error(ERR_TIMEOUT, "Command timeout");
        return false;
    }

    return true;
}

void UARTProtocol::update()
{
    // Read incoming bytes
    while (uart->available())
    {
        uint8_t byte = uart->read();

        if (!frame_in_progress)
        {
            // Look for start marker
            if (byte == UART_START_MARKER)
            {
                reset_rx_buffer();
                rx_buffer[rx_index++] = byte;
                frame_in_progress = true;
            }
        }
        else
        {
            // Collect frame bytes
            rx_buffer[rx_index++] = byte;

            // Check for end marker or buffer overflow
            if (byte == UART_END_MARKER || rx_index >= UART_MAX_FRAME_SIZE)
            {
                if (byte == UART_END_MARKER && rx_index >= 6)
                {
                    // We have a complete frame
                    if (parse_frame(rx_buffer, rx_index))
                    {
                        Serial.println("[UART] Valid frame received");
                    }
                }

                frame_in_progress = false;
                reset_rx_buffer();
            }
        }
    }

    handle_heartbeat();
}

bool UARTProtocol::parse_frame(uint8_t *buffer, uint16_t len)
{
    if (len < 6)
        return false; // Minimum frame size

    UARTFrame frame;
    frame.start = buffer[0];
    frame.cmd = buffer[1];
    frame.len = buffer[2];

    if (frame.len > UART_MAX_DATA_LEN)
        return false;
    if (len != 5 + frame.len)
        return false; // start+cmd+len+data+crc+end

    // Copy data
    if (frame.len > 0)
    {
        memcpy(frame.data, &buffer[3], frame.len);
    }

    frame.crc = buffer[3 + frame.len];
    frame.end = buffer[4 + frame.len];

    // Validate frame
    if (frame.start != UART_START_MARKER || frame.end != UART_END_MARKER)
    {
        return false;
    }

    // Check CRC
    uint8_t calculated_crc = calculate_crc(frame.cmd, frame.len, frame.data);
    if (frame.crc != calculated_crc)
    {
        send_error(ERR_CRC_MISMATCH, "CRC mismatch");
        return false;
    }

    process_received_frame(&frame);
    return true;
}

void UARTProtocol::process_received_frame(UARTFrame *frame)
{
    Serial.printf("[UART] Processing command 0x%02X\n", frame->cmd);

    switch (frame->cmd)
    {
    case CMD_PONG:
        if (state == CONNECTING)
        {
            state = CONNECTED;
            Serial.println("[UART] Connection established");
        }
        break;

    case CMD_BTN_STATE:
        if (response_callback)
        {
            response_callback(frame->cmd, frame->data, frame->len);
        }
        break;

    case CMD_SENSOR_DATA:
        if (response_callback)
        {
            response_callback(frame->cmd, frame->data, frame->len);
        }
        break;

    case CMD_ERROR:
        if (error_callback && frame->len > 0)
        {
            error_callback(frame->data[0], "Peripheral error");
        }
        break;

    default:
        if (response_callback)
        {
            response_callback(frame->cmd, frame->data, frame->len);
        }
        break;
    }
}

bool UARTProtocol::connect(uint32_t timeout_ms)
{
    Serial.println("[UART] Attempting to connect...");
    state = CONNECTING;

    uint32_t start_time = millis();
    uint32_t last_ping = 0;

    while (millis() - start_time < timeout_ms && state != CONNECTED)
    {
        // Send ping every 500ms
        if (millis() - last_ping > 500)
        {
            ping();
            last_ping = millis();
        }

        update();
        delay(10);
    }

    if (state == CONNECTED)
    {
        last_heartbeat = millis();
        Serial.println("[UART] Connected successfully");
        return true;
    }
    else
    {
        state = DISCONNECTED;
        Serial.println("[UART] Connection failed");
        return false;
    }
}

void UARTProtocol::disconnect()
{
    state = DISCONNECTED;
    Serial.println("[UART] Disconnected");
}

void UARTProtocol::handle_heartbeat()
{
    if (state == CONNECTED && millis() - last_heartbeat > UART_HEARTBEAT_MS)
    {
        ping();
        last_heartbeat = millis();
    }
}

// Command helper functions
bool UARTProtocol::led_set(uint8_t led_id, bool state, uint8_t brightness)
{
    uint8_t data[3] = {led_id, state ? 1 : 0, brightness};
    return send_command(CMD_LED_SET, data, 3);
}

bool UARTProtocol::led_rgb(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t data[4] = {led_id, r, g, b};
    return send_command(CMD_LED_RGB, data, 4);
}

bool UARTProtocol::led_pattern(uint8_t led_id, uint8_t pattern_id, uint8_t speed)
{
    uint8_t data[3] = {led_id, pattern_id, speed};
    return send_command(CMD_LED_PATTERN, data, 3);
}

bool UARTProtocol::sensor_read(uint8_t sensor_id)
{
    return send_command(CMD_SENSOR_READ, &sensor_id, 1);
}

bool UARTProtocol::sensor_config(uint8_t sensor_id, uint8_t *config_data, uint8_t config_len)
{
    uint8_t data[UART_MAX_DATA_LEN];
    data[0] = sensor_id;
    if (config_data && config_len > 0 && config_len < UART_MAX_DATA_LEN - 1)
    {
        memcpy(&data[1], config_data, config_len);
        return send_command(CMD_SENSOR_CONFIG, data, config_len + 1);
    }
    return send_command(CMD_SENSOR_CONFIG, data, 1);
}

bool UARTProtocol::sensor_auto_enable(uint8_t sensor_id, uint16_t interval_ms)
{
    uint8_t data[4] = {sensor_id, 1, (uint8_t)(interval_ms >> 8), (uint8_t)(interval_ms & 0xFF)};
    return send_command(CMD_SENSOR_AUTO, data, 4);
}

bool UARTProtocol::training_start(uint8_t training_id)
{
    return send_command(CMD_TRAINING_START, &training_id, 1);
}

bool UARTProtocol::training_stop()
{
    return send_command(CMD_TRAINING_STOP);
}

bool UARTProtocol::ping()
{
    return send_command(CMD_PING);
}

bool UARTProtocol::reset_peripheral()
{
    return send_command(CMD_RESET);
}

bool UARTProtocol::get_status()
{
    return send_command(CMD_STATUS);
}

void UARTProtocol::reset_rx_buffer()
{
    rx_index = 0;
    memset(rx_buffer, 0, UART_MAX_FRAME_SIZE);
}

void UARTProtocol::send_error(uint8_t error_code, const char *message)
{
    Serial.printf("[UART] Error: %s (code: 0x%02X)\n", message, error_code);
    if (error_callback)
    {
        error_callback(error_code, message);
    }
}

const char *UARTProtocol::get_state_string()
{
    switch (state)
    {
    case DISCONNECTED:
        return "DISCONNECTED";
    case CONNECTING:
        return "CONNECTING";
    case CONNECTED:
        return "CONNECTED";
    case ERROR_STATE:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void UARTProtocol::print_frame(UARTFrame *frame)
{
    Serial.printf("[UART] Frame: START=0x%02X CMD=0x%02X LEN=%d CRC=0x%02X END=0x%02X\n",
                  frame->start, frame->cmd, frame->len, frame->crc, frame->end);
}