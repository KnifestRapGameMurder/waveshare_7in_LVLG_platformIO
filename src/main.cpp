/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "lv_conf.h"
#include <math.h>
#include "uart_protocol.h"
#include <Preferences.h>

#include <Wire.h>
#include <TCA9548.h>
#include <PCF8575.h>

// Include external components
#include "loading_screen.h"
#include "app_screens.h" // New: Includes AppState, extern vars, and screen creation functions

using namespace esp_panel::drivers;
using namespace esp_panel::board;

extern uint16_t button_state_cache;

// ---------- ANTI-TEARING CONFIGURATION (Based on ESP-BSP proven solutions) ----------
static const uint32_t TARGET_FPS = 30;
static const uint32_t FRAME_MS = 1000 / TARGET_FPS;

// Application states and variables (No longer static, accessible via extern in app_screens.h)
AppState current_state = STATE_LOADING;
uint32_t state_start_time = 0;
const uint32_t IDLE_TIMEOUT = 10000; // 10 seconds idle timeout
uint32_t last_interaction_time = 0;
Preferences preferences;

#define TCA_ADDR 0x70 // адреса мультиплексора
#define PCF_ADDR 0x20 // адреса PCF8575

// TCA9548 та PCF8575 будуть ініціалізовані в setup() з Wire1
TCA9548 *tca1 = nullptr;
PCF8575 *pcf = nullptr;
int pcf_channel = 0; // Зберігаємо канал, на якому знайдено PCF8575

// ========== UART SWITCH ===========
#define UART_USED 0
// ========== UART SWITCH ===========

HardwareSerial uart_serial(2);
#if UART_USED
UARTProtocol uart_protocol(&uart_serial);
#else
UARTProtocol uart_protocol(nullptr); // Dummy
#endif

// Screen dimensions (No longer static, accessible via extern in app_screens.h)
int32_t SCR_W = 800,
        SCR_H = 480;

// Note: UI objects (menu_buttons, back_button) are now extern and defined in app_screens.cpp.

// Forward declaration for the timer callback (remains here as it controls the core loop)
static void app_timer_cb(lv_timer_t *timer);

///
static void lv_color_to_hex6(lv_color_t c, char out[7]) // out: "RRGGBB"
{
    uint32_t xrgb = lv_color_to32(c); // XRGB8888 (alpha=0xFF)
    uint8_t r = (xrgb >> 16) & 0xFF;
    uint8_t g = (xrgb >> 8) & 0xFF;
    uint8_t b = (xrgb >> 0) & 0xFF;
    snprintf(out, 7, "%02X%02X%02X", r, g, b);
}
///

/**
 * @brief Main application timer for state management and continuous updates.
 */
static void app_timer_cb(lv_timer_t *timer)
{
    static uint32_t last_time = 0;
    uint32_t now = lv_tick_get();

    // 1. Check for idle timeout (return to loading screen if no interaction)
    if (current_state == STATE_MAIN_MENU)
    {
        // Serial.printf("[IDLE] now=%u, last=%u, diff=%u\n", now, last_interaction_time, now - last_interaction_time);

        if ((now - last_interaction_time) >= IDLE_TIMEOUT)
        {
            Serial.println("[ТАЙМ-АУТ] Досягнуто тайм-аут бездіяльності - повернення до екрану завантаження");
            lvgl_port_lock(-1); // Lock for UI changes
            current_state = STATE_LOADING;
            state_start_time = now;
            // Uses app_screen_touch_cb from app_screens.h
            loading_screen_create(app_screen_touch_cb);
            lvgl_port_unlock(); // Unlock
            return;
        }
    }

    // 2. Run animation update if in loading state
    if (current_state == STATE_LOADING)
    {
        // Calculate delta time for FPS-independent animation
        float dt = (now - last_time) / 1000.0f;
        if (last_time == 0 || dt < 0.001f || dt > 0.1f)
            dt = 0.033f; // 30 FPS fallback

        lvgl_port_lock(-1); // Lock for UI changes
        loading_screen_update_animation(dt);
        lvgl_port_unlock(); // Unlock
    }
    // 3. Run trainer logic based on current state
    else
    {
        lvgl_port_lock(-1); // Lock for UI changes in trainers
        switch (current_state)
        {
        case STATE_ACCURACY_TRAINER:
            run_accuracy_trainer();
            break;
        case STATE_REACTION_TIME_TRIAL:
            run_time_trial();
            break;
        case STATE_REACTION_SURVIVAL:
            run_survival_time_trainer();
            break;
        case STATE_MEMORY_TRAINER:
            run_memory_trainer();
            break;
        case STATE_COORDINATION_TRAINER:
            run_coordination_trainer();
            break;
        default:
            // Menu states don't need continuous updates
            break;
        }
        lvgl_port_unlock(); // Unlock
    }

    if (tca1 != nullptr && pcf != nullptr)
    {
        tca1->selectChannel(pcf_channel); // Використовуємо збережений канал
        
        // PCF діагностика та зчитування
        static uint32_t last_pcf_debug = 0;
        uint32_t now_pcf = lv_tick_get();
        
        button_state_cache = 0;
        for (uint8_t i = 0; i < 16; i++)
        {
            bool val = pcf->digitalRead(i);
            bitWrite(button_state_cache, i, val);
        }
        
        // Виводимо стан кнопок кожні 2 секунди для діагностики
        if ((now_pcf - last_pcf_debug) >= 2000)
        {
            Serial.printf("[PCF8575] Стан кнопок (біти): ");
            for (int i = 15; i >= 0; i--)
            {
                Serial.print((button_state_cache & (1 << i)) ? "1" : "0");
            }
            Serial.printf(" (0x%04X)\n", button_state_cache);
            last_pcf_debug = now_pcf;
        }
    }
    else
    {
        // Якщо PCF не ініціалізований, виводимо помилку кожні 5 секунд
        static uint32_t last_pcf_debug = 0;
        uint32_t now_pcf = lv_tick_get();
        
        if ((now_pcf - last_pcf_debug) >= 5000)
        {
            Serial.println("[PCF8575] ❌ PCF8575 не ініціалізований!");
            last_pcf_debug = now_pcf;
        }
        button_state_cache = 0; // Скидаємо стан кнопок
    }

    // 4. Handle UART communication with slave (LEDs and buttons)
    // ProtocolMessage msg;
    // if (uart_protocol.receiveMessage(msg))
    // {
    //     switch (msg.type)
    //     {
    //     case MSG_BUTTON_PRESSED:
    //         // Set the bit for the pressed button (param1 = button index)
    //         button_state_cache |= (1 << msg.param1);
    //         Serial.printf("[UART] Button %d pressed\n", msg.param1);
    //         break;
    //     case MSG_BUTTON_RELEASED:
    //         // Clear the bit for the released button (param1 = button index)
    //         button_state_cache &= ~(1 << msg.param1);
    //         Serial.printf("[UART] Button %d released\n", msg.param1);
    //         break;
    //     // Add other message types if needed (e.g., acknowledgments)
    //     default:
    //         break;
    //     }
    // }

    // Update debug label with current button states
    lvgl_port_lock(-1); // Lock for UI changes
    if (debug_label)
    {
        char buf[17];
        for (int i = 0; i < 16; i++)
        {
            buf[i] = (button_state_cache & (1 << i)) ? '1' : '0';
        }
        buf[16] = '\0';
        lv_label_set_text(debug_label, buf);
    }
    lvgl_port_unlock(); // Unlock

    last_time = now;
}

void setup()
{
    Serial.begin(115200);
    Serial.println("=== ESP32-S3 RGB LCD Система Без Розривів ===");

    // Спочатку ініціалізуємо плату (це налаштує I2C драйвер)
    Serial.println("Ініціалізація плати з конфігурацією без розривів...");
    Board *board = new Board();
    board->init();

#if UART_USED
    uart_serial.begin(115200, SERIAL_8N1, 44, 43);
#endif

// Configure anti-tearing RGB double-buffer mode (ESP-BSP style)
#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();

    Serial.println("Налаштування RGB подвійного буфера для усунення розривів...");
    lcd->configFrameBufferNumber(2); // RGB double-buffer mode

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
    {
        Serial.println("Налаштування RGB буфера відскоку для ESP32-S3...");
        int bounce_buffer_height = lcd->getFrameHeight() / 10;
        int bounce_buffer_size = lcd->getFrameWidth() * bounce_buffer_height;

        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(bounce_buffer_size);

        Serial.printf("RGB буфер відскоку налаштовано: %dx%d пікселів\n",
                      lcd->getFrameWidth(), bounce_buffer_height);
    }
#endif
#endif

    assert(board->begin());
    Serial.println("Плата ініціалізована з RGB конфігурацією без розривів!");

    // Ініціалізуємо другий I2C контролер ПІСЛЯ ESP Panel
    Serial.println("🔧 Ініціалізація другого I2C контролера для зовнішніх пристроїв...");
    
    // Повертаємося до GPIO8/9 тепер, коли touch відключений
    const int EXTERNAL_SDA_PIN = 8;
    const int EXTERNAL_SCL_PIN = 9;
    Wire1.begin(EXTERNAL_SDA_PIN, EXTERNAL_SCL_PIN); // Використовуємо Wire1 замість Wire
    
    // I2C сканер для діагностики
    Serial.printf("🔍 Сканування зовнішніх I2C пристроїв на GPIO%d(SDA)/GPIO%d(SCL)...\n", EXTERNAL_SDA_PIN, EXTERNAL_SCL_PIN);
    int found_devices = 0;
    for (byte address = 1; address < 127; address++)
    {
        Wire1.beginTransmission(address);
        byte error = Wire1.endTransmission();
        if (error == 0)
        {
            Serial.printf("✅ I2C пристрій знайдено на адресі 0x%02X\n", address);
            found_devices++;
        }
        else if (error == 4)
        {
            Serial.printf("⚠️ Невідома помилка на адресі 0x%02X\n", address);
        }
    }
    if (found_devices == 0)
    {
        Serial.printf("❌ Зовнішніх I2C пристроїв не знайдено на GPIO%d/GPIO%d!\n", EXTERNAL_SDA_PIN, EXTERNAL_SCL_PIN);
        Serial.println("🔧 Перевірте підключення TCA9548A та PCF8575");
    }
    else
    {
        Serial.printf("📊 Знайдено %d зовнішніх I2C пристроїв\n", found_devices);
    }
    // Створюємо TCA9548 з Wire1
    tca1 = new TCA9548(TCA_ADDR, &Wire1); // Використовуємо Wire1
    
    if (!tca1->begin())
    {
        Serial.println("❌ Не знайдено TCA9548A на GPIO8/9!");
        Serial.println("💡 Перевірте підключення або спробуйте використати інші піни");
        // Не блокуємо виконання, просто встановлюємо pcf як nullptr
        delete tca1;
        tca1 = nullptr;
        pcf = nullptr;
    }
    else
    {
        Serial.println("✅ Знайдено TCA9548A на GPIO8/9.");

        // Шукаємо PCF8575 на всіх каналах TCA9548A
        bool pcf_found = false;
        int pcf_channel = -1;
        
        for (int channel = 0; channel < 8; channel++)
        {
            Serial.printf("🔍 Перевірка каналу %d TCA9548A...\n", channel);
            tca1->selectChannel(channel);
            delay(10); // Невелика затримка для стабілізації
            
            // Перевіряємо кілька можливих адрес PCF8575
            uint8_t pcf_addresses[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27};
            
            for (int addr_idx = 0; addr_idx < 8; addr_idx++)
            {
                uint8_t test_addr = pcf_addresses[addr_idx];
                Wire1.beginTransmission(test_addr);
                byte error = Wire1.endTransmission();
                
                if (error == 0)
                {
                    Serial.printf("� Знайдено пристрій на каналі %d, адреса 0x%02X\n", channel, test_addr);
                    
                    // Спробуємо ініціалізувати як PCF8575
                    PCF8575 *test_pcf = new PCF8575(&Wire1, test_addr);
                    if (test_pcf->begin())
                    {
                        Serial.printf("✅ PCF8575 успішно ініціалізовано на каналі %d, адреса 0x%02X!\n", channel, test_addr);
                        pcf = test_pcf;
                        pcf_found = true;
                        pcf_channel = channel;
                        
                        // Тестування PCF8575 - зчитуємо всі піни
                        Serial.print("🧪 Тест зчитування PCF8575: ");
                        uint16_t test_val = 0;
                        for (uint8_t i = 0; i < 16; i++)
                        {
                            bool pin_val = pcf->digitalRead(i);
                            bitWrite(test_val, i, pin_val);
                        }
                        Serial.printf("0x%04X\n", test_val);
                        
                        // --- Встановлюємо всі піни як INPUT_PULLUP ---
                        for (uint8_t i = 0; i < 16; i++)
                        {
                            pcf->pinMode(i, INPUT_PULLUP);
                        }
                        break;
                    }
                    else
                    {
                        delete test_pcf;
                    }
                }
            }
            
            if (pcf_found) break;
        }
        
        if (!pcf_found)
        {
            Serial.println("❌ PCF8575 не знайдено на жодному каналі TCA9548A!");
            Serial.println("🔧 Перевірте фізичне підключення PCF8575");
            pcf = nullptr;
        }
        else
        {
            Serial.printf("📋 PCF8575 готовий до роботи на каналі %d\n", pcf_channel);
        }
    }

    Serial.println("📋 Готово до зчитування кнопок...\n");

    Serial.println("Ініціалізація LVGL з повним оновленням...");
    // Тимчасово відключаємо touch для усунення I2C конфліктів
    lvgl_port_init(board->getLCD(), nullptr); // nullptr замість board->getTouch()

    Serial.println("Створення UI з градієнтом без розривів...");
    lvgl_port_lock(-1);

    // Get actual screen dimensions
    SCR_W = lv_disp_get_hor_res(NULL);
    SCR_H = lv_disp_get_ver_res(NULL);

    Serial.printf("Роздільність екрану: %dx%d\n", SCR_W, SCR_H);
    
    // Перевіряємо кольорову конфігурацію LVGL
    Serial.printf("LVGL кольорова глибина: %d біт\n", LV_COLOR_DEPTH);
    
    // Тестуємо кольори
    lv_color_t test_red = lv_color_make(255, 0, 0);
    lv_color_t test_green = lv_color_make(0, 255, 0);
    lv_color_t test_blue = lv_color_make(0, 0, 255);
    
    Serial.printf("Тест кольорів - Red: 0x%04X, Green: 0x%04X, Blue: 0x%04X\n", 
                  lv_color_to16(test_red), lv_color_to16(test_green), lv_color_to16(test_blue));

    // Initialize orbit parameters using the new function
    loading_screen_init_params(SCR_W, SCR_H);

    // Initialize app state
    current_state = STATE_LOADING;
    state_start_time = lv_tick_get();
    last_interaction_time = state_start_time;

    Serial.println("[НАЛАГОДЖЕННЯ] Додаток ініціалізовано - Запуск в стані ЗАВАНТАЖЕННЯ");

    // Create initial loading screen using the new function
    loading_screen_create(app_screen_touch_cb);

    // Start application timer for state management and animation
    lv_timer_create(app_timer_cb, FRAME_MS, NULL);

    lvgl_port_unlock();

    Serial.println("=== КОНФІГУРАЦІЯ БЕЗ РОЗРИВІВ ЗАВЕРШЕНА ===");
}

void loop()
{
    delay(5);
}
