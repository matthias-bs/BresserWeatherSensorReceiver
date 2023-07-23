///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorTest.ino
//
// Example for BresserWeatherSensorReceiver - 
// Using recorded test data instead of data from radio receiver.
//
// The data may be incomplete, because certain sensors need two messages to
// transmit a complete data set.
// Which sensor data is received in case of multiple sensors are in range
// depends on the timing of transmitter and receiver.  
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 05/2022
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
// 20220523 Created from BresserWeatherSensorBasic.ino
//
// ToDo: 
// - 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

uint8_t testData[][MSG_BUF_SIZE-1] = {
    // Lightning Sensor
    {0x73, 0x69, 0xB5, 0x08, 0xAA, 0xA2, 0x90, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
     
    // 5-in-1 Weather Sensor
    {0xEA, 0xEC, 0x7F, 0xEB, 0x5F, 0xEE, 0xEF, 0xFA, 0xFE, 0x76, 0xBB, 0xFA, 0xFF, 0x15, 0x13, 0x80, 0x14, 0xA0, 0x11,
     0x10, 0x05, 0x01, 0x89, 0x44, 0x05, 0x00},

    // 7-in-1 Weather Sensor
    {0xC4, 0xD6, 0x3A, 0xC5, 0xBD, 0xFA, 0x18, 0xAA, 0xAA, 0xAA, 0xAA, 0xAB, 0xFC, 0xAA, 0x98, 0xDA, 0x89, 0xA3, 0x2F,
     0xEC, 0xAF, 0x9A, 0xAA, 0xAA, 0xAA, 0x00}
};

WeatherSensor weatherSensor;


void setup() {    
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    Serial.printf("Starting execution...\n");

    weatherSensor.begin();
}

void loop() 
{   
    // This example uses only a single slot in the sensor data array
    int const i=0;

    int idx = 0;
    // Clear all sensor data
    weatherSensor.clearSlots();

    ixd = (idx == (sizeof(testData)/sizeof(MSG_BUF_SIZE)) - 1) ? 0 : idx++;

    // Tries to receive radio message (non-blocking) and to decode it.
    // Timeout occurs after a small multiple of expected time-on-air.
    DecodeStatus decode_status = weatherSensor.decodeMessage(test_data[idx][0], MSG_BUF_SIZE-1);

    if (decode_status == DECODE_OK) {
      if (weatherSensor.sensor[i].s_type != SENSOR_TYPE_LIGHTNING) {
        Serial.printf("Id: [%8X] Typ: [%X] Battery: [%s] ",
            weatherSensor.sensor[i].sensor_id,
            weatherSensor.sensor[i].s_type,
            weatherSensor.sensor[i].battery_ok ? "OK " : "Low");
        #ifdef BRESSER_6_IN_1
            Serial.printf("Ch: [%d] ", weatherSensor.sensor[i].chan);
        #endif
        if (weatherSensor.sensor[i].temp_ok) {
            Serial.printf("Temp: [%5.1fC] ",
                weatherSensor.sensor[i].temp_c);
        } else {
            Serial.printf("Temp: [---.-C] ");
        }
        if (weatherSensor.sensor[i].humidity_ok) {
            Serial.printf("Hum: [%3d%%] ",
                weatherSensor.sensor[i].humidity);
        }
        else {
            Serial.printf("Hum: [---%%] ");
        }
        if (weatherSensor.sensor[i].wind_ok) {
            Serial.printf("Wind max: [%4.1fm/s] Wind avg: [%4.1fm/s] Wind dir: [%5.1fdeg] ",
                    weatherSensor.sensor[i].wind_gust_meter_sec,
                    weatherSensor.sensor[i].wind_avg_meter_sec,
                    weatherSensor.sensor[i].wind_direction_deg);
        } else {
            Serial.printf("Wind max: [--.-m/s] Wind avg: [--.-m/s] Wind dir: [---.-deg] ");
        }
        if (weatherSensor.sensor[i].rain_ok) {
            Serial.printf("Rain: [%7.1fmm] ",  
                weatherSensor.sensor[i].rain_mm);
        } else {
            Serial.printf("Rain: [-----.-mm] "); 
        }
        if (weatherSensor.sensor[i].moisture_ok) {
            Serial.printf("Moisture: [%2d%%] ",
                weatherSensor.sensor[i].moisture);
        }
        else {
            Serial.printf("Moisture: [--%%] ");
        }
        #if defined BRESSER_6_IN_1 || defined BRESSER_7_IN_1
        if (weatherSensor.sensor[i].uv_ok) {
            Serial.printf("UV index: [%1.1f] ",
                weatherSensor.sensor[i].uv);
        }
        else {
            Serial.printf("UV index: [-.-%%] ");
        }
        #endif
        #ifdef BRESSER_7_IN_1
        if (weatherSensor.sensor[i].light_ok) {
            Serial.printf("Light (Klux): [%2.1fKlux] ",
                weatherSensor.sensor[i].light_klx);
        }
        else {
            Serial.printf("Light (lux): [--.-Klux] ");
        }
        #endif      
        
      } else {
        Serial.printf("Id: [%8X] Typ: [%X] Battery: [%s] ",
            weatherSensor.sensor[i].sensor_id,
            weatherSensor.sensor[i].s_type,
            weatherSensor.sensor[i].battery_ok ? "OK " : "Low");
        Serial.printf("Lightning Counter: [%3d] ", weatherSensor.sensor[i].lightning_count);
        if (weatherSensor.sensor[i].lightning_distance_km != 0) {
          Serial.printf("Distance: [%2dkm] ", weatherSensor.sensor[i].lightning_distance_km);
        } else {
          Serial.printf("Distance: [----] ");
        }
        Serial.printf("unknown1: [0x%03X] ", weatherSensor.sensor[i].lightning_unknown1);
        Serial.printf("unknown2: [0x%04X] ", weatherSensor.sensor[i].lightning_unknown2);
      }
      Serial.printf("RSSI: [%5.1fdBm]\n", weatherSensor.sensor[i].rssi);
    } // if (decode_status == DECODE_OK)
    delay(100);
} // loop()
