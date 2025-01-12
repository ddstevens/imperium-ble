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

#pragma once

// ESP32S3
#define MCU_S3
#define GP_B1     2
#define GP_B2     3
#define GP_UP     7
#define GP_RIGHT  4
#define GP_DOWN   6
#define GP_LEFT   5
#define NFC_SDA   8
#define NFC_SCL   9
#define NFC_IRQ   10
#define NFC_RST   11

#define BLE_NAME "ImperiumBLE"
#define BLE_MANUFACTURER "Imperium"
#define BLE_SIG 0x01
#define BLE_VID 0x2301
#define BLE_PID 0xCDAB
#define BLE_VERSION 0x0001
#define BLE_SERVICE_UUID "CE00299E-EA4B-4BB6-B631-A93F4F16E71B"
#define BLE_CHARACTERISTIC_UUID "8CD024AE-4EA5-4F06-9836-D5CA72976A40"

// Debounce counter max. How many checks after a button is released before sending the release.
#define DBC_MAX 32 // Set to 1 to disable