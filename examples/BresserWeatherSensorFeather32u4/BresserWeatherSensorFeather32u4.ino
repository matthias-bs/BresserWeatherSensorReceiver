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
//
// ToDo: 
// - 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"


WeatherSensor weatherSensor;


void setup() {    
    Serial.begin(115200);

    Serial.println(F("Starting execution..."));

    weatherSensor.begin();
}


void loop() 
{   
    // This example uses only a single slot in the sensor data array
    int const i=0;

    // Clear all sensor data
    weatherSensor.clearSlots();

    // Tries to receive radio message (non-blocking) and to decode it.
    // Timeout occurs after a small multiple of expected time-on-air.
    int decode_status = weatherSensor.getMessage();
  
    if (decode_status == DECODE_OK) {
      Serial.print(F("Id: "));
      Serial.print(weatherSensor.sensor[i].sensor_id, HEX);
      Serial.print(F(" Typ: "));
      Serial.print(weatherSensor.sensor[i].s_type);
      Serial.print(F(" Battery: "));
      Serial.print(weatherSensor.sensor[i].battery_ok ? "OK " : "Low");
      #ifdef BRESSER_6_IN_1
          Serial.print(F(" Ch: "));
          Serial.print(weatherSensor.sensor[i].chan);
      #endif
      if (weatherSensor.sensor[i].temp_ok) {
          Serial.print(F(" Temp [C]: "));
          Serial.print(weatherSensor.sensor[i].temp_c, 1);
      } else {
          Serial.print(F(" Temp [C]: ---.- "));
      }
      if (weatherSensor.sensor[i].humidity_ok) {
          Serial.print(F(" Hum [%]: "));
          Serial.print(weatherSensor.sensor[i].humidity);
      }
      else {
          Serial.print(F(" Hum [%]: --- "));
      }
      
      if (weatherSensor.sensor[i].wind_ok) {
          Serial.print(F(" Wind max [m/s]: "));
          Serial.print(weatherSensor.sensor[i].wind_gust_meter_sec_fp1/10.0, 1);
          Serial.print(F(" Wind avg [m/s]: "));
          Serial.print(weatherSensor.sensor[i].wind_avg_meter_sec_fp1/10.0, 1);
          Serial.print(F(" Wind dir [deg]: "));
          Serial.print(weatherSensor.sensor[i].wind_direction_deg_fp1/10.0, 1);
      } else {
          Serial.print(F(" Wind max [m/s]: --.- Wind avg [m/s]: --.- Wind dir [deg]: ---.- "));
      }
      if (weatherSensor.sensor[i].rain_ok) {
          Serial.print(F(" Rain [mm]: "));
          Serial.print(weatherSensor.sensor[i].rain_mm, 1);
      } else {
          Serial.print(F(" Rain [mm]: -----.- ")); 
      }
      if (weatherSensor.sensor[i].moisture_ok) {
          Serial.print(F(" Moisture [%]: "));
          Serial.print(weatherSensor.sensor[i].moisture);
      } else {
          Serial.print(F(" Moisture [%]: -- "));
      }
      #if defined BRESSER_6_IN_1 || defined BRESSER_7_IN_1
      if (weatherSensor.sensor[i].uv_ok) {
          Serial.print(F(" UV index [%]: "));
          Serial.print(weatherSensor.sensor[i].uv, 1);
      } else {
          Serial.print(F(" UV index [%]: -.- "));
      }
      #endif
      #ifdef BRESSER_7_IN_1
      if (weatherSensor.sensor[i].light_ok) {
          Serial.print(F(" Light [klux]:"));
          Serial.print(weatherSensor.sensor[i].light_klx, 1);
      }
      else {
          Serial.print(F(" Light [klux]: --.- "));
      }
      #endif
      
      Serial.print(F(" RSSI [dBm]: "));
      Serial.println(weatherSensor.sensor[i].rssi, 1);
      
    } // if (decode_status == DECODE_OK)

    delay(100);
} // loop()
