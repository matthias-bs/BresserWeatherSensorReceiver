///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorM5Core.ino
//
// Example for BresserWeatherSensorReceiver on M5Stack Core2 with M5Stack Module LoRa868
// Using getMessage() for non-blocking reception of a single data message.
// Weather sensor data is presented on the display.
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 03/2024
//
//
// MIT License
//
// Copyright (c) 2024 Matthias Prinke
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
// 20240324 Created from BresserWeatherSensorBasic.ino
// 20240325 Fake missing degree sign with small 'o', print only weather sensor data on LCD
// 20240504 Added board initialization
// 20241103 Added logging to SD card
//
// Notes:
// - The character set does not provide a degrees sign
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <M5Unified.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "InitBoard.h"
#include "src/utils.h"

#define LOG_INTERVAL 600 // Log interval [s]
#define FILENAME_PREFIX "bresser-" // Filename: <FILENAME_PREFIX><YYYY>-<MM>-<DD>.csv
#define TZINFO "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"
#define MAX_SENSORS 1
#define RX_TIMEOUT 180000 // sensor receive timeout [ms]

// Stop reception when data of at least one sensor is complete
// #define RX_FLAGS DATA_COMPLETE

// Stop reception when data of all (max_sensors) is complete
#define RX_FLAGS (DATA_COMPLETE | DATA_ALL_SLOTS)

// See https://docs.m5stack.com/en/core/core2
static const uint8_t sd_cs = 4;

bool SD_initialized = false;
WeatherSensor ws;

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
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    initBoard();
    initLed();

    setenv("TZ", TZINFO, 1);
    tzset();

    Serial.printf("Starting execution...\n");


    // Note: M5.begin() is called in ws.begin()
    ws.begin();
    ws.setSensorsCfg(MAX_SENSORS, RX_FLAGS);

    set_rtc();

    M5.Lcd.setTextSize(2);    // Set the font size
    M5.Lcd.fillScreen(WHITE); // Set the screen background.
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(BLUE); // Set the font color
    M5.Lcd.printf("Weather Sensor Receiver");
    M5.Lcd.setCursor(0, 50);
    M5.Lcd.setTextColor(BLACK);

    // Initialize SD card (SPI master shared with SX1276 radio)
    if (!SD.begin(sd_cs))
    {
        M5.Lcd.printf("SD Card init. failed!\n\n");
    } else {
        SD_initialized = true;
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        M5.Lcd.printf("SD Card Size: %5llu MB\n", cardSize);
        M5.Lcd.printf("Total space:  %5llu MB\n", SD.totalBytes() / (1024 * 1024));
        M5.Lcd.printf("Used space:   %5llu MB\n\n", SD.usedBytes() / (1024 * 1024));

        time_t now = time(nullptr);
        struct tm *tm_info;
        tm_info = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", tm_info);
        M5.Lcd.printf("%s\n\n", timestamp);
        delay(2000);
    }

    M5.Lcd.printf("Waiting for data...");
}

void loop()
{
    // This example uses only a single slot in the sensor data array
    int const i = 0;
    static time_t lastLog = time(nullptr);

    char timestamp[20];
    struct tm *tm_info;
    
    String logEntry;

    logEntry.reserve(128);
    logEntry = "";

    // Clear all sensor data
    ws.clearSlots();

    // Tries to receive radio message (non-blocking) and to decode it.
    // Timeout occurs after a small multiple of expected time-on-air.
    int decode_status = ws.getData(RX_TIMEOUT, RX_FLAGS, 0, nullptr);
    uint8_t y = 10;

    if (decode_status == DECODE_OK)
    {
        if ((ws.sensor[i].s_type == SENSOR_TYPE_WEATHER0) || (ws.sensor[i].s_type == SENSOR_TYPE_WEATHER1))
        {
            M5.Lcd.fillScreen(WHITE); // Set the screen background.
            M5.Lcd.setCursor(10, y);
            M5.Lcd.setTextColor(BLUE); // Set the font color
            M5.Lcd.printf("Weather Sensor Receiver");
            M5.Lcd.setTextColor(BLACK); // Set the font color
        }

        Serial.printf("Id: [%8X] Typ: [%X] Ch: [%d] St: [%d] Bat: [%-3s] RSSI: [%6.1fdBm] ",
                      static_cast<int>(ws.sensor[i].sensor_id),
                      ws.sensor[i].s_type,
                      ws.sensor[i].chan,
                      ws.sensor[i].startup,
                      ws.sensor[i].battery_ok ? "OK " : "Low",
                      ws.sensor[i].rssi);

        if (ws.sensor[i].s_type == SENSOR_TYPE_LIGHTNING)
        {
            // Lightning Sensor
            Serial.printf("Lightning Counter: [%4d] ", ws.sensor[i].lgt.strike_count);
            if (ws.sensor[i].lgt.distance_km != 0)
            {
                Serial.printf("Distance: [%2dkm] ", ws.sensor[i].lgt.distance_km);
            }
            else
            {
                Serial.printf("Distance: [----] ");
            }
            Serial.printf("unknown1: [0x%03X] ", ws.sensor[i].lgt.unknown1);
            Serial.printf("unknown2: [0x%04X]\n", ws.sensor[i].lgt.unknown2);
        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_LEAKAGE)
        {
            // Water Leakage Sensor
            Serial.printf("Leakage: [%-5s]\n", (ws.sensor[i].leak.alarm) ? "ALARM" : "OK");
        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_AIR_PM)
        {
            // Air Quality (Particular Matter) Sensor
            if (ws.sensor[i].pm.pm_1_0_init)
            {
                Serial.printf("PM1.0: [init] ");
            }
            else
            {
                Serial.printf("PM1.0: [%uµg/m³] ", ws.sensor[i].pm.pm_1_0);
            }
            if (ws.sensor[i].pm.pm_2_5_init)
            {
                Serial.printf("PM2.5: [init] ");
            }
            else
            {
                Serial.printf("PM2.5: [%uµg/m³] ", ws.sensor[i].pm.pm_2_5);
            }
            if (ws.sensor[i].pm.pm_10_init)
            {
                Serial.printf("PM10: [init]\n");
            }
            else
            {
                Serial.printf("PM10: [%uµg/m³]\n", ws.sensor[i].pm.pm_10);
            }
        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_CO2)
        {
            // CO2 Sensor
            if (ws.sensor[i].co2.co2_init)
            {
                Serial.printf("CO2: [init]\n");
            }
            else
            {
                Serial.printf("CO2: [%uppm]\n", ws.sensor[i].co2.co2_ppm);
            }
        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_HCHO_VOC)
        {
            // HCHO / VOC Sensor
            if (ws.sensor[i].voc.hcho_init)
            {
                Serial.printf("HCHO: [init] ");
            }
            else
            {
                Serial.printf("HCHO: [%uppb] ", ws.sensor[i].voc.hcho_ppb);
            }
            if (ws.sensor[i].voc.voc_init)
            {
                Serial.printf("VOC: [init]\n");
            }
            else
            {
                Serial.printf("VOC: [%u]\n", ws.sensor[i].voc.voc_level);
            }
        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_SOIL)
        {
            Serial.printf("Temp: [%5.1fC] ", ws.sensor[i].soil.temp_c);
            Serial.printf("Moisture: [%2d%%]\n", ws.sensor[i].soil.moisture);
        }
        else
        {
            // Any other (weather-like) sensor is very similar
            if (ws.sensor[i].w.temp_ok)
            {
                Serial.printf("Temp: [%5.1fC] ", ws.sensor[i].w.temp_c);
            }
            else
            {
                Serial.printf("Temp: [---.-C] ");
            }
            if (ws.sensor[i].w.humidity_ok)
            {
                Serial.printf("Hum: [%3d%%] ", ws.sensor[i].w.humidity);
            }
            else
            {
                Serial.printf("Hum: [---%%] ");
            }
            if (ws.sensor[i].w.wind_ok)
            {
                Serial.printf("Wmax: [%4.1fm/s] Wavg: [%4.1fm/s] Wdir: [%5.1fdeg] ",
                              ws.sensor[i].w.wind_gust_meter_sec,
                              ws.sensor[i].w.wind_avg_meter_sec,
                              ws.sensor[i].w.wind_direction_deg);
            }
            else
            {
                Serial.printf("Wmax: [--.-m/s] Wavg: [--.-m/s] Wdir: [---.-deg] ");
            }
            if (ws.sensor[i].w.rain_ok)
            {
                Serial.printf("Rain: [%7.1fmm] ",
                              ws.sensor[i].w.rain_mm);
            }
            else
            {
                Serial.printf("Rain: [-----.-mm] ");
            }

#if defined BRESSER_6_IN_1 || defined BRESSER_7_IN_1
            if (ws.sensor[i].w.uv_ok)
            {
                Serial.printf("UVidx: [%2.1f] ",
                              ws.sensor[i].w.uv);
            }
            else
            {
                Serial.printf("UVidx: [--.-] ");
            }
#endif
#ifdef BRESSER_7_IN_1
            if (ws.sensor[i].w.light_ok)
            {
                Serial.printf("Light: [%2.1fklx] ",
                              ws.sensor[i].w.light_klx);
            }
            else
            {
                Serial.printf("Light: [--.-klx] ");
            }
#endif
            Serial.printf("\n");

            if ((ws.sensor[i].s_type == SENSOR_TYPE_WEATHER0) || (ws.sensor[i].s_type == SENSOR_TYPE_WEATHER1))
            {
                y += 30;
                M5.Lcd.setTextSize(3); // Set the font size
                M5.Lcd.setCursor(10, y);
                static float temp_c;
                static uint8_t humidity;
                if (ws.sensor[i].w.temp_ok)
                {
                    M5.Lcd.printf("Temp: %5.1f", temp_c = ws.sensor[i].w.temp_c);
                    logEntry += String(ws.sensor[i].w.temp_c, 1);

                }
                else
                {
                    M5.Lcd.printf("Temp: %5.1f", temp_c);
                }
                logEntry += ",";

                // Fake degrees sign which is not available in character set :-(
                M5.Lcd.setTextSize(1);
                M5.Lcd.printf("o");
                M5.Lcd.setTextSize(3);
                M5.Lcd.printf("C");

                y += 34;
                M5.Lcd.setTextSize(2);
                M5.Lcd.setCursor(10, y);
                if (ws.sensor[i].w.humidity_ok)
                {
                    M5.Lcd.printf("Hum:     %3d%% ", humidity = ws.sensor[i].w.humidity);
                    logEntry += String(ws.sensor[i].w.humidity);
                }
                else
                {
                    M5.Lcd.printf("Hum:     %3d%% ", humidity);
                }
                logEntry += ",";

                y += 24;
                M5.Lcd.setCursor(10, y);
                if (ws.sensor[i].w.wind_ok)
                {
                    M5.Lcd.printf("Wmx: %4.1fm/s Wav: %4.1fm/s",
                                  ws.sensor[i].w.wind_gust_meter_sec,
                                  ws.sensor[i].w.wind_avg_meter_sec);
                    logEntry += String(ws.sensor[i].w.wind_gust_meter_sec, 1) + ",";
                    logEntry += String(ws.sensor[i].w.wind_avg_meter_sec, 1) + ",";

                    y += 24;
                    M5.Lcd.setCursor(10, y);
                    M5.Lcd.printf("Wdir:  %5.1fdeg",
                                  ws.sensor[i].w.wind_direction_deg);
                    logEntry += String(ws.sensor[i].w.wind_direction_deg, 1);
                }
                else
                {
                    M5.Lcd.printf("Wmx: --.-m/s Wav: --.-m/s");
                    y += 22;
                    M5.Lcd.setCursor(10, y);
                    M5.Lcd.printf("Wdir:  ---.-deg");
                    logEntry += ",,";
                }
                logEntry += ",";

                y += 24;
                M5.Lcd.setCursor(10, y);
                static float rain_mm;
                if (ws.sensor[i].w.rain_ok)
                {
                    M5.Lcd.printf("Rain:%7.1fmm ",
                                  rain_mm = ws.sensor[i].w.rain_mm);
                    logEntry += String(ws.sensor[i].w.rain_mm, 1);
                }
                else
                {
                    M5.Lcd.printf("Rain:%7.1fmm ",
                                  rain_mm);
                }
                logEntry += ",";

                M5.Lcd.setTextColor(DARKGREY);
                y = 195;
                M5.Lcd.setCursor(10, y);
                M5.Lcd.printf("Id: %8X Typ: %X Ch: %d",
                              static_cast<int>(ws.sensor[i].sensor_id),
                              ws.sensor[i].s_type,
                              ws.sensor[i].chan);

                static bool battery_ok;
                y += 24;
                M5.Lcd.setCursor(10, y);
                if (ws.sensor[i].w.temp_ok)
                {
                    // 6-in-1 protocol - only valid in messages containing temp
                    battery_ok = ws.sensor[i].battery_ok;
                }
                M5.Lcd.printf("S: %u B: %u RSSI: %6.1fdBm",
                              ws.sensor[i].startup,
                              battery_ok,
                              ws.sensor[i].rssi);
            } // if ((ws.sensor[i].s_type == SENSOR_TYPE_WEATHER0) || (ws.sensor[i].s_type == SENSOR_TYPE_WEATHER1))
        } // weather-like sensor

        time_t now = time(nullptr);
        log_d("now - lastLog: %llu", now - lastLog);
        if (SD_initialized && (now - lastLog >= LOG_INTERVAL))
        {
            // Print the RTC time in ISO 8601 format
            tm_info = localtime(&now);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", tm_info);

            String fileName = String('/') + String(FILENAME_PREFIX) + String(timestamp).substring(0, 10) + ".csv";

            if (logEntry.length() != 0)
            {
                lastLog = now;
                setLed(true);
                delay(500);
                logEntry = String(timestamp) + "," + logEntry;

                // If file does not exist, create it and add header line
                if (!SD.exists(fileName))
                {
                    String header = "Timestamp,Temperature_C,Humidity_%,WindGust_m/s,WindAvg_m/s,WindDir_deg,Rain_mm";
                    if (!writeLog(fileName, header))
                    {
                        log_d("Failed to write log header");
                    }
                }

                // Append log entry
                if (writeLog(fileName, logEntry))
                {
                    log_d("Logged: %s\n", logEntry.c_str());
                }
                else
                {
                    log_d("Failed to write log entry");
                }
                setLed(false);
            }
        } // if time to log
    } // if (decode_status == DECODE_OK)
    delay(100);
} // loop()
