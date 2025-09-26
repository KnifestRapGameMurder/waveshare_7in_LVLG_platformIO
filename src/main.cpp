#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "UARTProtocol.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// UART2 for slave communication with proper protocol
HardwareSerial slaveUART(2);
UARTProtocol protocol(&slaveUART);

// Test variables
static int buttonPressCount[16] = {0}; // Track button press counts

// Forward declarations
void handleSlaveMessage(const ProtocolMessage &message);

// LVGL objects for on-screen logging
static lv_obj_t *log_screen;
static lv_obj_t *log_textarea;
static lv_obj_t *status_label;

// Initialize the board and display
void init_display()
{
    Board *board = new Board();
    board->init();
    assert(board->begin());

    // Initialize LVGL
    lvgl_port_init(board->getLCD(), board->getTouch());
}

// Add log message to screen
void screen_log(const char *message)
{
    if (log_textarea == NULL)
        return;

    // Get current time
    unsigned long currentTime = millis();
    unsigned long seconds = currentTime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    // Format log entry
    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "[%02lu:%02lu:%02lu] %s\n",
             hours, minutes, seconds, message);

    // Get current text and append new log
    const char *current_text = lv_textarea_get_text(log_textarea);
    size_t current_len = strlen(current_text);
    size_t new_entry_len = strlen(log_entry);
    size_t total_len = current_len + new_entry_len + 1;

    char *new_text = (char *)malloc(total_len);
    if (new_text == NULL)
        return;

    strcpy(new_text, current_text);
    strcat(new_text, log_entry);

    // Limit text length to prevent memory issues
    if (strlen(new_text) > 2000)
    {
        // Keep only last 1500 characters
        char *trimmed = new_text + (strlen(new_text) - 1500);
        lv_textarea_set_text(log_textarea, trimmed);
    }
    else
    {
        lv_textarea_set_text(log_textarea, new_text);
    }

    // Scroll to bottom
    lv_textarea_set_cursor_pos(log_textarea, LV_TEXTAREA_CURSOR_LAST);

    free(new_text);
}

// Update status display
void update_status(const char *status)
{
    if (status_label != NULL)
    {
        lv_label_set_text(status_label, status);
    }
}

// Create the logging screen
void create_log_screen()
{
    // Create main screen
    log_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(log_screen, lv_color_hex(0x000000), 0); // Black background

    // Create status label at top
    status_label = lv_label_create(log_screen);
    lv_label_set_text(status_label, "UART2 Logger - Waiting for slave...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0); // Green text
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);

    // Create log textarea (terminal style)
    log_textarea = lv_textarea_create(log_screen);
    lv_obj_set_size(log_textarea, 780, 400); // Almost full screen
    lv_obj_align(log_textarea, LV_ALIGN_CENTER, 0, 20);

    // Terminal styling
    lv_obj_set_style_bg_color(log_textarea, lv_color_hex(0x000000), 0);     // Black background
    lv_obj_set_style_text_color(log_textarea, lv_color_hex(0x00FF00), 0);   // Green text
    lv_obj_set_style_border_color(log_textarea, lv_color_hex(0x00FF00), 0); // Green border
    lv_obj_set_style_border_width(log_textarea, 2, 0);
    lv_obj_set_style_pad_all(log_textarea, 10, 0);

    // Make it read-only
    lv_obj_clear_flag(log_textarea, LV_OBJ_FLAG_CLICKABLE);

    // Set initial text
    lv_textarea_set_text(log_textarea, "=== UART2 ON-SCREEN LOGGER ===\n");

    // Load the screen
    lv_scr_load(log_screen);
}

void setup()
{
    // Initialize display first
    init_display();

    // Create logging screen
    lvgl_port_lock(-1);
    create_log_screen();
    lvgl_port_unlock();

    // Initialize UART2 for slave communication
    slaveUART.begin(115200, SERIAL_8N1, 44, 43); // RX=44, TX=43

    // Log initialization
    screen_log("=== SIMPLE BUTTON-LED TEST ===");
    screen_log("System initialized");
    screen_log("UART2 configured on pins RX=44, TX=43");
    screen_log("Baud rate: 115200");
    screen_log("Test: Button Press -> LED On");
    screen_log("Test: Button Release -> LED Off");
    screen_log("Sending handshake to slave...");

    // Send handshake to establish connection
    protocol.sendMessage(protocol.createHandshakeMessage("MASTER_BUTTON_LED_TEST"));

    update_status("Simple Test - Waiting for button events...");
}

void loop()
{
    // Process LVGL tasks
    lv_timer_handler();
    unsigned long now = millis();

    // Handle incoming protocol messages from slave
    ProtocolMessage message;
    if (protocol.receiveMessage(message))
    {
        handleSlaveMessage(message);
    }

    // Simple status update every 15 seconds
    static unsigned long lastStatus = 0;
    if (now - lastStatus > 15000)
    {
        lastStatus = now;

        lvgl_port_lock(-1);
        char status_msg[100];
        snprintf(status_msg, sizeof(status_msg), "STATUS: Uptime %lu sec", now / 1000);
        screen_log(status_msg);
        screen_log("Waiting for button events from slave...");
        lvgl_port_unlock();
    }

    delay(10);
}

// Handle protocol messages from slave
void handleSlaveMessage(const ProtocolMessage &message)
{
    lvgl_port_lock(-1);

    switch (message.type)
    {
    case MSG_HANDSHAKE:
        screen_log(">>> SLAVE CONNECTED! <<<");
        screen_log(("Slave info: " + message.data).c_str());
        protocol.sendMessage(protocol.createAckMessage("HANDSHAKE"));
        update_status("Protocol Test - Slave connected!");
        break;

    case MSG_BUTTON_PRESSED:
    {
        char msg[100];
        snprintf(msg, sizeof(msg), "BUTTON %d PRESSED", message.param1);
        screen_log(msg);

        // Light up the corresponding LED in GREEN
        protocol.sendMessage(protocol.createLEDSetPixelMessage(message.param1, PROTO_COLOR_GREEN));

        char response[100];
        snprintf(response, sizeof(response), "-> Sent: LED %d ON (GREEN)", message.param1);
        screen_log(response);

        update_status(("Button " + String(message.param1) + " pressed - LED ON").c_str());
    }
    break;

    case MSG_BUTTON_RELEASED:
    {
        char msg[100];
        snprintf(msg, sizeof(msg), "BUTTON %d RELEASED", message.param1);
        screen_log(msg);

        // Turn off the corresponding LED
        protocol.sendMessage(protocol.createLEDSetPixelMessage(message.param1, PROTO_COLOR_BLACK));

        char response[100];
        snprintf(response, sizeof(response), "-> Sent: LED %d OFF", message.param1);
        screen_log(response);

        update_status(("Button " + String(message.param1) + " released - LED OFF").c_str());
    }
    break;

    case MSG_BUTTON_STATE:
    {
        char msg[100];
        snprintf(msg, sizeof(msg), "BUTTON STATE: 0x%04X", message.param3);
        screen_log(msg);
    }
    break;

    case MSG_HALL_DETECTED:
        screen_log(">>> HALL SENSOR DETECTED! <<<");
        // Flash all LEDs when hall sensor detected
        protocol.sendMessage(protocol.createLEDEffectMessage(PROTO_EFFECT_SPARKLE));
        screen_log("-> Started SPARKLE effect");
        break;

    case MSG_HALL_REMOVED:
        screen_log(">>> HALL SENSOR REMOVED <<<");
        protocol.sendMessage(protocol.createLEDClearMessage());
        screen_log("-> Cleared all LEDs");
        break;

    case MSG_STATUS:
    {
        char msg[150];
        snprintf(msg, sizeof(msg), "Slave status - Time: %u, Buttons: 0x%04X",
                 message.param3, (message.param2 << 8) | message.param1);
        screen_log(msg);
    }
    break;

    case MSG_ERROR:
        screen_log(("SLAVE ERROR: " + message.data).c_str());
        break;

    case MSG_ACK:
        screen_log(("Slave ACK: " + message.data).c_str());
        break;

    default:
        screen_log("Unknown message from slave");
        break;
    }

    lvgl_port_unlock();
}
