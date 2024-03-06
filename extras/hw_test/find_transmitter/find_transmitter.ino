///////////////////////////////////////////////////////////////////////////////////////////////////
// find_transmitter.ino
//  
// Sketch for finding a sensor transmitter with a TTGO LoRa32-OLED and a directional antenna
//
// - BresserWeatherSensorReceiver receives the sensor data and measures the RSSI
// - An LSM303DLH module is used as an electronic compass and provides the heading
// - LoRa32-OLED displays heading, RSSI and some sensor data
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
// Wiring:
// see https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/assets/image/lora32_v1.6.1_pinmap.jpg
// and https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/schematic/T3_V1.6.pdf
//
// TTGO LoRa32-OLED | LSM303DLH
// -----------------|----------
// JP1.2 GND        | GND
// JP1.3 VCC3V3     | VIN
// JP1.10 IO22      | SCL
// JP1.13 IO21      | SDA
//
// Based on
// - BresserWeatherSensorBasic.ino
// - Adafruit SSD1306 examples/ssd_128x32_i2c.ino (https://github.com/adafruit/Adafruit_SSD1306)
// - Adafruit LSM303DLH Mag examples/compass.ino (https://github.com/adafruit/Adafruit_LSM303DLH_Mag)
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
// 20240306 Created
//
// ToDo: 
// - 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_LSM303DLH_Mag.h>
#include <Adafruit_Sensor.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_LSM303DLH_Mag_Unified mag = Adafruit_LSM303DLH_Mag_Unified(12345);

WeatherSensor ws;

void setup() {    
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    Serial.printf("Starting execution...\n");
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (1); // Don't proceed, loop forever
    }

    // Initialise the sensor
    if (!mag.begin()) {
       // There was a problem detecting the LSM303 ... check your connections
       Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
       while (1); // Don't proceed, loop forever
    }

    display.cp437(true);
    ws.begin();
}


void loop() 
{   
    // Get a new sensor event
    sensors_event_t event;
    mag.getEvent(&event);

    float const Pi = 3.14159;

    // Calculate the angle of the vector y,x
    float heading = (atan2(event.magnetic.y, event.magnetic.x) * 180) / Pi;

    // Normalize to 0-360
    if (heading < 0) {
        heading = 360 + heading;
    }

    // This example uses only a single slot in the sensor data array
    int const i=0;

    // Clear all sensor data
    ws.clearSlots();

    // Tries to receive radio message (non-blocking) and to decode it.
    // Timeout occurs after a small multiple of expected time-on-air.
    int decode_status = ws.getMessage();

    if (decode_status == DECODE_OK) {
        char buf[44];
        Serial.printf("Heading: [%3.0f°] Id: [%8X] Typ: [%X] Ch: [%d] St: [%d] Bat: [%-3s] RSSI: [%6.1fdBm]\n",
            heading,
            static_cast<int> (ws.sensor[i].sensor_id),
            ws.sensor[i].s_type,
            ws.sensor[i].chan,
            ws.sensor[i].startup,
            ws.sensor[i].battery_ok ? "OK " : "Low",
            ws.sensor[i].rssi);
        snprintf(buf, sizeof(buf)-1, "Id: %8X Typ: %X\nCh: %d St: %d Bat: %u",
            static_cast<int> (ws.sensor[i].sensor_id),
            ws.sensor[i].s_type,
            ws.sensor[i].chan,
            ws.sensor[i].startup,
            ws.sensor[i].battery_ok);

        display.clearDisplay();

        display.setTextSize(1);              // Normal 1:1 pixel scale
        display.setTextColor(SSD1306_WHITE); // Draw white text
        display.setCursor(0,0);              // Start at top-left corner
        display.println(buf);
       
        snprintf(buf, sizeof(buf)-1, "%03d%c %4d",
            static_cast<int>(heading),
            0xF8, // degree character in cp437
            static_cast<int>(ws.sensor[i].rssi));
        display.setTextSize(2);              // Draw 2X-scale text
        display.println(buf);
        display.display();
    } // if (decode_status == DECODE_OK)
    delay(100);
} // loop()
