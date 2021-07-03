#include <LiquidCrystal_I2C.h>
#include <virtuabotixRTC.h>
#include <time.h>

const int buttonRightPin = 12;
const int buttonSetPin = 11;
const int buttonMinusPin = 10;
const int buttonPlusPin = 9;
const int screenSelectSize = 3;
const int activityBacklightSeconds = 60;

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
virtuabotixRTC myRTC(6, 7, 8); //If you change the wiring change the pins here also
int screenSelectCurrent = 0;
int buttonRightState = LOW;
int buttonSetState = LOW;
int buttonMinusState = LOW;
int buttonPlusState = LOW;
time_t lastActivity = 0;
bool backlightLast = false;

void setup()
{
  // Serial.begin(9600);
  lcd.init();
  setLastActivity();
  setScreenBacklight();
}

void loop() {
  setButton();
  screenSelect();
  setScreenBacklight();
  switch (screenSelectCurrent) {
    case 0:
      screenInfo();
      break;
    case 1:
      screenDate();
      break;
    case 2:
      screenBrightness();
      break;
  }
  delay(100);
}

void screenInfo() {
  struct tm * timeinfo = getCurrentTm();
  char time[9];
  char date[11];
  char line[2][16];
  strftime(time, 9, "%H:%M:%S", timeinfo);
  lcd.setCursor(0, 0);
  sprintf(line[0], "Time: %s", time);
  lcd.print(line[0]);
  lcd.setCursor(0, 1);
  lcd.print("Power: 200W 100%");
}

void screenBrightness() {
  lcd.setCursor(0, 0);
  lcd.print("Brightness");
}

void screenDate() {
  struct tm * timeinfo = getCurrentTm();
  char time[9];
  char date[11];
  char line[2][16];

  strftime(time, 9, "%H:%M:%S", timeinfo);
  strftime(date, 11, "%Y/%m/%d", timeinfo);

  lcd.setCursor(0, 0);
  sprintf(line[0], "Time: %s", time);
  lcd.print(line[0]);
  lcd.setCursor(0, 1);
  sprintf(line[1], "Date: %s", date);
  lcd.print(line[1]);

  if (buttonSetState == HIGH) {
    setScreenDate();
  }
}

void adjustTimeDate(struct tm * timeinfo, int hour, int minute, int second, int year, int month, int day) {
  myRTC.setDS1302Time(timeinfo->tm_sec+second, timeinfo->tm_min+minute, timeinfo->tm_hour+hour, 0, timeinfo->tm_mday+day, timeinfo->tm_mon+month+1, timeinfo->tm_year+year+1900);
}

void setScreenDate() {
  char time[9];
  char date[11];
  char line[2][16];
  char timeFormat[] = "%H:%M:%S";
  char dateFormat[] = "%Y/%m/%d";
  bool display = true;
  int selectPos = 0;
  int menuSize = 6;
  struct tm * timeinfo;
  do {
    setButton();
    timeinfo = getCurrentTm();
    strftime(time, 9, timeFormat, timeinfo);
    strftime(date, 11, dateFormat, timeinfo);

    if (buttonRightState == HIGH) {
      selectPos += 1;
      if (selectPos >= menuSize) {
        selectPos = 0;
      }
    }

    switch (selectPos) {
      case 0:
        if (!display) strftime(time, 9, "  :%M:%S", timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(timeinfo, 1, 0, 0, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(timeinfo, -1, 0, 0, 0, 0, 0);
        break;
      case 1:
        if (!display) strftime(time, 9, "%H:  :%S", timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(timeinfo, 0, 1, 0, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(timeinfo, 0, -1, 0, 0, 0, 0);
        break;
      case 2:
        if (!display) strftime(time, 9, "%H:%M:  ", timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(timeinfo, 0, 0, 1, 0, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(timeinfo, 0, 0, -1, 0, 0, 0);
        break;
      case 3:
        if (!display) strftime(date, 11, "    /%m/%d", timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(timeinfo, 0, 0, 0, 1, 0, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(timeinfo, 0, 0, 0, -1, 0, 0);
        break;
      case 4:
        if (!display) strftime(date, 11, "%Y/  /%d", timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(timeinfo, 0, 0, 0, 0, 1, 0);
        if (buttonMinusState == HIGH) adjustTimeDate(timeinfo, 0, 0, 0, 0, -1, 0);
        break;
      case 5:
        if (!display) strftime(date, 11, "%Y/%m/  ", timeinfo);
        if (buttonPlusState == HIGH) adjustTimeDate(timeinfo, 0, 0, 0, 0, 0, 1);
        if (buttonMinusState == HIGH) adjustTimeDate(timeinfo, 0, 0, 0, 0, 0, -1);
        break;
    }

    if (display) {
      display = false;
    } else {
      display = true;
    }

    lcd.setCursor(0, 0);
    sprintf(line[0], "Time: %s", time);
    lcd.print(line[0]);
    lcd.setCursor(0, 1);
    sprintf(line[1], "Date: %s", date);
    lcd.print(line[1]);
    delay(100);
  } while (buttonSetState == LOW);
}

void setButton() {
  buttonRightState = getDigitalButton(buttonRightPin);
  buttonSetState = getDigitalButton(buttonSetPin);
  buttonMinusState = getDigitalButton(buttonMinusPin);
  buttonPlusState = getDigitalButton(buttonPlusPin);
}

void screenSelect() {
  if (buttonRightState == HIGH) {
    // Serial.println("+");
    screenSelectCurrent += 1;
  }
  if (screenSelectCurrent >= screenSelectSize) {
    screenSelectCurrent = 0;
  }
}

bool getDigitalButton(int pin) {
  int state = digitalRead(pin);
  if (state == HIGH) {
    setLastActivity();
    int afterState = HIGH;
    while (afterState == HIGH) {
      afterState = digitalRead(pin);
      delay(10);
    }
  }
  if (!backlightLast) {
    return LOW;
  }
  return state;
}

struct tm * getCurrentTm() {
  time_t tmpNow = 0;
  struct tm * timeinfo = localtime(&tmpNow);
  myRTC.updateTime();
  timeinfo->tm_year = myRTC.year - 1900;
  timeinfo->tm_mon = myRTC.month - 1;
  timeinfo->tm_mday = myRTC.dayofmonth;
  timeinfo->tm_hour = myRTC.hours;
  timeinfo->tm_min = myRTC.minutes;
  timeinfo->tm_sec = myRTC.seconds;
  timeinfo->tm_isdst = -1;
  return timeinfo;
}

time_t getCurrentTimestamp() {
  struct tm * timeinfo = getCurrentTm();
  return mktime(timeinfo);
}

void setLastActivity() {
  lastActivity = getCurrentTimestamp();
}

void setScreenBacklight() {
  bool backlight;
  time_t now = getCurrentTimestamp();
  if (difftime(now, lastActivity) <= activityBacklightSeconds ) {
    backlight = true;
  } else {
    backlight = false;
  }
  if (backlightLast != backlight) {
    backlightLast = backlight;
    if (backlightLast) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
  }
}
