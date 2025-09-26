#pragma once

// Application configuration constants
#define TARGET_FPS 30
#define FRAME_MS (1000 / TARGET_FPS)
#define LOADING_DURATION 5000 // 5 seconds
#define IDLE_TIMEOUT 10000    // 10 seconds idle timeout
#define UART_BAUD_RATE 115200
#define DEBUG_INTERVAL 5000 // Debug output every 5 seconds

// Screen dimensions (will be set at runtime)
extern int32_t SCR_W, SCR_H;

// Application states
enum AppState
{
    STATE_LOADING,   // Loading screen with animation
    STATE_MAIN_MENU, // Main menu with 4 training buttons
    STATE_TRAINER_1, // Training module 1
    STATE_TRAINER_2, // Training module 2
    STATE_TRAINER_3, // Training module 3
    STATE_TRAINER_4, // Training module 4
    STATE_CONSOLE    // Debug console window
};

// Global state variables
extern AppState current_state;
extern uint32_t state_start_time;
extern uint32_t last_interaction_time;