//////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorOLED.ino
//
// Example for BresserWeatherSensorReceiver
//
// This sketch prints the received weather sensor values to the on-board OLED display.
// See https://github.com/adafruit/Adafruit_SSD1306/tree/master/examples for text style options
// and scrolling.
//
// Notes:
// - Currently only some boards by LILYGO are supported.
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
// 20251006 Created
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

#define MAX_SENSORS 1
#define RX_TIMEOUT 180000 // sensor receive timeout [ms]
#define UPDATE_DELAY 30   // Delay between updates [s]

// Stop reception when data of at least one sensor is complete
// #define RX_FLAGS DATA_COMPLETE

// Stop reception when data of all (max_sensors) is complete
#define RX_FLAGS (DATA_COMPLETE | DATA_ALL_SLOTS)

#if defined(ARDUINO_TTGO_LoRa32_V1) || \
    defined(ARDUINO_TTGO_LoRa32_V2) || \
    defined(ARDUINO_TTGO_LoRa32_v21new)
#define OLED_RESET -1
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#elif defined(ARDUINO_LILYGO_T3S3_SX1262) || \
    defined(ARDUINO_LILYGO_T3S3_SX1276) ||   \
    defined(ARDUINO_LILYGO_T3S3_LR1121)
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#else
#pragma message("Board not supported yet!")
#endif

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create weather sensor receiver object
WeatherSensor weatherSensor;

void setup()
{
    // Serial port for debugging purposes
    Serial.begin(115200);

#if defined(ARDUINO_TTGO_LoRa32_V1) || \
    defined(ARDUINO_TTGO_LoRa32_V2) || \
    defined(ARDUINO_TTGO_LoRa32_v21new)
    // I2C pin configuration
    Wire.begin(OLED_SDA, OLED_SCL);
#elif defined(ARDUINO_LILYGO_T3S3_SX1262) || \
    defined(ARDUINO_LILYGO_T3S3_SX1276) ||   \
    defined(ARDUINO_LILYGO_T3S3_LR1121)
    // I2C pin configuration
    Wire.begin(SDA, SCL);
#endif

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        log_e("SSD1306 allocation failed");
    }

    // Clear the buffer
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);             // Start at top-left corner
    display.cp437(true);                 // Use full 256 char 'Code Page 437' font
    display.println("Bresser");
    display.println("Weather Sensor");
    display.println("Receiver");
    display.display(); // Show initial text

    weatherSensor.begin();
    weatherSensor.setSensorsCfg(MAX_SENSORS, RX_FLAGS);
    display.clearDisplay();
}

void loop()
{
    // Clear sensor data buffer
    weatherSensor.clearSlots();

// Attempt to receive data set with timeout of <xx> s
#if (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO)
    bool decode_ok = weatherSensor.getData(RX_TIMEOUT, RX_FLAGS, 0, nullptr);
#else
    weatherSensor.getData(RX_TIMEOUT, RX_FLAGS, 0, nullptr);
#endif
    log_i("decode_ok: %d", decode_ok);

    for (size_t i = 0; i < weatherSensor.sensor.size(); i++)
    {
        if (!weatherSensor.sensor[i].valid)
            continue;

        if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER3) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER8))
        {
            display.clearDisplay();
            display.setCursor(0, 0);
            if (weatherSensor.sensor[i].w.temp_ok)
            {
                display.print(weatherSensor.sensor[i].w.temp_c);
                display.print("\xF8");
                display.print("C ");
            }
            if (weatherSensor.sensor[i].w.humidity_ok)
            {
                display.print(weatherSensor.sensor[i].w.humidity);
                display.println("%");
            }
            if (weatherSensor.sensor[i].w.wind_ok)
            {
                display.print(weatherSensor.sensor[i].w.wind_avg_meter_sec);
                display.print("m/s ");
                display.print(weatherSensor.sensor[i].w.wind_gust_meter_sec);
                display.println("m/s ");
                display.print(weatherSensor.sensor[i].w.wind_direction_deg);
                display.println("\xF8");
            }
            if (weatherSensor.sensor[i].w.rain_ok)
            {
                display.print(weatherSensor.sensor[i].w.rain_mm);
                display.println("mm");
            }
            display.display();
        }
    }

    esp_sleep_enable_timer_wakeup(UPDATE_DELAY * 1000000);
    esp_light_sleep_start();
}