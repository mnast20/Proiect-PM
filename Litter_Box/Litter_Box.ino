// C++ code
//
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "hd44780.h"
#include <Servo.h>
#include <RTClib.h>

#define LED 13
#define TRIGGER_PIN 7
#define ECHO_PIN 3
#define BUTTON_STOP 2
#define BUTTON_OK 6
#define BUTTON1 5
#define BUTTON2 4
#define PIR_SENSOR 8
#define BUZZER 9
#define POTENTIOMETER A0

RTC_DS3231 rtc;

DateTime endDateTime, next5Mins;

int buttonState1 = 0;
int buttonState2 = 0;
int buttonStateOK = 0;
int count = 0;
int pirState = -1;
int start = 0;
volatile boolean interruptFlag = false;
int hours = 0, minutes = 0, seconds = 0;
long totalEndSeconds = 0;
int column = 1, columnReset = 1;
int periodChosen = 0;
char periodCharacter = 'd';
int r = 0;
int distance = 0;
int distanceChosen = 0;
int resetStart = 0;
int lastCheck = 0;
int buzz = 0;
int previousBuzzerState = 0;
int buzzerStartTime = 0;

int buttonState[2] = {LOW, LOW};
int pressed[2] = {LOW, LOW}; 
int lastButtonState[2] = {LOW, LOW};

long lastDebounceTime[2] = {0, 0};
long time[2] = {0, 0};
long debounce = 300;

Servo servoMotor;
LiquidCrystal_I2C LCD(0x27,20,4);


// Stop button interrupt function
void buttonInterrupt() {
  Serial.println("Interrupt");
  count = 0;
  start = 0;
  periodChosen = 0;
  distanceChosen = 0;
  column = 1;
  columnReset = 1;
  r = 0;
  resetStart = 1;
  lastCheck = 0;
  buzz = 0;
  previousBuzzerState = 0;
  buzzerStartTime = 0;
  
  interruptFlag = true;
}

// Function clearing LCD
void clearLCD() {
  LCD.setCursor(0, 0);
  LCD.print("                                   ");
  LCD.setCursor(0, 1);
  LCD.print("                                   ");
  LCD.setCursor(0, 2);
  LCD.print("                                   ");
  LCD.setCursor(0, 3);
  LCD.print("                                   ");
}

// Function opening and then closing door
void openCloseDoor() {
  servoMotor.attach(A1);
  servoMotor.write(300);
  delay(9000);
  servoMotor.write(-100);
  delay(10000);
  servoMotor.detach();
}

// Function to add days to a given date
void addDays(DateTime& dateTime, int days) {
    dateTime = dateTime + TimeSpan(days, 0, 0, 0);
}

// Function to add minutes to a given date
void addMinutes(DateTime& dateTime, int minutes) {
    dateTime = dateTime + TimeSpan(0, 0, minutes, 0);
}

// Function to add hours to a given date
void addHours(DateTime& dateTime, int hours) {
    dateTime = dateTime + TimeSpan(0, hours, 0, 0);
}

// Function calculating end time based on chosen period
void getEndTime() {
  DateTime now = rtc.now();
  endDateTime = now;

  if (column == 1) {
    addDays(endDateTime, count);
  } else if (column == 2) {
    addHours(endDateTime, count);
  } else {
    addMinutes(endDateTime, count);
  }

  // calculate time in next 5 minutes
  next5Mins = now;
  addMinutes(next5Mins, 5);
}

// Function searching for movement
int checkMovement() {
  int statePIR = digitalRead(PIR_SENSOR);
  
  if(statePIR == HIGH){
    LCD.setCursor(0, 0);
    LCD.print(" Movement detected! ");
    LCD.setCursor(0, 1);
    LCD.print("Entering sleep mode ");

    digitalWrite(BUZZER, HIGH);
    delay(3000);
    digitalWrite(BUZZER, LOW);

    delay(2000);
    clearLCD();

    return 0;
  }

  return 1;
}

// Function calculating distance
int checkDistance() {
  long duration, distanceMeasured;
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distanceMeasured = (duration / 2) / 29.1;

  Serial.print("Expected distance: ");
  Serial.println(distance);

  if (distanceMeasured >= distance || distanceMeasured <= 0){
    Serial.println("Out of range");

    if (lastCheck) {
      clearLCD();
      LCD.setCursor(0, 0);
      LCD.print("Couldn't find basket");
      LCD.setCursor(0, 1);
      LCD.print("Entering sleep mode ");

      digitalWrite(BUZZER, HIGH);
      delay(3000);
      digitalWrite(BUZZER, LOW);
      delay(2000);

      clearLCD();
    }

    return 0;
  } else {
    digitalWrite(LED, HIGH);
    Serial.print(distanceMeasured);
    Serial.println(" cm");
    return 1;
  }
}

// Function printing time when interval reaches end
void printEmptyTime() {
      LCD.setCursor(0, 0);
    LCD.print("   Time remaining   ");

    LCD.setCursor(0, 1);
    LCD.print("  ");
    LCD.print(0);
    LCD.print(0);
    LCD.print(" : ");
    LCD.print(0);
    LCD.print(0);
    LCD.print(" : ");
    LCD.print(0);
    LCD.print(0);
    LCD.print(" : ");
    LCD.print(0);
    LCD.print(0);
    LCD.print("   ");
}

// Function printing the differnce in time
int printTimeDifference() {
    DateTime now = rtc.now();
    // calculate difference between now and end time
    long difference = endDateTime.unixtime() - now.unixtime();
    long differenceMins5 = next5Mins.unixtime() - now.unixtime();

    // check if 5 minutes passed
    if ((differenceMins5 == 0 || differenceMins5 < 0) && difference > 0) {
      checkDistance();
      buzz = 1;
      next5Mins = now;
      addMinutes(next5Mins, 5);
    }

    // display the difference in hours, minutes, and seconds
    int days = difference / 86400;
    int hours = (difference % 86400) / 3600;
    int minutes = (difference % 3600) / 60;
    int seconds = difference % 60;

    LCD.setCursor(0, 0);
    LCD.print("   Time remaining");

    LCD.setCursor(0, 1);
    LCD.print("  ");
    if (days < 10) {
      LCD.print(0);
    }
    LCD.print(days);
    LCD.print(" : ");

    if (hours < 10) {
      LCD.print(0);
    }
    LCD.print(hours);
    LCD.print(" : ");

    if (minutes < 10) {
      LCD.print(0);
    }
    LCD.print(minutes);
    LCD.print(" : ");

    if (seconds < 10) {
      LCD.print(0);
    }
    LCD.print(seconds);
    LCD.print("   ");

    if (difference == 0 || difference < 0) {
      return 1;
    }

    return 0;
}

// Function checking if button was pressed
void checkButtonPressed(int nrButton) {
  pressed[nrButton] = 0;
    if (
        buttonState[nrButton] == HIGH && millis() - time[nrButton] > debounce) {
        time[nrButton] = millis();
        pressed[nrButton] = 1;
    }
  
  lastButtonState[nrButton] = buttonState[nrButton];
}

// Function for selecting time period
void selectTimePeriod() {
  buttonState[0] = digitalRead(BUTTON1);
  buttonState[1] = digitalRead(BUTTON2);

  checkButtonPressed(0);
  checkButtonPressed(1);
  
  if (pressed[0] == HIGH && pressed[1] == LOW){
    column--;
    if (column == 0) {
      column = 3;
    }
  } else if (pressed[1] == HIGH && pressed[0] == LOW) {
    column++;
    if (column == 4) {
      column = 1;
    }
  }
  
  LCD.setCursor(0, 0);
  LCD.print("Select option below");

  if (column == 1) {
    LCD.setCursor(0, 1);
    LCD.print("        Days  <--   ");
    LCD.setCursor(0, 2);
    LCD.print("       Hours        ");
    LCD.setCursor(0, 3);
    LCD.print("      Minutes       ");
    periodCharacter = 'd';
  } else if (column == 2) {
    LCD.setCursor(0, 1);
    LCD.print("        Days        ");
    LCD.setCursor(0, 2);
    LCD.print("       Hours  <--   ");
    LCD.setCursor(0, 3);
    LCD.print("      Minutes       ");
    periodCharacter = 'h';
  } else {
    LCD.setCursor(0, 1);
    LCD.print("        Days        ");
    LCD.setCursor(0, 2);
    LCD.print("       Hours        ");
    LCD.setCursor(0, 3);
    LCD.print("      Minutes  <--  ");
    periodCharacter = 'm';
  }
}

// Function checking if period was selected
void checkPeriod() {
  if (buttonStateOK == HIGH) {
    clearLCD();
    LCD.setCursor(0, 0);
    LCD.print("Now choose interval");
    periodChosen = 1;
    delay(3000);
    clearLCD();
  }
}

// Function for choosing the bin distance
void chooseBinDistance() {
  LCD.setCursor(0, 0);
  LCD.print(" Choose bin distance  ");
  LCD.setCursor(0, 1);
  LCD.print("        ");
  distance = analogRead(POTENTIOMETER) / 10;
  if (distance < 10) {
    LCD.print("0");
  }
  LCD.print(distance);
  LCD.print(" cm     ");

  if (buttonStateOK == HIGH) {
    clearLCD();
    LCD.setCursor(0, 0);
    distanceChosen = 1;
    LCD.print(" Now choose period  ");
    delay(3000);
    clearLCD();
  }

}

// Function for setting time interval
void setTimeInterval() {
  buttonState[0] = digitalRead(BUTTON1);
  buttonState[1] = digitalRead(BUTTON2);

  checkButtonPressed(0);
  checkButtonPressed(1);
  
  if (pressed[0] == HIGH && pressed[1] == LOW){
    if (count > 0) {
      count--;
    }
  } else if (pressed[1] == HIGH && pressed[0] == LOW) {
    count++;
  }
  
  LCD.setCursor(2, 0);
  LCD.print("You selected ");
  LCD.print(count);
  LCD.print(periodCharacter);
  LCD.print(".");

  LCD.setCursor(1, 1);
  LCD.print("Press OK to start");
}

// Function checking if interval was chosen
void checkStart() {
    if (buttonStateOK == HIGH && count == 0) {
    LCD.setCursor(0, 0);
  	LCD.print("  Please select a   ");
    LCD.setCursor(0, 1);
    LCD.print("  higher interval   ");
    delay(3000);
    clearLCD();
    LCD.clear();
  } else if (buttonStateOK == HIGH && count > 0){
    LCD.setCursor(0, 0);
    LCD.print("  Starting program  ");
    LCD.setCursor(0, 1);
    LCD.print("                    ");
    start = 1;
    delay(3000);
    clearLCD();
    LCD.clear();
    getEndTime();
  }
}

// Function checking for a reset
void checkReset() {
  if (buttonStateOK == HIGH) {
    clearLCD();
    LCD.setCursor(0, 0);
    if (columnReset == 1) {
      distance = 0;
      LCD.print(" Now choose distance  ");
    } else {
      distanceChosen = 1;
      LCD.print(" Now choose period  ");
    }
    resetStart = 0;
    delay(3000);
    clearLCD();
  }
}

// Function handling reset
void reset() {
  buttonState[0] = digitalRead(BUTTON1);
  buttonState[1] = digitalRead(BUTTON2);

  checkButtonPressed(0);
  checkButtonPressed(1);
  
  if (pressed[0] == HIGH && pressed[1] == LOW){
    columnReset--;
    if (columnReset == 0) {
      columnReset = 2;
    }
  } else if (pressed[1] == HIGH && pressed[0] == LOW) {
    columnReset++;
    if (columnReset == 3) {
      columnReset = 1;
    }
  }
  
  LCD.setCursor(0, 0);
  LCD.print("       Reset        ");

  if (columnReset == 1) {
    LCD.setCursor(0, 1);
    LCD.print("   Bin distance  <--");
    LCD.setCursor(0, 2);
    LCD.print("       Period        ");
  } else {
    LCD.setCursor(0, 1);
    LCD.print("    Bin distance    ");
    LCD.setCursor(0, 2);
    LCD.print("       Period  <--   ");
  }
}

// Function controlling buzzer
void controlBuzzer() {
  unsigned long currentMillis = millis();
  Serial.println(currentMillis);
  if (previousBuzzerState == 0) {
      digitalWrite(BUZZER, HIGH);
      buzzerStartTime = currentMillis;
      previousBuzzerState = 1;
  } else {
    if (currentMillis - buzzerStartTime >= 3000) {
      digitalWrite(BUZZER, LOW);
      previousBuzzerState = 0;
      buzz = 0;
    }
  }
}

void setup()
{
  Serial.begin(9600);

  // setup LCD
  LCD.init();
  clearLCD();         
  LCD.backlight();
  Serial.flush();
  
  // setup pins
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);
  pinMode(BUTTON_STOP, INPUT_PULLUP);
  pinMode(PIR_SENSOR, INPUT);
  pinMode(BUZZER, OUTPUT);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  
  // attach interrupt on STOP button
  attachInterrupt(digitalPinToInterrupt(BUTTON_STOP), buttonInterrupt, FALLING);
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  servoMotor.attach(A1);

  // lift servomotor
  // servoMotor.write(-100);
  // delay(9000);
  // servoMotor.detach();
}

void loop()
{
  if (interruptFlag) {
    clearLCD();
    LCD.setCursor(0, 0);
  	LCD.print("  Program stopped   ");
  	LCD.setCursor(0, 1);
  	LCD.print(" Choose reset option ");
    
    delay(5000);
    clearLCD();
    interruptFlag = false;
  }
  
  buttonStateOK = digitalRead(BUTTON_OK);

  if (resetStart) {
    reset();
    checkReset();
  } else {
    if (!distanceChosen) {
      chooseBinDistance();
    } else {
      if (!periodChosen) {
        selectTimePeriod();
        checkPeriod();
      } else {
        if (start == 0) {
          checkStart();
        }
        
        if (start == 0) {
          setTimeInterval();
        }
        
        if (start) {
          if (r == 0) {
            r = printTimeDifference();
          } else {
            printEmptyTime();
          }

          if (!lastCheck && buzz) {
            controlBuzzer();
          }

          if (r == 1) {
            lastCheck = 1;
            int result1 = 0, result2 = 0;

            result1 = checkDistance();

            if (result1) {
            result2 = checkMovement();
            }
      
            if (result1 && result2) {
              openCloseDoor();
              getEndTime();
              r = 0;
              lastCheck = 0;
            }
          }
        }
      }
    }
  }
}