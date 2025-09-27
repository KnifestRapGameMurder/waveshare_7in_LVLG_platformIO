#ifndef CONSOLE_H
#define CONSOLE_H

/**
 * @file console.h
 * @brief Public interface for the LVGL-based display console.
 */

// Initializes the console screen and all its LVGL objects.
void console_init();

// Adds a new, timestamped message to the log display.
void console_log(const char *message);

// Updates the text in the top status bar.
void console_set_status(const char *status);

// Handles periodic updates, like the clock. Must be called in the main loop().
void console_update();

#endif // CONSOLE_H