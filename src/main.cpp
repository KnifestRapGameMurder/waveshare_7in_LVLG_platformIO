#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "console.h" // <-- Include your new console module

using namespace esp_panel::drivers;
using namespace esp_panel::board;

HardwareSerial slaveUART(2);

void init_display()
{
    Board *board = new Board();
    board->init();
    assert(board->begin());
    lvgl_port_init(board->getLCD(), board->getTouch());
}

void setup()
{
    // Initialize hardware
    init_display();
    Serial.begin(115200);
    slaveUART.begin(115200, SERIAL_8N1, 44, 43);

    // Initialize the console UI
    console_init();
    Serial.println("=== UART LOG DISPLAY TEST ===");

    // Use the console functions to log messages
    console_log("System initialized.");
    console_log("UART2 configured on pins RX=44, TX=43.");
    console_set_status("Receiving UART messages...");
}

void loop()
{
    // Process LVGL tasks
    lv_timer_handler();

    // Handle periodic console updates (e.g., the clock)
    console_update();

    // --- Application Logic ---
    static String uartBuffer = "";
    while (slaveUART.available())
    {
        char c = slaveUART.read();
        if (c == '\n' || c == '\r')
        {
            if (uartBuffer.length() > 0)
            {
                Serial.printf("UART RX: %s\n", uartBuffer.c_str());

                // Log the received message to the display
                lvgl_port_lock(-1);
                console_log(uartBuffer.c_str());
                lvgl_port_unlock();

                uartBuffer = ""; // Clear buffer
            }
        }
        else if (c >= 32 && c <= 126)
        {
            uartBuffer += c;
        }
    }

    delay(50); // Reduce CPU load
}