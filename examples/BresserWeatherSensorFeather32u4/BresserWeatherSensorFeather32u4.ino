///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorFeather32u4.ino
//
// Example for BresserWeatherSensorReceiver - 
// Using getMessage() for non-blocking reception of a single data message.
//
// The data may be incomplete, because certain sensors need two messages to
// transmit a complete data set.
// Which sensor data is received in case of multiple sensors are in range
// depends on the timing of transmitter and receiver.  
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 03/2023
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
// 20230330 Created from BresserWeatherSensorBasic.ino
// 20231027 Refactored sensor structure
//
// ToDo: 
// - 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"


WeatherSensor ws;


void setup() {    
    Serial.begin(115200);

    Serial.println(F("Starting execution..."));

    ws.begin();
}


void loop() 
{   
    // This example uses only a single slot in the sensor data array
    int const i=0;

    // Clear all sensor data
    ws.clearSlots();

    // Tries to receive radio message (non-blocking) and to decode it.
    // Timeout occurs after a small multiple of expected time-on-air.
    int decode_status = ws.getMessage();
  
    if (decode_status == DECODE_OK) {
        Serial.print(F("Id: "));
        Serial.print(ws.sensor[i].sensor_id, HEX);
        Serial.print(F(" Typ: "));
        Serial.print(ws.sensor[i].s_type);
        Serial.print(F(" Battery: "));
        Serial.print(ws.sensor[i].battery_ok ? "OK " : "Low");
        Serial.print(F(" Ch: "));
        Serial.print(ws.sensor[i].chan);
        
        if (ws.sensor[i].s_type == SENSOR_TYPE_SOIL) {
            Serial.print(F(" Temp [C]: "));
            Serial.print(ws.sensor[i].soil.temp_c, 1);
            Serial.print(F(" Moisture [%]: "));
            Serial.print(ws.sensor[i].soil.moisture);

        } else if ((ws.sensor[i].s_type == SENSOR_TYPE_WEATHER0) || (ws.sensor[i].s_type == SENSOR_TYPE_WEATHER1)) {

            if (ws.sensor[i].w.temp_ok) {
                Serial.print(F(" Temp [C]: "));
                Serial.print(ws.sensor[i].w.temp_c, 1);
            } else {
                Serial.print(F(" Temp [C]: ---.- "));
            }
            if (ws.sensor[i].w.humidity_ok) {
                Serial.print(F(" Hum [%]: "));
                Serial.print(ws.sensor[i].w.humidity);
            }
            else {
                Serial.print(F(" Hum [%]: --- "));
            }
            
            if (ws.sensor[i].w.wind_ok) {
                Serial.print(F(" Wind max [m/s]: "));
                Serial.print(ws.sensor[i].w.wind_gust_meter_sec_fp1/10.0, 1);
                Serial.print(F(" Wind avg [m/s]: "));
                Serial.print(ws.sensor[i].w.wind_avg_meter_sec_fp1/10.0, 1);
                Serial.print(F(" Wind dir [deg]: "));
                Serial.print(ws.sensor[i].w.wind_direction_deg_fp1/10.0, 1);
            } else {
                Serial.print(F(" Wind max [m/s]: --.- Wind avg [m/s]: --.- Wind dir [deg]: ---.- "));
            }
            if (ws.sensor[i].w.rain_ok) {
                Serial.print(F(" Rain [mm]: "));
                Serial.print(ws.sensor[i].w.rain_mm, 1);
            } else {
                Serial.print(F(" Rain [mm]: -----.- ")); 
            }
            #if defined BRESSER_6_IN_1 || defined BRESSER_7_IN_1
            if (ws.sensor[i].w.uv_ok) {
                Serial.print(F(" UV index [%]: "));
                Serial.print(ws.sensor[i].w.uv, 1);
            } else {
                Serial.print(F(" UV index [%]: -.- "));
            }
            #endif
            #ifdef BRESSER_7_IN_1
            if (ws.sensor[i].w.light_ok) {
                Serial.print(F(" Light [klux]:"));
                Serial.print(ws.sensor[i].w.light_klx, 1);
            }
            else {
                Serial.print(F(" Light [klux]: --.- "));
            }
            #endif
        }
      Serial.print(F(" RSSI [dBm]: "));
      Serial.println(ws.sensor[i].rssi, 1);
      
    } // if (decode_status == DECODE_OK)

    delay(100);
} // loop()
