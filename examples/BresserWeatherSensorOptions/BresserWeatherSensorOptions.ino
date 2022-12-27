///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorOptions.ino
//
// Example for BresserWeatherSensorReceiver - Test getData() options
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
// 20220815 Created
// 20221227 Replaced DEBUG_PRINT/DEBUG_PRINTLN by Arduino logging functions
//
// ToDo: 
// - 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

// Set TIMEOUT to a relative small value to see the behaviour of different options -
// depending on the timing of sensor transmission, start of getData() call and selected option,
// getData() may succeed or encounter a timeout.
// For practical use, set TIMEOUT large enough that getData() will succeed in most cases.
#define TIMEOUT 45000 // Timeout in ms 

WeatherSensor weatherSensor;

void print_data(void)
{
  for (int i=0; i<NUM_SENSORS; i++) {
    if (!weatherSensor.sensor[i].valid)
      continue;
      
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
    Serial.printf("RSSI: [%6.1fdBm]\n", weatherSensor.sensor[i].rssi);
  } // for (int i=0; i<NUM_SENSORS; i++)
} // void print_data(void)

void setup() {    
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    weatherSensor.begin();
}


void loop() 
{    
    bool decode_ok;
    
    // Clear all sensor data
    weatherSensor.clearSlots();
  
    // Try to receive data from at least one sensor,
    // finish even if data is incomplete. 
    // (Data can be distributed across multiple radio messages.)
    decode_ok = weatherSensor.getData(TIMEOUT);

    Serial.println();
    Serial.println("1 -- Flags: (none)");
    Serial.println("--------------------------------------------------------------");
    if (!decode_ok) {
      Serial.printf("Sensor timeout\n");
    } else {
      print_data();
    }
    
    
    // Try to receive data from at least one sensor, 
    // finish only if data is complete.
    // (Data can be distributed across multiple radio messages.)
    weatherSensor.clearSlots();
    decode_ok = weatherSensor.getData(TIMEOUT, DATA_COMPLETE);

    Serial.println();
    Serial.println("2 -- Flags: DATA_COMPLETE");
    Serial.println("--------------------------------------------------------------");
    if (!decode_ok) {
      Serial.printf("Sensor timeout\n");
    } else {
      print_data();
    }

    // Try to receive data from (at least) one specific sensor type, 
    // finish only if data is complete.
    // (Data can be distributed across multiple radio messages.)
    weatherSensor.clearSlots();
    decode_ok = weatherSensor.getData(TIMEOUT, DATA_TYPE | DATA_COMPLETE, SENSOR_TYPE_WEATHER1);

    Serial.println();
    Serial.println("3 -- Flags: DATA_TYPE | DATA_COMPLETE, Type: SENSOR_TYPE_WEATHER1");
    Serial.println("-----------------------------------------------------------------");
    if (!decode_ok) {
      Serial.printf("Sensor timeout\n");
    } else {
      print_data();
    }

    // Try to receive data from (at least) one specific sensor type, 
    // finish only if data is complete.
    // (Data can be distributed across multiple radio messages.)
    weatherSensor.clearSlots();
    decode_ok = weatherSensor.getData(TIMEOUT, DATA_TYPE | DATA_COMPLETE, SENSOR_TYPE_SOIL);

    Serial.println();
    Serial.println("4 -- Flags: DATA_TYPE | DATA_COMPLETE, Type: SENSOR_TYPE_SOIL");
    Serial.println("-------------------------------------------------------------");
    if (!decode_ok) {
        Serial.printf("Sensor timeout\n");
    } else {
      print_data();
    }

    // Try to receive data from all (NUM_SENSORS) sensors, 
    // finish only if data is complete.
    // (Data can be distributed across multiple radio messages.)
    weatherSensor.clearSlots();
    decode_ok = weatherSensor.getData(TIMEOUT, DATA_ALL_SLOTS);

    Serial.println();
    Serial.println("5 -- Flags: DATA_ALL_SLOTS");
    Serial.println("--------------------------------------------------------------");
    if (!decode_ok) {
        Serial.printf("Sensor timeout\n");
    } else {
      print_data();
    }
        
    delay(100);
} // loop()
