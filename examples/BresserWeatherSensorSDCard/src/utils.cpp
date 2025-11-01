//////////////////////////////////////////////////////////////////////////////////////////////////
// utils.cpp
//
// Utilities for BresserWeatherSensorSDCard.ino
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
// 20251101 Added M5Stack Core2 support
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Preferences.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include "utils.h"
#include "../config.h"

#if defined(EXT_RTC)
// Adafruit RTClib - https://github.com/adafruit/RTClib
#include <RTClib.h>

// Create an instance of the external RTC class
EXT_RTC ext_rtc; //<! External RTC instance
#endif

#if defined(ARDUINO_M5STACK_CORE2)
#include <M5Unified.h>
#endif

#if defined(ARDUINO_M5STACK_CORE2)
void setupM5StackCore2(void)
{
  auto cfg = M5.config();
  cfg.clear_display = true; // default=true. clear the screen when begin.
  cfg.external_display.module_display = false; // default=true. use Module Display.
  cfg.output_power = true;  // default=true. use external port 5V output.
  cfg.internal_imu = false; // default=true. use internal IMU.
  cfg.internal_rtc = true;  // default=true. use internal RTC.
  cfg.internal_spk = false; // default=true. use internal speaker.
  cfg.internal_mic = false; // default=true. use internal microphone.
  M5.begin(cfg);
  M5.Display.setBrightness(0); // set system LED brightness (0=off / 255=max)
}
#endif

/**
 * @brief Convert unix time to ISO 8601 format
 * 
 * @param buffer string buffer to hold the result
 * @param len maximum length of the buffer
 * @param t unix time to convert
 */
void unixtime_to_iso8601(char *buffer, size_t len, time_t t)
{
    time_t now = time(nullptr);
    struct tm *tm_info = localtime(&now);

    // Print the RTC time in ISO 8601 format
    strftime(buffer, len, "%Y-%m-%dT%H:%M:%S", tm_info);
}

/**
 * @brief Convert time and date strings to time_t
 * 
 * @param time Time string in the format HH:MM:SS (default: __TIME__)
 * @param date Date string in the format "MMM DD YYYY" (default: __DATE__)
 * @return time_t unix time
 */
time_t convert_time(char const *time = __TIME__, char const *date = __DATE__)
{
    char s_month[5];
    int month;
    int year;
    struct tm t = {
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0,
        .tm_wday = 0,
        .tm_yday = 0,
        .tm_isdst = 0
    };
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf(date, "%s %d %d", s_month, &t.tm_mday, &year);
    sscanf(time, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);

    month = (strstr(month_names, s_month) - month_names) / 3;

    t.tm_mon = month;
    t.tm_year = year - 1900;
    t.tm_isdst = -1;

    return mktime(&t);
}

#if !defined(EXT_RTC) && !defined(ARDUINO_M5STACK_CORE2)
/**
 * @brief Set internal RTC from compile time
 * 
 * The time is stored in Preferences to check if the RTC was set before.
 * If the stored time is older than the compile time or the time was not stored yet,
 * the RTC is set to the compile time and the compile time is stored.
 * 
 * Otherwise, the RTC is kept running as is.
 */
void set_rtc(void)
{
    Preferences rtcPrefs;
    
    time_t compiled_at = convert_time();

    rtcPrefs.begin("SDCARD-RTC", false);
    time_t stored_at = rtcPrefs.getULong("time", 0);
    log_d("Stored at: %llu", stored_at);
    log_d("Compiled at: %llu", compiled_at);

    log_w("Using internal RTC - setting will be lost on power fail/power off/reset");
    if (stored_at == 0) {
        log_d("No stored RTC time found");
    } else {
        log_d("Stored RTC time found");
        if (stored_at < compiled_at)
        {
            log_d("Stored RTC time is older than compile time.");
        }
    }

    if (stored_at < compiled_at) {
        log_d("Setting RTC to compile time");
        rtcPrefs.putULong("time", compiled_at);
        struct timeval tv = {.tv_sec = compiled_at, .tv_usec = 0};
        settimeofday(&tv, NULL);

        // Wait until time is set
        for (int i = 0; i < 20; i++) {
            if (!(time(nullptr) < compiled_at)) {
                break;
            }
            delay(10);
        }
    }
    rtcPrefs.end();
}
#endif

#if defined(EXT_RTC)
// Synchronize the internal RTC with the external RTC
void syncRTCWithExtRTC(void)
{
    DateTime now = ext_rtc.now();

    // Convert DateTime to time_t
    struct tm timeinfo;
    timeinfo.tm_year = now.year() - 1900;
    timeinfo.tm_mon = now.month() - 1;
    timeinfo.tm_mday = now.day();
    timeinfo.tm_hour = now.hour();
    timeinfo.tm_min = now.minute();
    timeinfo.tm_sec = now.second();

    time_t t = mktime(&timeinfo);

    // Set the MCU's internal RTC (ESP32) or SW RTC (RP2040)
    struct timeval tv = {t, 0}; // `t` is seconds, 0 is microseconds
    settimeofday(&tv, nullptr);
}
#endif // EXT_RTC

#if defined(EXT_RTC)
// Get the time from external RTC
void set_rtc(void)
{
    if (!ext_rtc.begin())
    {
        log_w("External RTC not available");
    }
    else if (ext_rtc.lostPower())
    {
        log_w("External RTC lost power");
    }
    else
    {
        syncRTCWithExtRTC();
        log_i("Set time and date from external RTC");
    }
}
#endif

#if defined(ARDUINO_M5STACK_CORE2)
// Synchronize the internal RTC with the external RTC
void syncRTCWithExtRTC(void)
{
  auto dt = M5.Rtc.getDateTime();

  // Convert DateTime to time_t
  struct tm timeinfo;
  timeinfo.tm_year = dt.date.year - 1900;
  timeinfo.tm_mon = dt.date.month - 1;
  timeinfo.tm_mday = dt.date.date;
  timeinfo.tm_hour = dt.time.hours;
  timeinfo.tm_min = dt.time.minutes;
  timeinfo.tm_sec = dt.time.seconds;

  time_t t = mktime(&timeinfo);

  // Set the MCU's internal RTC (ESP32) or SW RTC (RP2040)
  struct timeval tv = {t, 0}; // `t` is seconds, 0 is microseconds
  settimeofday(&tv, nullptr);
}
#endif // ARDUINO_M5STACK_CORE2

#if defined(ARDUINO_M5STACK_CORE2)
// Get the time from M5Stack Core2 RTC
void set_rtc(void)
{
  if (!M5.Rtc.begin())
  {
    log_w("M5 RTC not available");
  }
  else if (M5.Rtc.getVoltLow())
  {
    log_w("M5 RTC lost power");
  }
  else
  {
    syncRTCWithExtRTC();
    log_i("Set time and date from RTC IC");
  }
}
#endif // ARDUINO_M5STACK_CORE2

#if !defined(ARDUINO_M5STACK_CORE2)
void initLed(void)
{
    // LED for indicating failure or SD card activity
    // turn LED on (full brightness)
    // LED for indicating failure or SD card activity
    pinMode(LED_BUILTIN, OUTPUT);
}
#else
void initLed(void)
{
    // LED for indicating failure or SD card activity
    // turn LED off
    M5.Power.setLed(0); // off
}
#endif

#if !defined(ARDUINO_M5STACK_CORE2)
void setLed(bool on)
{
    if (on)
    {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
}
#else
void setLed(bool on)
{
    if (on)
    {
        M5.Power.setLed(255); // on
    }
    else
    {
        M5.Power.setLed(0); // off
    }
}
#endif