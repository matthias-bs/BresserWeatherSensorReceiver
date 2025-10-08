//////////////////////////////////////////////////////////////////////////////////////////////////
// config.h
//
// Configuration for BresserWeatherSensorSDCard.ino
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 10/2025
//
//
// MIT License
//
// Copyright (c) 2025 Matthias Prinke
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
// 20251008 Created
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// Select one of the external RTC chips supported by Adafruit RTClib (optional)
// https://github.com/adafruit/RTClib
//#define EXT_RTC RTC_DS3231
//#define EXT_RTC RTC_DS1307
//#define EXT_RTC RTC_PCF8523
//#define EXT_RTC RTC_PCF8563

// Filename: <FILENAME_PREFIX><YYYY>-<MM>-<DD>.csv
#define FILENAME_PREFIX "bresser-"

// Ignore if RTC is not set (e.g., due to power loss or reset)
// Otherwise halt if RTC is not set
#define IGNORE_IF_RTC_NOT_SET
