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

#include <cstring>
#include "Nfc532.h"

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

PN532_I2C pn532_i2c(Wire);
PN532 nfc532 = PN532(pn532_i2c);

unsigned long timeLastCardRead = 0;
boolean readerDisabled = false;
uint8_t cardState = 0;

void IRAM_ATTR cardDetected() {
  detachInterrupt(NFC_IRQ);
  cardState = 1;
}

Nfc532::Nfc532() {}

void Nfc532::start() {
  // Disable the sleep locks (if any)
  gpio_hold_dis((gpio_num_t) NFC_RST);
  gpio_deep_sleep_hold_dis();

  pinMode(NFC_RST, OUTPUT);

  // Reset the PN532 just to make sure we're in a good state
  digitalWrite(NFC_RST, LOW);
  delay(1000);
  digitalWrite(NFC_RST, HIGH);

  Serial.println("NFC reader ready");
  Wire.setPins(NFC_SDA, NFC_SCL);
  nfc532.begin();

  uint32_t versiondata = nfc532.getFirmwareVersion();
  if (!versiondata) {
      Serial.println("PN532 not found");
  }

  nfc532.SAMConfig();
  nfc532.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
  attachInterrupt(NFC_IRQ, cardDetected, FALLING);
}

void Nfc532::sleep() {
  Serial.println("Sleeping the PN532");

  // Power down
  digitalWrite(NFC_RST, LOW);

  // Make sure we hold the pin state even in deep sleep
  gpio_deep_sleep_hold_en();
  gpio_hold_en((gpio_num_t) NFC_RST);
}

bool Nfc532::read(uint8_t* value) {
  if (readerDisabled) {
    unsigned long current = millis();
    unsigned long delta = current - timeLastCardRead;

    if (delta > DELAY_BETWEEN_CARDS) {
      Serial.println("Re-enabling the reader");

      cardState = 0;
      readerDisabled = false;
      nfc532.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
      attachInterrupt(NFC_IRQ, cardDetected, FALLING);
    }
  } else {
    if (cardState == 1) {
      cardState = 0;
      readerDisabled = true;
      timeLastCardRead = millis();

      uint8_t success;
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
      uint8_t uidLength;

      success = nfc532.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

      if (success) {
        Serial.println("Found an ISO14443A card");

        if (uidLength == 7)
        {
          MifareUltralight ml = MifareUltralight(nfc532);
          NfcTag tag = ml.read(uid, uidLength);
          return processPayload(&tag, value);
        }
      }
    }
  }
  return false;
}

bool Nfc532::processPayload(NfcTag* tag, uint8_t* value) {
  Serial.println(tag->getTagType());
  Serial.print("UID: ");Serial.println(tag->getUidString());

  if (tag->hasNdefMessage())
  {
    NdefMessage message = tag->getNdefMessage();
    int recordCount = message.getRecordCount();

    if (recordCount < 1) return false;

    NdefRecord record = message.getRecord(0);

    int payloadLength = record.getPayloadLength();

    if (payloadLength < 4 || payloadLength > 255) return false;

    byte payload[payloadLength];
    record.getPayload(payload);

    int i = 0;
    for (int j = 3; j < payloadLength; j++) {
      char ch = (char)payload[j];
      
      // Ignore any non expected characters
      if (!(isprint(ch) || isspace(ch)))
      {
        continue;
      }

      value[i++] = ch;
    }
    value[i] = '\0';

    Serial.print("Payload: ");
    Serial.println((char *)value);

    return true;
  }
  return false;
}