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

class Nfc {
public:
  virtual ~Nfc() {}
  virtual void start() = 0;
  virtual void sleep() = 0;
  virtual bool read(uint8_t* value) = 0;
};