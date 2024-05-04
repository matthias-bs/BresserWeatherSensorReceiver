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
// 20230723 Created from BresserWeatherSensorBasic.ino
// 20230804 Added Bresser Water Leakage Sensor decoder
// 20231027 Refactored sensor structure
// 20240209 Added Air Quality (HCHO/VOC), Air Quality (PM2.5/PM10), CO2 Sensor and Pool Thermometer
// 20240504 Added board initialization
//
// ToDo: 
// - 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "InitBoard.h"

const uint8_t testData[][MSG_BUF_SIZE-1] = {
    // #0: Lightning Sensor
    {0x73, 0x69, 0xB5, 0x08, 0xAA, 0xA2, 0x90, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},

    // #1: 5-in-1 Weather Sensor
    {0xEA, 0xEC, 0x7F, 0xEB, 0x5F, 0xEE, 0xEF, 0xFA, 0xFE, 0x76, 0xBB, 0xFA, 0xFF, 0x15, 0x13, 0x80, 0x14, 0xA0, 0x11,
     0x10, 0x05, 0x01, 0x89, 0x44, 0x05, 0x00},

    // #2: 6-in-1 Sensor - Wind, Battery, Temperature, Humidity, UV
    {0x54, 0x1B, 0x21, 0x10, 0x34, 0x27, 0x18, 0xFF, 0x88, 0xFF, 0x29, 0x28, 0x06, 0x42, 0x87, 0xFF, 0xF0, 0xC6, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    // #3: 6-in-1 Sensor - Wind, Rain
    {0x2A, 0xAF, 0x21, 0x10, 0x34, 0x27, 0x18, 0xFF, 0xAA, 0xFF, 0x29, 0x28, 0xFF, 0xBB, 0x89, 0xFF, 0x01, 0x1F, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    // #4: Soil Temperature/Moisture (6-in-1 protocol)
    {0xA1, 0x30, 0x74, 0x50, 0x85, 0x86, 0x49, 0xBB, 0xBB, 0xBB, 0xBB, 0xB0, 0x20, 0x56, 0x08, 0xBB, 0xB0, 0x62, 0x00, 
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    
    // #5: Pool Thermometer (6-in-1 protocol)
    {0x5D, 0x37, 0x22, 0x40, 0x08, 0x73, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x86, 0x00, 0x00, 0x00, 0x3C, 0x00,
     0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00},

    // #6: 7-in-1 Weather Sensor
    {0xC4, 0xD6, 0x3A, 0xC5, 0xBD, 0xFA, 0x18, 0xAA, 0xAA, 0xAA, 0xAA, 0xAB, 0xFC, 0xAA, 0x98, 0xDA, 0x89, 0xA3, 0x2F,
     0xEC, 0xAF, 0x9A, 0xAA, 0xAA, 0xAA, 0x00},

    // #7: Water Leakage Sensor
    {0xB3, 0xDA, 0x55, 0x57, 0x17, 0x40, 0x53, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB},

    // #8: Air Quality (PM2.5/PM10) Sensor
    {0x55, 0x3D, 0xFC, 0x2A, 0xAA, 0xAA, 0x89, 0xAA, 0xAA, 0xAB, 0x8A, 0xAB, 0xFA, 0xAB, 0xCA, 0xA2, 0xAA, 0xA3, 0xCA,
     0xA3, 0x2A, 0x92, 0xFA, 0xAA, 0xAA, 0x00},

    // #9: CO2 Sensor
    {0x04, 0xa9, 0xd7, 0x82, 0xac, 0xd8, 0xa1, 0xad, 0x9a, 0xad, 0x9a, 0xad, 0x9a, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xe9, 0x9a, 0xaa, 0xaa, 0x00},

    // #10: Air Quality (HCHO/VOC) Sensor
    {0x0c, 0x1c, 0xc4, 0xa5, 0xaa, 0xaf, 0xb1, 0xaa, 0xa8, 0xaa, 0xa8, 0xaa, 0xa8, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xe9, 0xff, 0xaa, 0xaa, 0x00}
};

WeatherSensor ws;


void setup() {    
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    initBoard();

    Serial.printf("Starting execution...\n");

    ws.begin();
}

void loop() 
{   
    // This example uses only a single slot in the sensor data array
    int const i=0;

    static int idx = 0;
    // Clear all sensor data
    ws.clearSlots();

    // Tries to receive radio message (non-blocking) and to decode it.
    // Timeout occurs after a small multiple of expected time-on-air.
    DecodeStatus decode_status = ws.decodeMessage(&testData[idx][0], MSG_BUF_SIZE-1);

    idx = (idx == 10) ? 0 : idx+1;
    Serial.printf("testData[%d]\n", idx);

    if (decode_status == DECODE_OK) {
        Serial.printf("Id: [%8X] Typ: [%X] Ch: [%d] St: [%d] Bat: [%-3s] RSSI: [%6.1fdBm] ",
            (unsigned int)ws.sensor[i].sensor_id,
            ws.sensor[i].s_type,
            ws.sensor[i].chan,
            ws.sensor[i].startup,
            ws.sensor[i].battery_ok ? "OK " : "Low",
            ws.sensor[i].rssi);
           
        if (ws.sensor[i].s_type == SENSOR_TYPE_LIGHTNING) {
            // Lightning Sensor
            Serial.printf("Lightning Counter: [%3d] ", ws.sensor[i].lgt.strike_count);
            if (ws.sensor[i].lgt.distance_km != 0) {
                Serial.printf("Distance: [%2dkm] ", ws.sensor[i].lgt.distance_km);
            } else {
                Serial.printf("Distance: [----] ");
            }
            Serial.printf("unknown1: [0x%03X] ", ws.sensor[i].lgt.unknown1);
            Serial.printf("unknown2: [0x%04X]\n", ws.sensor[i].lgt.unknown2);

        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_LEAKAGE) {
            // Water Leakage Sensor
            Serial.printf("Leakage: [%-5s]\n", (ws.sensor[i].leak.alarm) ? "ALARM" : "OK");
      
        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_AIR_PM) {
            // Air Quality (Particular Matter) Sensor
            if (ws.sensor[i].pm.pm_2_5_init) {
                Serial.printf("PM2.5: [init] ");
            } else {
                Serial.printf("PM2.5: [%uµg/m³] ", ws.sensor[i].pm.pm_2_5);
            }
            if (ws.sensor[i].pm.pm_10_init) {
                Serial.printf("PM10: [init]\n");
            } else {
                Serial.printf("PM10: [%uµg/m³]\n", ws.sensor[i].pm.pm_10);
            }
            
        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_CO2) {
            // CO2 Sensor
            if (ws.sensor[i].co2.co2_init) {
                Serial.printf("CO2: [init]\n");
            } else {
                Serial.printf("CO2: [%uppm]\n", ws.sensor[i].co2.co2_ppm);
            }

        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_HCHO_VOC) {
            // HCHO / VOC Sensor
            if (ws.sensor[i].voc.hcho_init) {
                Serial.printf("HCHO: [init] ");
            } else {
                Serial.printf("HCHO: [%uppb] ", ws.sensor[i].voc.hcho_ppb);
            }
            if (ws.sensor[i].voc.voc_init) {
                Serial.printf("VOC: [init]\n");
            } else {
                Serial.printf("VOC: [%u]\n", ws.sensor[i].voc.voc_level);
            }

        }
        else if (ws.sensor[i].s_type == SENSOR_TYPE_SOIL) {
            Serial.printf("Temp: [%5.1fC] ", ws.sensor[i].soil.temp_c);
            Serial.printf("Moisture: [%2d%%]\n", ws.sensor[i].soil.moisture);

        } else {
            // Any other (weather-like) sensor is very similar
            if (ws.sensor[i].w.temp_ok) {
                Serial.printf("Temp: [%5.1fC] ", ws.sensor[i].w.temp_c);
            } else {
                Serial.printf("Temp: [---.-C] ");
            }
            if (ws.sensor[i].w.humidity_ok) {
                Serial.printf("Hum: [%3d%%] ", ws.sensor[i].w.humidity);
            }
            else {
                Serial.printf("Hum: [---%%] ");
            }
            if (ws.sensor[i].w.wind_ok) {
                Serial.printf("Wmax: [%4.1fm/s] Wavg: [%4.1fm/s] Wdir: [%5.1fdeg] ",
                        ws.sensor[i].w.wind_gust_meter_sec,
                        ws.sensor[i].w.wind_avg_meter_sec,
                        ws.sensor[i].w.wind_direction_deg);
            } else {
                Serial.printf("Wmax: [--.-m/s] Wavg: [--.-m/s] Wdir: [---.-deg] ");
            }
            if (ws.sensor[i].w.rain_ok) {
                Serial.printf("Rain: [%7.1fmm] ",  
                    ws.sensor[i].w.rain_mm);
            } else {
                Serial.printf("Rain: [-----.-mm] "); 
            }
        
            #if defined BRESSER_6_IN_1 || defined BRESSER_7_IN_1
            if (ws.sensor[i].w.uv_ok) {
                Serial.printf("UVidx: [%1.1f] ",
                    ws.sensor[i].w.uv);
            }
            else {
                Serial.printf("UVidx: [-.-%%] ");
            }
            #endif
            #ifdef BRESSER_7_IN_1
            if (ws.sensor[i].w.light_ok) {
                Serial.printf("Light: [%2.1fKlux] ",
                    ws.sensor[i].w.light_klx);
            }
            else {
                Serial.printf("Light: [--.-Klux] ");
            }
            #endif
            Serial.printf("\n");
        }
    } // if (decode_status == DECODE_OK)
    delay(1000);
} // loop()
