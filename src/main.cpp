#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include <deque> // ADDED: For managing the message buffer

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// STEP 1: Add back basic UART2 (without protocol yet)
HardwareSerial slaveUART(2);

// Forward declarations
void update_time_display();

// --- MODIFIED FOR LAST N MESSAGES ---
const int MAX_LOG_MESSAGES = 15; // Set N here
static lv_obj_t *log_screen;
static lv_obj_t *log_label;           // A single label to hold all log messages
static std::deque<String> log_buffer; // Buffer to store the last N messages
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

// MODIFIED: Add message to a scrolling log
void screen_log(const char *message)
{
    if (log_label == NULL)
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
    char log_entry[200];
    snprintf(log_entry, sizeof(log_entry), "[%02lu:%02lu:%02lu] %s",
             hours, minutes, seconds, message);

    // Add new message to buffer
    log_buffer.push_back(String(log_entry));

    // If buffer is full, remove the oldest message
    if (log_buffer.size() > MAX_LOG_MESSAGES)
    {
        log_buffer.pop_front();
    }

    // Build the full log text from the buffer
    String full_log_text;
    for (const auto &line : log_buffer)
    {
        full_log_text += line;
        full_log_text += "\n";
    }

    // Update the label with the combined text
    lv_label_set_text(log_label, full_log_text.c_str());
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
    }
}

// MODIFIED: Create screen with a multi-line log display
void create_log_screen()
{
    // Create main screen
    log_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(log_screen, lv_color_hex(0x000000), 0); // Black background

    // Create status label at top
    status_label = lv_label_create(log_screen);
    lv_label_set_text(status_label, "Message Log");                       // Updated title
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0); // Green text
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);

    // Create time display in upper right corner
    time_label = lv_label_create(log_screen);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFF00), 0); // Yellow text
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    // Create the log label for multiple messages
    log_label = lv_label_create(log_screen);
    screen_log("System ready - waiting for messages...");              // Initial message
    lv_obj_set_style_text_color(log_label, lv_color_hex(0x00FF00), 0); // Green text

    // Set size to fill the screen and enable text wrapping
    lv_obj_set_width(log_label, lv_disp_get_hor_res(NULL) - 20); // Screen width - 20px padding
    lv_label_set_long_mode(log_label, LV_LABEL_LONG_WRAP);       // Wrap long text
    lv_obj_align(log_label, LV_ALIGN_TOP_LEFT, 10, 40);          // Align below the top labels

    // Load the screen
    lv_scr_load(log_screen);
}

void setup()
{
    // Initialize display first
    init_display();

    // Create logging screen
    create_log_screen();

    Serial.begin(115200);
    Serial.println("=== UART LOG DISPLAY TEST ===");
    Serial.println("System initialized - testing display of last N messages");

    // Initialize UART2 for basic communication
    slaveUART.begin(115200, SERIAL_8N1, 44, 43); // RX=44, TX=43

    // Log initialization messages
    screen_log("UART LOG DISPLAY");
    screen_log("UART2 initialized on pins RX=44, TX=43");
    update_status("Receiving UART messages...");
}

void loop()
{
    // Process LVGL tasks
    lv_timer_handler();

    // Process incoming UART characters
    static String uartBuffer = "";
    while (slaveUART.available())
    {
        char c = slaveUART.read();
        if (c == '\n' || c == '\r')
        {
            if (uartBuffer.length() > 0)
            {
                Serial.printf("UART RX: %s\n", uartBuffer.c_str());

                // Update display immediately
                lvgl_port_lock(-1);
                screen_log(uartBuffer.c_str());
                lvgl_port_unlock();

                uartBuffer = ""; // Clear buffer
            }
        }
        else if (c >= 32 && c <= 126) // Printable characters only
        {
            uartBuffer += c;
        }
    }

    // Update time display every second
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate > 1000)
    {
        lastTimeUpdate = millis();
        update_time_display();
    }

    delay(50); // Reduce CPU load
}