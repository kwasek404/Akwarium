#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// To disable all debug messages, comment out the line below.
// #define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) { char buf[128]; snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); Serial.print(buf); }
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
#endif

#endif // DEBUG_H