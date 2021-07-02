#include <LiquidCrystal_I2C.h>
#include <virtuabotixRTC.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

virtuabotixRTC myRTC(6, 7, 8); //If you change the wiring change the pins here also

void setup()
{
  // Serial.begin(9600);
  lcd.init();
  lcd.backlight();
}

void loop() {
  char line0[16];
  char line1[16];
  lcd.setCursor(0,1);
  lcd.print("Power: 200W 100%");
  myRTC.updateTime();
  lcd.setCursor(0,0);
  sprintf(line0, "%.2d:%.2d:%.2d", myRTC.hours, myRTC.minutes, myRTC.seconds);
  lcd.print(line0);
  delay(1000);
}
