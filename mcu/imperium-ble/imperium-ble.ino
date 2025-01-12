/*
imperium-ble - BLE gamepad with NFC support
Copyright (C) 2025  Doug Stevens

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include "Config.h"
#include "Ble.h"
#include "Nfc532.h"

#define DEBUG

#define NUM_INPUTS 6

const int8_t inputPin[NUM_INPUTS] = {GP_B1, GP_B2, GP_UP, GP_RIGHT, GP_DOWN, GP_LEFT};
const int16_t inputPinMask[NUM_INPUTS] = {1<<GP_B1, 1<<GP_B2, 1<<GP_UP, 1<<GP_RIGHT, 1<<GP_DOWN, 1<<GP_LEFT};

// Debounce counter for each input
uint8_t input[NUM_INPUTS]  = {
  DBC_MAX, DBC_MAX, DBC_MAX, DBC_MAX, DBC_MAX, DBC_MAX
};

char *directionLabels[9] = {"Centered", "Up", "Up Right", "Right", "Down Right", "Down", "Down Left", "Left", "Up Left"};
enum direction { CENTERED, UP, UP_RIGHT, RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT, LEFT, UP_LEFT };

RTC_DATA_ATTR int bootCount = 0;
uint32_t lastSendTime = 0;

Ble ble;
Nfc* nfc = new Nfc532();

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup()
{
  Serial.begin(9600);
  delay(200);
  Serial.println(BLE_NAME);

  ++bootCount;

  #ifdef DEBUG
    print_wakeup_reason();
  #endif

  // Setup the wakeup pin
  #ifdef MCU_S3
    esp_sleep_enable_ext0_wakeup((gpio_num_t) GP_B1, 0);
  #endif
  #ifdef MCU_C3
    esp_deep_sleep_enable_gpio_wakeup(BIT(GP_B1), ESP_GPIO_WAKEUP_GPIO_LOW);
  #endif

  ble.start();
  nfc->start();

  // Setup joystick and buttons
  for (uint8_t i = 0; i < NUM_INPUTS; i++) {
    pinMode(inputPin[i], INPUT_PULLUP);
  }

  lastSendTime = millis();
}

// Joystick and button state
bool prevB1 = false;
bool prevB2 = false;
uint8_t prevHat = CENTERED;
bool press[NUM_INPUTS] = { false, false, false, false, false, false };
bool sendReport = false;

uint8_t command[256];

void loop()
{
  // Sleep after 5 minutes of inactivity
  if ((millis() - lastSendTime) >= INACTIVITY_BEFORE_SLEEP) {
    Serial.println("Going to sleep now");
    nfc->sleep();
    delay(2000);
    esp_deep_sleep_start();
  }

  // Don't bother doing anything else if we're not connected
  if (!ble.isConnected()) return;

  // Check to see if there is a command found
  if (nfc->read(command)) {
    ble.sendNfcCommand((char*)command);
  }

  uint32_t gpio = REG_READ(GPIO_IN_REG); 

  // Read / debounce inputs (joysticks and buttons)  
  for (uint8_t i = 0; i < NUM_INPUTS; i++) {    
    if (!(gpio & inputPinMask[i])) {
      if (input[i] == DBC_MAX) {
        press[i] = true;
      }
      input[i] = 0;
    }
    else if (input[i] < DBC_MAX) {
      input[i]++;
      if (input[i] == DBC_MAX) {
        press[i] = false;
      }
    }
  }

  // Button 1
  if (press[0] != prevB1) {
    sendReport = true;
    prevB1 = press[0];

    #ifdef DEBUG
      printButtonDebug(1, prevB1);
    #endif
  }

  // Button 2
  if (press[1] != prevB2) {
    sendReport = true;
    prevB2 = press[1];

    #ifdef DEBUG
      printButtonDebug(2, prevB2);
    #endif
  }

  // Joystick
  uint8_t newHat = CENTERED;
  if (press[2] && press[3]) {
    newHat = UP_RIGHT;
  } else if (press[3] && press[4]) {
    newHat = DOWN_RIGHT;
  } else if (press[4] && press[5]) {
    newHat = DOWN_LEFT;
  } else if (press[5] && press[2]) {
    newHat = UP_LEFT;
  } else if (press[2]) {
    newHat = UP;
  } else if (press[3]) {
    newHat = RIGHT;
  } else if (press[4]) {
    newHat = DOWN;
  } else if (press[5]) {
    newHat = LEFT;
  }

  if (prevHat != newHat) {
    sendReport = true;
    prevHat = newHat;

    #ifdef DEBUG
      printJoystickDebug(newHat);
    #endif
  }

  if (sendReport) {
    if (millis() - lastSendTime < 1) return; // Don't send it more than 1000 times a second
    
    ble.sendGamepadUpdate(prevB1, prevB2, prevHat);

    lastSendTime = millis();
    sendReport = false;
  }
}

void printButtonDebug(uint8_t i, bool state) {
    Serial.print("Button ");
    Serial.print(i);
    if (state) Serial.println(" pressed");
    else Serial.println(" released");
}

void printJoystickDebug(uint8_t state) {
    Serial.print("Joystick ");
    Serial.println(directionLabels[state]);
}