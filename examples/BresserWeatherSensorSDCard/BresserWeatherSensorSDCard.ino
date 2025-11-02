//////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorSDCard.ino
//
// Example for BresserWeatherSensorReceiver
//
// This sketch logs the received weather sensor values to an SD Card with FAT32 file system.
// See https://github.com/espressif/arduino-esp32/tree/master/libraries/SD for more information.
//
// Notes:
// - Currently only some boards by LILYGO are supported.
// - SD card must be formatted with FAT32 file system
// - The on-board LED is used to indicate SD card activity (short flash) and failure (permanent on)
// - The internal RTC of the ESP32 is used to create date-based CSV log files.
//   This RTC is set to the compile time after flashing the sketch. The time will be lost after
//   power off (or power failure) or reset. The internal RTC is also not very accurate.
// - If an external RTC is connected (see config.h), the internal RTC is synchronized with it at startup.
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
// 20251010 Added CSV header line
//          Added timezone support
//          Refactored
// 20251011 Modified timestamp handling
//          Adjusted sleep duration to achieve constant wakeup interval
//          Added delay between LED on and SD write to avoid unwanted removal of SD card
// 20251101 Added M5Stack Core2 support
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// To Do:
// - Synchronize time

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "config.h"
#include "src/utils.h"

// Wake up interval [s]
const uint32_t wakeupInterval = WAKEUP_INTERVAL_SEC;

// Create weather sensor receiver object
WeatherSensor ws;

#if defined(ARDUINO_TTGO_LoRa32_V2) || \
    defined(ARDUINO_TTGO_LoRa32_v21new)
static const uint8_t sd_sck = SD_SCK;
static const uint8_t sd_miso = SD_MISO;
static const uint8_t sd_mosi = SD_MOSI;
static const uint8_t sd_cs = SD_CS;
#elif defined(ARDUINO_LILYGO_T3S3_SX1262) || \
    defined(ARDUINO_LILYGO_T3S3_SX1276) ||   \
    defined(ARDUINO_LILYGO_T3S3_LR1121)
static const uint8_t sd_sck = SCK;
static const uint8_t sd_miso = MISO;
static const uint8_t sd_mosi = MOSI;
static const uint8_t sd_cs = SS;
#elif defined(ARDUINO_M5STACK_CORE2)
// See https://docs.m5stack.com/en/core/core2
static const uint8_t sd_sck = SCK;
static const uint8_t sd_miso = MISO;
static const uint8_t sd_mosi = MOSI;
static const uint8_t sd_cs = 4;
#else
#pragma message("Board not supported yet!")
#endif

/**
 * @brief Receive sensor data and build log entry
 *
 * @return String log entry (CSV format) or empty string if no data received
 */
String receiveSensorData()
{
    // Clear sensor data buffer
    ws.clearSlots();

    String logEntry;

    logEntry.reserve(128);
    logEntry = "";

// Attempt to receive data set with timeout of <xx> s
#if (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO)
    bool decode_ok = ws.getData(RX_TIMEOUT, RX_FLAGS, 0, nullptr);
#else
    ws.getData(RX_TIMEOUT, RX_FLAGS, 0, nullptr);
#endif
    log_i("decode_ok: %d", decode_ok);

    for (size_t i = 0; i < ws.sensor.size(); i++)
    {
        if (!ws.sensor[i].valid)
            continue;

        if ((ws.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
            (ws.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
            (ws.sensor[i].s_type == SENSOR_TYPE_WEATHER2))
        {
            // Any other (weather-like) sensor is very similar
            if (ws.sensor[i].w.temp_ok)
            {
                log_i("Temp: %5.1fC |", ws.sensor[i].w.temp_c);
                logEntry += String(ws.sensor[i].w.temp_c, 1);
            }
            else
            {
                log_i("Temp: ---.-C");
            }
            logEntry += ",";
            if (ws.sensor[i].w.humidity_ok)
            {
                log_i("Hum: %3d%%", ws.sensor[i].w.humidity);
                logEntry += String(ws.sensor[i].w.humidity);
            }
            else
            {
                log_i("Hum: ---%%");
            }
            logEntry += ",";
            if (ws.sensor[i].w.wind_ok)
            {
                log_i("Wmax: %4.1fm/s | Wavg: %4.1fm/s | Wdir: %5.1fdeg",
                      ws.sensor[i].w.wind_gust_meter_sec,
                      ws.sensor[i].w.wind_avg_meter_sec,
                      ws.sensor[i].w.wind_direction_deg);
                logEntry += String(ws.sensor[i].w.wind_gust_meter_sec, 1) + ",";
                logEntry += String(ws.sensor[i].w.wind_avg_meter_sec, 1) + ",";
                logEntry += String(ws.sensor[i].w.wind_direction_deg, 1);
            }
            else
            {
                log_i("Wmax: --.-m/s | Wavg: --.-m/s | Wdir: ---.-deg");
            }
            logEntry += ",";
            if (ws.sensor[i].w.rain_ok)
            {
                log_i("Rain: %7.1fmm",
                      ws.sensor[i].w.rain_mm);
                logEntry += String(ws.sensor[i].w.rain_mm, 1);
            }
            else
            {
                log_i("Rain: -----.-mm");
            }

#if defined BRESSER_6_IN_1 || defined BRESSER_7_IN_1
            logEntry += ",";
            if (ws.sensor[i].w.uv_ok)
            {
                log_i("UVidx: %2.1f",
                      ws.sensor[i].w.uv);
                logEntry += String(ws.sensor[i].w.uv, 1);
            }
            else
            {
                log_i("UVidx: --.-");
            }
#endif
#ifdef BRESSER_7_IN_1
            logEntry += ",";
            if (ws.sensor[i].w.light_ok)
            {
                log_i("Light: %2.1fklx",
                      ws.sensor[i].w.light_klx);
                logEntry += String(ws.sensor[i].w.light_klx, 1);
            }
            else
            {
                log_i("Light: --.-klx");
            }
            if (ws.sensor[i].s_type == SENSOR_TYPE_WEATHER2)
            {
                logEntry += ",";
                if (ws.sensor[i].w.tglobe_ok)
                {
                    log_i("T_globe: %3.1fC",
                          ws.sensor[i].w.tglobe_c);
                    logEntry += String(ws.sensor[i].w.tglobe_c, 1);
                }
                else
                {
                    log_i("T_globe: --.-C");
                }
            }
#endif
            return logEntry;
        } // if weather sensor
    } // for each sensor
    return "";
}

/**
 * @brief Halt execution (indicate failure with LED)
 *
 */
void failureHalt()
{
    while (1)
    {
        delay(1000);
        setLed(true);
    }
}

/**
 * @brief Write log data to a specified file
 *
 * @param fileName The name of the file to write to
 * @param data The data to write to the file
 * @return true if the write was successful
 * @return false if the write failed
 */
bool writeLog(String fileName, String data)
{
    File logFile = SD.open(fileName, FILE_APPEND);

    if (!logFile)
    {
        log_e("Failed to open file for writing");
        return false;
    }

    // Add timestamp and data
    logFile.println(data);
    logFile.close();

    return true;
}

void setup()
{
#if defined(ARDUINO_M5STACK_CORE2)
    setupM5StackCore2();
#endif
    initLed();

    char timestamp[20];
    time_t now;
    struct tm *tm_info;

    Serial.begin(115200);
    delay(500);

    setenv("TZ", TZINFO, 1);
    tzset();

    set_rtc();
    time_t timeStart = time(nullptr);

// Print the RTC time in ISO 8601 format
#if (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG)
    tm_info = localtime(&timeStart);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", tm_info);
#endif
    log_d("Time: %s", timestamp);

#if !defined(IGNORE_IF_RTC_NOT_SET)
    if (timeStart < 100000)
    {
        log_e("RTC not set, halting!");
        failureHalt();
    }
#endif
#if !defined(ARDUINO_M5STACK_CORE2)
    // Initialize SPI for SD card
    // Using hspi, because SPI is already used by WeatherSensor (with other pins)
    SPIClass hspi(HSPI);
    hspi.begin(sd_sck, sd_miso, sd_mosi, sd_cs);
    bool sd_ok = SD.begin(sd_cs, hspi);
#else
    // M5Stack Core2: Using same SPI master for SD card and SX1276 LoRa module
    bool sd_ok = SD.begin(sd_cs);
#endif

    if (!sd_ok)
    {
        log_e("SD Card initialization failed!");
        log_e("Check connections and card format (FAT32)");
        failureHalt();
    }

#if (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG)
    // Print SD card info
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    log_d("SD Card Size: %lluMB", cardSize);
    log_d("Total space: %lluMB", SD.totalBytes() / (1024 * 1024));
    log_d("Used space: %lluMB", SD.usedBytes() / (1024 * 1024));
#endif

    ws.begin();
    ws.setSensorsCfg(MAX_SENSORS, RX_FLAGS);
    String logEntry = receiveSensorData();

    // Print the RTC time in ISO 8601 format
    time_t timeLog = time(nullptr);
    tm_info = localtime(&timeLog);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", tm_info);
    String fileName = String('/') + String(FILENAME_PREFIX) + String(timestamp).substring(0, 10) + ".csv";

    if (logEntry.length() != 0)
    {
        logEntry = String(timestamp) + "," + logEntry;

        // Turn on LED _before_ writing
        setLed(true);
        delay(500);

        // If file does not exist, create it and add header line
        if (!SD.exists(fileName))
        {
            String header = "Timestamp,Temperature_C,Humidity_%,WindGust_m/s,WindAvg_m/s,WindDir_deg,Rain_mm";
#if defined BRESSER_6_IN_1 || defined BRESSER_7_IN_1
            header += ",UV_idx";
#endif
#ifdef BRESSER_7_IN_1
            header += ",Light_klx,T_globe_C";
#endif
            if (!writeLog(fileName, header))
            {
                Serial.println("Failed to write log header");
                failureHalt();
            }
        }

        // Append log entry
        if (writeLog(fileName, logEntry))
        {
            log_i("Logged: %s", logEntry.c_str());
        }
        else
        {
            Serial.println("Failed to write log");
            failureHalt();
        }
        // Turn off LED after writing
        setLed(false);
    }

    // Adjust sleepDuration wrt. execution time
    // to achieve a wakeup interval of 'wakeupInterval'
    uint32_t sleepDuration;
    if (static_cast<long long>(timeLog - timeStart + 10) < wakeupInterval)
        sleepDuration = wakeupInterval - (timeLog - timeStart);
    else
        sleepDuration = 10;

    esp_sleep_enable_timer_wakeup(sleepDuration * 1000UL * 1000UL); // function uses µs
    log_i("Sleeping for %lu s", sleepDuration);
    Serial.flush();

    esp_deep_sleep_start();
}

void loop()
{
    // Not used in this example
}
