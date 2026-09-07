///////////////////////////////////////////////////////////////////////////////////////////////////
// InitBoard.cpp
//
// Board specific initialization
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 05/2024
//
//
// MIT License
//
// Copyright (c) 2026 Matthias Prinke
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// History:
//
// 20240504 Created
// 20260826 Added ARDUINO_ESP32S3_POWERFEATHER_V2
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "InitBoard.h"

#if defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_Core2)
#include <Wire.h>
#endif
#if defined(ARDUINO_ESP32S3_POWERFEATHER) || defined(ARDUINO_ESP32S3_POWERFEATHER_V2)
#include <PowerFeather.h>
using namespace PowerFeather;
#endif

void initBoard(void)
{
#if defined(ARDUINO_M5STACK_CORE2)
    Wire.begin(32, 33, 400000U);
    Wire.beginTransmission(0x34);
    Wire.write(0x90); // AXP192 GPIO0: external 5 V output.
    Wire.write(0x02); // Enable external 5 V output.
    Wire.endTransmission();
#endif
#if defined(ARDUINO_ESP32S3_POWERFEATHER) || defined(ARDUINO_ESP32S3_POWERFEATHER_V2)
    Board.init();
    // Enable power supply for Adafruit LoRa Radio FeatherWing
    Board.enable3V3(true);
#endif
}
