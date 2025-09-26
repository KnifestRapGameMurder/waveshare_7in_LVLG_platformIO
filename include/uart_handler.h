#pragma once
#include <Arduino.h>

// UART communication functions
void uart_handler_init();
void uart_handler_process();
void handle_slave_uart_data();

// UART callback functions
void uart_response_callback(uint8_t cmd, uint8_t *data, uint8_t len);
void uart_error_callback(uint8_t error_code, const char *message);