#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "console.h" // <-- Include your new console module
#include "UARTProtocol.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

HardwareSerial slaveUART(2);
UARTProtocol uartProtocol(&slaveUART);

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

void printToConsole(const char *message)
{
    lvgl_port_lock(-1);
    console_log(message);
    lvgl_port_unlock();
}

void SetLedColor(int index, ProtocolColor color)
{
    String ledMsg = uartProtocol.createLEDSetPixelMessage(index, color);
    printToConsole(ledMsg.c_str());
    uartProtocol.sendMessage(ledMsg);
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
            // End of line - process the received message
            if (uartBuffer.length() > 0)
            {
                printToConsole(uartBuffer.c_str());

                ProtocolMessage msg = uartProtocol.parseMessage(uartBuffer);
                if (msg.type == MSG_BUTTON_PRESSED)
                {
                    int btnIndex = msg.param1;
                    SetLedColor(btnIndex, ProtocolColor::PROTO_COLOR_GREEN);
                }
                else if (msg.type == MSG_BUTTON_RELEASED)
                {
                    int btnIndex = msg.param1;
                    SetLedColor(btnIndex, ProtocolColor::PROTO_COLOR_BLACK);
                }

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
