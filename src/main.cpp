#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// UART2 for slave communication (when switcher is on UART2)
HardwareSerial slaveUART(2);

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
    screen_log("System initialized");
    screen_log("UART2 configured on pins RX=44, TX=43");
    screen_log("Baud rate: 115200");
    screen_log("MODE: MASTER SENDING TO SLAVE");
    screen_log("Sending test messages every 2 seconds...");

    update_status("UART2 Master Sender - Ready");
}

void loop()
{
    // Process LVGL tasks
    lv_timer_handler();

    // Send test messages to slave every 2 seconds
    static unsigned long lastSendTime = 0;
    static unsigned long messageCounter = 0;
    unsigned long now = millis();

    // if (now - lastSendTime >= 5000) // Every 5 seconds
    // {
    //     lastSendTime = now;
    //     messageCounter++;

    //     // Create test message
    //     char testMessage[100];
    //     snprintf(testMessage, sizeof(testMessage),
    //              "MASTER_TEST_MSG_%lu_TIME_%lu", messageCounter, now);

    //     // Send via UART2
    //     slaveUART.println(testMessage);

    //     // Log to screen
    //     lvgl_port_lock(-1);
    //     char log_msg[150];
    //     snprintf(log_msg, sizeof(log_msg), "SENT: %s", testMessage);
    //     screen_log(log_msg);

    //     char status_msg[100];
    //     snprintf(status_msg, sizeof(status_msg), "Sent message #%lu", messageCounter);
    //     update_status(status_msg);

    //     lvgl_port_unlock();
    // }

    // Check for slave data FIRST - highest priority
    if (slaveUART.available())
    {
        lvgl_port_lock(-1);

        // Read ALL available data immediately
        String receivedData = "";
        int bytesRead = 0;
        while (slaveUART.available())
        {
            char c = slaveUART.read();
            receivedData += c;
            bytesRead++;
            delay(1); // Small delay to let more data arrive
        }

        if (receivedData.length() > 0)
        {
            // Display the raw received message
            char msg[400];
            snprintf(msg, sizeof(msg), "SLAVE MSG (%d bytes): %s", bytesRead, receivedData.c_str());
            screen_log(msg);

            // Update status to show we got data
            char status_update[100];
            snprintf(status_update, sizeof(status_update), "RECEIVED DATA! %d bytes", bytesRead);
            update_status(status_update);
        }

        lvgl_port_unlock();
    }

    // Status updates (less frequent, and won't interfere with data reading)
    static unsigned long lastStatus = 0;
    if (now - lastStatus > 10000) // Every 10 seconds, less frequent
    {
        lastStatus = now;

        lvgl_port_lock(-1);

        char status_msg[200];
        snprintf(status_msg, sizeof(status_msg),
                 "STATUS: Uptime %lu sec, UART avail: %d bytes, Switcher=UART2",
                 now / 1000, slaveUART.available());
        screen_log(status_msg);

        // Just show connection status without consuming data
        if (slaveUART.available() > 0)
        {
            char avail_msg[100];
            snprintf(avail_msg, sizeof(avail_msg), ">>> %d BYTES WAITING - will be read next! <<<", slaveUART.available());
            screen_log(avail_msg);
        }
        else
        {
            screen_log("No UART data detected - check connections/slave");
        }

        // Update main status
        char main_status[100];
        snprintf(main_status, sizeof(main_status),
                 "UART2 Logger - Uptime: %lu sec", now / 1000);
        update_status(main_status);

        lvgl_port_unlock();
    }

    delay(50);
}