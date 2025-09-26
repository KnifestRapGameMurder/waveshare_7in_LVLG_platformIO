#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

// Protocol constants
#define UART_START_MARKER 0xAA
#define UART_END_MARKER 0x55
#define UART_MAX_DATA_LEN 250
#define UART_MAX_FRAME_SIZE 255
#define UART_TIMEOUT_MS 1000
#define UART_RETRY_COUNT 3
#define UART_HEARTBEAT_MS 30000

// Command categories
#define CMD_LED_SET 0x10
#define CMD_LED_RGB 0x11
#define CMD_LED_PATTERN 0x12
#define CMD_LED_STATUS 0x13

#define CMD_BTN_STATE 0x20
#define CMD_BTN_CONFIG 0x21
#define CMD_BTN_STATUS 0x22

#define CMD_SENSOR_READ 0x30
#define CMD_SENSOR_DATA 0x31
#define CMD_SENSOR_CONFIG 0x32
#define CMD_SENSOR_AUTO 0x33

#define CMD_PING 0x40
#define CMD_PONG 0x41
#define CMD_RESET 0x42
#define CMD_STATUS 0x43
#define CMD_ERROR 0x44

#define CMD_TRAINING_START 0x50
#define CMD_TRAINING_STOP 0x51
#define CMD_TRAINING_DATA 0x52
#define CMD_TRAINING_STATUS 0x53

// Error codes
#define ERR_INVALID_CMD 0x01
#define ERR_INVALID_LEN 0x02
#define ERR_CRC_MISMATCH 0x03
#define ERR_HARDWARE 0x04
#define ERR_TIMEOUT 0x05
#define ERR_BUSY 0x06
#define ERR_UNKNOWN 0xFF

// Connection states
enum ConnectionState
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR_STATE
};

// Frame structure
struct UARTFrame
{
    uint8_t start;                   // 0xAA
    uint8_t cmd;                     // Command ID
    uint8_t len;                     // Data length
    uint8_t data[UART_MAX_DATA_LEN]; // Data payload
    uint8_t crc;                     // Checksum
    uint8_t end;                     // 0x55
};

// Response callback function type
typedef void (*ResponseCallback)(uint8_t cmd, uint8_t *data, uint8_t len);
typedef void (*ErrorCallback)(uint8_t error_code, const char *message);

class UARTProtocol
{
private:
    HardwareSerial *uart;
    ConnectionState state;
    ResponseCallback response_callback;
    ErrorCallback error_callback;

    uint32_t last_heartbeat;
    uint32_t last_command_time;

    // Internal buffers
    uint8_t tx_buffer[UART_MAX_FRAME_SIZE];
    uint8_t rx_buffer[UART_MAX_FRAME_SIZE];
    uint16_t rx_index;
    bool frame_in_progress;

    // CRC calculation
    uint8_t calculate_crc(uint8_t cmd, uint8_t len, uint8_t *data);

    // Frame handling
    bool parse_frame(uint8_t *buffer, uint16_t len);
    void process_received_frame(UARTFrame *frame);

    // Internal utilities
    void reset_rx_buffer();
    void send_error(uint8_t error_code, const char *message);

public:
    UARTProtocol(HardwareSerial *serial);

    // Initialization
    bool begin(uint32_t baud_rate = 115200);
    void set_response_callback(ResponseCallback callback);
    void set_error_callback(ErrorCallback callback);

    // Connection management
    bool connect(uint32_t timeout_ms = 5000);
    void disconnect();
    ConnectionState get_state() { return state; }
    bool is_connected() { return state == CONNECTED; }

    // Command sending
    bool send_command(uint8_t cmd, uint8_t *data = nullptr, uint8_t len = 0);
    bool send_command_with_response(uint8_t cmd, uint8_t *data, uint8_t len,
                                    uint8_t *response_data, uint8_t *response_len,
                                    uint32_t timeout_ms = UART_TIMEOUT_MS);

    // Specific command helpers
    bool led_set(uint8_t led_id, bool state, uint8_t brightness = 255);
    bool led_rgb(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b);
    bool led_pattern(uint8_t led_id, uint8_t pattern_id, uint8_t speed = 100);

    bool sensor_read(uint8_t sensor_id);
    bool sensor_config(uint8_t sensor_id, uint8_t *config_data, uint8_t config_len);
    bool sensor_auto_enable(uint8_t sensor_id, uint16_t interval_ms);

    bool training_start(uint8_t training_id);
    bool training_stop();

    bool ping();
    bool reset_peripheral();
    bool get_status();

    // Update function - call in main loop
    void update();

    // Heartbeat management
    void handle_heartbeat();

    // Debug
    void print_frame(UARTFrame *frame);
    const char *get_state_string();
};