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

#include "Ble.h"
#include "NimBLECharacteristic.h"
#include "NimBLEConnInfo.h"
#include <NimBLEDevice.h>
#include "NimBLEHIDDevice.h"
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

static const uint8_t hidReportDescriptor[] PROGMEM = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x05,        // Usage (Game Pad)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x05, 0x09,        //   Usage Page (Button)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x02,        //   Usage Maximum (0x02)
  0x95, 0x02,        //   Report Count (2)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x06,        //   Report Count (6)
  0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xA1, 0x00,        //   Collection (Physical)
  0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
  0x09, 0x39,        //     Usage (Hat switch)
  0x15, 0x01,        //     Logical Minimum (1)
  0x25, 0x08,        //     Logical Maximum (8)
  0x35, 0x00,        //     Physical Minimum (0)
  0x46, 0x3B, 0x01,  //     Physical Maximum (315)
  0x65, 0x12,        //     Unit (System: SI Rotation, Length: Centimeter)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x01,        //     Report Count (1)
  0x81, 0x42,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
  0xC0,              //   End Collection
  0xC0,              // End Collection
}; // 55 bytes

// HID Report
typedef struct {
  uint8_t buttons;
  uint8_t hat;
} HidReport;

HidReport hidReport;

NimBLEServer *pServer;
NimBLEHIDDevice *pHid;
NimBLECharacteristic *pGamepad;
NimBLECharacteristic *pNfc;

bool connected = false;

Ble::Ble(void) {
} 

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
      Serial.println("BLE connected");
      pServer->updateConnParams(connInfo.getConnHandle(), 6, 7, 0, 600);
      connected = true;
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
      Serial.println("BLE diconnected");
      connected = false;
    }
} serverCallbacks;

bool Ble::isConnected() {
  return connected;
}

void Ble::start() {
  NimBLEDevice::init(BLE_NAME);
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);

  pHid = new NimBLEHIDDevice(pServer);
  pGamepad = pHid->getInputReport(1);
  pHid->setManufacturer(BLE_MANUFACTURER);
  pHid->setPnp(BLE_SIG, BLE_VID, BLE_PID, BLE_VERSION);
  pHid->setHidInfo(0x00, 0x01);

  NimBLEDevice::setSecurityAuth(true /* bonding */, false /* MITM */, false /* SC */);
  
  pHid->setReportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));

  NimBLEService* pNfcService = pServer->createService(NimBLEUUID(BLE_SERVICE_UUID));
  pNfc = pNfcService->createCharacteristic(NimBLEUUID(BLE_CHARACTERISTIC_UUID),
                                                  NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  pNfc->setValue("none");

  pHid->startServices();
  pNfcService->start();

  NimBLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_GAMEPAD);
  pAdvertising->addServiceUUID(pHid->getHidService()->getUUID());
  pAdvertising->addServiceUUID(pNfcService->getUUID());
  pAdvertising->start();                                                                             
                                                                                                      
  pHid->setBatteryLevel(100);

  Serial.println("BLE started");
}

void Ble::sendNfcCommand(String payload) {
  pNfc->setValue((uint8_t*) payload.c_str(), payload.length());
  pNfc->notify();
}

void Ble::sendGamepadUpdate(bool b1, bool b2, uint8_t hat) {
  hidReport.buttons = b1 | b2<<1;
  hidReport.hat = hat;
  pGamepad->setValue((uint8_t*) &hidReport, 2);
  pGamepad->notify();

  #ifdef DEBUG
    Serial.print("Sending ");
    Serial.print(hidReport.buttons);
    Serial.print(" : ");
    Serial.print(hidReport.hat);
    Serial.print(" : ");
    printJoystickDebug(prevHat);
  #endif
}
