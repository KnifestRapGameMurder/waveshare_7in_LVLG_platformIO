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

// ULTRA SIMPLE: Just show ONE last message
static lv_obj_t *log_screen;
static lv_obj_t *last_message_label; // Single label for the last message
static lv_obj_t *status_label;
static lv_obj_t *time_label; // Time display in upper right corner

// Initialize the board and display
void init_display()
{
    Board *board = new Board();
    board->init();
    assert(board->begin());

    // Initialize LVGL
    lvgl_port_init(board->getLCD(), board->getTouch());
}

// ULTRA SIMPLE: Just update one message label
void screen_log(const char *message)
{
    if (last_message_label == NULL)
        return;

    // Get current time
    unsigned long currentTime = millis();
    unsigned long seconds = currentTime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    // Format log entry with timestamp
    static char log_entry[200];
    snprintf(log_entry, sizeof(log_entry), "[%02lu:%02lu:%02lu] %s",
             hours, minutes, seconds, message);

    // Simply update the one label - it will auto-resize for long text
    lv_label_set_text(last_message_label, log_entry);
} // Update status display
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

// ULTRA SIMPLE: Create screen with just ONE message display
void create_log_screen()
{
    // Create main screen
    log_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(log_screen, lv_color_hex(0x000000), 0); // Black background

    // Create status label at top
    status_label = lv_label_create(log_screen);
    lv_label_set_text(status_label, "Last Message Only");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0); // Green text
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);

    // Create time display in upper right corner
    time_label = lv_label_create(log_screen);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFF00), 0); // Yellow text
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    // Create ONE message label that can handle long text
    last_message_label = lv_label_create(log_screen);
    lv_label_set_text(last_message_label, "System ready - waiting for messages...");
    lv_obj_set_style_text_color(last_message_label, lv_color_hex(0x00FF00), 0); // Green text

    // Set size and enable text wrapping for long messages
    lv_obj_set_size(last_message_label, 760, LV_SIZE_CONTENT);      // Width fixed, height auto
    lv_label_set_long_mode(last_message_label, LV_LABEL_LONG_WRAP); // Wrap long text
    lv_obj_align(last_message_label, LV_ALIGN_CENTER, 0, 0);        // Center position

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

    // STEP 1: INSTANT UART receive - character by character
    static String uartBuffer = "";
    while (slaveUART.available())
    {
        char c = slaveUART.read(); // Read immediately, no buffering delay
        if (c == '\n' || c == '\r')
        {
            if (uartBuffer.length() > 0)
            {
                Serial.printf("UART RX: %s\n", uartBuffer.c_str());

                // Update display immediately
                lvgl_port_lock(-1);
                screen_log(("RX: " + uartBuffer).c_str());
                lvgl_port_unlock();

                uartBuffer = ""; // Clear buffer
            }
        }
        else if (c >= 32 && c <= 126) // Printable characters only
        {
            uartBuffer += c;
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
