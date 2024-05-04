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
// Copyright (c) 2022 Matthias Prinke
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
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "InitBoard.h"

#if defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_Core2)
// Note: Depending on the environment, both variants are used!
#include <M5Unified.h>
#endif
#if defined(ARDUINO_ESP32S3_POWERFEATHER)
#include <PowerFeather.h>
using namespace PowerFeather;
#endif

void initBoard(void)
{
#if defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_Core2)
    // Note: Depending on the environment, both variants are used!
    auto cfg = M5.config();
    cfg.clear_display = true; // default=true. clear the screen when begin.
    cfg.output_power = true;  // default=true. use external port 5V output.
    cfg.internal_imu = false; // default=true. use internal IMU.
    cfg.internal_rtc = true;  // default=true. use internal RTC.
    cfg.internal_spk = false; // default=true. use internal speaker.
    cfg.internal_mic = false; // default=true. use internal microphone.
    M5.begin(cfg);
#endif
#if defined(ARDUINO_ESP32S3_POWERFEATHER)
    Board.init();
    // Enable power supply for Adafruit LoRa Radio FeatherWing
    Board.enable3V3(true);
#endif
}
