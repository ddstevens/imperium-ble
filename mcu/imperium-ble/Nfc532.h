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
#include <NfcAdapter.h>
#include "Nfc.h"

class Nfc532 : public Nfc {
public:
  Nfc532();
  void start();
  void sleep();
  bool read(uint8_t* value);
private:
  bool processPayload(NfcTag* tag, uint8_t* value);
};