#include <Arduino.h>

// Include all controller and settings headers
#include "Constants.h"
#include "Debug.h"
#include "Settings.h"
#include "TimeController.h"
#include "DisplayController.h"
#include "InputManager.h"
#include "LightingController.h"
#include "UIManager.h"

// Global objects for our controllers
Settings settings;
TimeController timeController(RTC_CLK_PIN, RTC_DAT_PIN, RTC_RST_PIN);
DisplayController displayController(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
InputManager inputManager;
InputProcessor inputProcessor(inputManager);
LightingController lightingController;
UIManager uiManager(displayController, timeController, lightingController, inputProcessor, settings);

void setup() {
    Serial.begin(9600);

    // Initialize hardware and load settings
    displayController.begin();
    timeController.begin();
    settings.load();
    
    displayController.print(0, 0, "Aquarium Ctrl PRO");
    displayController.print(0, 1, "Stabilizing...");
    
    // Wait for hardware to stabilize before activating power components
    delay(1000);
    lightingController.begin(); // Safely turn on the 1-10V transformer
    
    delay(500); // Visual pause
    displayController.clear();
}

void loop() {
    // Get current time once per loop
    time_t utc_now = timeController.nowUTC();
    time_t local_now = timeController.toLocal(utc_now, settings);
    
    // Update all controllers
    lightingController.update(local_now, settings);
    displayController.update();
    uiManager.update();
}