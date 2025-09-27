#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
// #include "UARTProtocol.h" // COMMENTED OUT FOR TESTING

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// STEP 1: Add back basic UART2 (without protocol yet)
HardwareSerial slaveUART(2);

// COMMENTED OUT: UARTProtocol and test variables for now
/*
UARTProtocol protocol(&slaveUART);
static int buttonPressCount[16] = {0}; // Track button press counts
*/

// Forward declarations
// void handleSlaveMessage(const ProtocolMessage &message); // COMMENTED OUT
void update_time_display();

// SIMPLIFIED CONSOLE: Just show last 10 messages
#define MAX_LOG_LINES 10
static lv_obj_t *log_screen;
static lv_obj_t *log_labels[MAX_LOG_LINES];  // Array of labels for messages
static lv_obj_t *status_label;
static lv_obj_t *time_label; // Time display in upper right corner
static int current_log_line = 0; // Current line to write to

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

// Update time display - shows loop is alive
void update_time_display()
{
    if (time_label != NULL)
    {
        unsigned long now = millis();
        unsigned long seconds = (now / 1000) % 60;
        unsigned long minutes = (now / 60000) % 60;
        unsigned long hours = (now / 3600000) % 24;

        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02lu:%02lu:%02lu", hours, minutes, seconds);
        lv_label_set_text(time_label, time_str);

        // Debug output to serial to confirm function is called
        static int call_count = 0;
        if (++call_count % 10 == 0) // Print every 10 seconds
        {
            Serial.printf("Time update #%d: %s\n", call_count, time_str);
        }
    }
    else
    {
        Serial.println("ERROR: time_label is NULL!");
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

    // Create time display in upper right corner
    time_label = lv_label_create(log_screen);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFF00), 0); // Yellow text
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);

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

    // Create logging screen - SIMPLIFIED
    create_log_screen();

    // COMMENTED OUT: Initialize UART2 for slave communication
    /*
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
    */

    // STEP 1: Add back basic UART initialization
    Serial.println("=== STEP 1: BASIC UART TEST ===");
    Serial.println("System initialized - testing UART + time display");

    // Initialize UART2 for basic communication
    slaveUART.begin(115200, SERIAL_8N1, 44, 43); // RX=44, TX=43

    // Log initialization
    screen_log("STEP 1: Basic UART + Time Test");
    screen_log("UART2 initialized on pins RX=44, TX=43");
    update_status("Step 1 - Basic UART + Time Display");
}

void loop()
{
    static unsigned long lastLoopTime = 0;
    static unsigned long loopCounter = 0;
    unsigned long now = millis();

    // COMMENTED OUT: Watchdog: detect if loop is stuck
    /*
    if (now - lastLoopTime > 5000) // If more than 5 seconds since last loop
    {
        lastLoopTime = now;
        loopCounter++;
    }
    */

    // Process LVGL tasks (this can sometimes hang)
    lv_timer_handler();

    // STEP 1: Simple UART receive check (just log raw data)
    if (slaveUART.available())
    {
        String receivedData = slaveUART.readString();
        receivedData.trim(); // Remove whitespace
        if (receivedData.length() > 0)
        {
            Serial.printf("UART RX: %s\n", receivedData.c_str());

            // Log to screen occasionally (not every message to avoid spam)
            static unsigned long lastUartLog = 0;
            if (millis() - lastUartLog > 2000) // Every 2 seconds max
            {
                lastUartLog = millis();
                screen_log(("RX: " + receivedData).c_str());
            }
        }
    }

    // Update time display every second - SIMPLIFIED VERSION
    static unsigned long lastTimeUpdate = 0;
    if (now - lastTimeUpdate > 1000)
    {
        lastTimeUpdate = now;

        // Debug: confirm loop is running
        static int debug_counter = 0;
        debug_counter++;
        Serial.printf("Loop alive: %d seconds\n", debug_counter);

        // Try direct time update without locks first
        update_time_display();
    }

    // COMMENTED OUT: Send periodic status requests
    /*
    static unsigned long lastStatusRequest = 0;
    if (now - lastStatusRequest > 15000) // Every 15 seconds
    {
        lastStatusRequest = now;

        // Safe logging with timeout
        if (lvgl_port_lock(100) == 0) // 100ms timeout
        {
            char status_msg[150];
            snprintf(status_msg, sizeof(status_msg),
                     "STATUS: Uptime %lu sec, Loops: %lu", now / 1000, loopCounter);
            screen_log(status_msg);
            screen_log("Waiting for button events from slave...");
            lvgl_port_unlock();
        }
    }
    */

    delay(50); // Slightly longer delay to reduce CPU load
}

// COMMENTED OUT: Handle protocol messages from slave
/*
void handleSlaveMessage(const ProtocolMessage &message)
{
    // Use timeout to prevent infinite lock
    if (lvgl_port_lock(200) != 0) // 200ms timeout
    {
        return; // Skip this message if can't get lock
    }

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
*/
