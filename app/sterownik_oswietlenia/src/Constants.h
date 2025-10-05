#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

// Pin Definitions
const uint8_t BUTTON_RIGHT_PIN = 12;
const uint8_t BUTTON_SET_PIN = 11;
const uint8_t BUTTON_MINUS_PIN = 10;
const uint8_t BUTTON_PLUS_PIN = 9;

const uint8_t RTC_CLK_PIN = 6;
const uint8_t RTC_DAT_PIN = 7;
const uint8_t RTC_RST_PIN = 8;

const uint8_t VOLTAGE_FEEDBACK_PIN = A2;
const uint8_t VOLTAGE_OUTPUT_PIN = 5;

const uint8_t SWITCH_TRANSFORMER_PIN = 2;
const uint8_t SWITCH_BALLAST_1_PIN = 3;
const uint8_t SWITCH_BALLAST_2_PIN = 4;
const uint8_t SWITCH_BALLAST_3_PIN = A3;

// I2C LCD Configuration
const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 2;

// EEPROM Addresses
const int EEPROM_TIMER_START_HOUR_ADDR = 0;
const int EEPROM_TIMER_START_MINUTE_ADDR = 1;
const int EEPROM_TIMER_STOP_HOUR_ADDR = 2;
const int EEPROM_TIMER_STOP_MINUTE_ADDR = 3;
const int EEPROM_TIMEZONE_ADDR = 4;

// Behavior Constants
const unsigned long ACTIVITY_BACKLIGHT_SECONDS = 60;
const unsigned long BUTTON_DEBOUNCE_DELAY = 50; // ms
const unsigned long SEQUENTIAL_SWITCH_DELAY_MS = 1000; // 1 second between switching ballasts

// Lighting Control
const int ANALOG_READ_RESOLUTION = 1023;
const int ANALOG_WRITE_RESOLUTION = 255;

// PID-like controller for voltage regulation
const float VOLTAGE_KP = 1.0f; // Proportional gain for voltage adjustment

// UI Constants
const int MAIN_MENU_SIZE = 4;

#endif // CONSTANTS_H