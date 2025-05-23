///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorCallback.ino
//
// Example for BresserWeatherSensorReceiver -
// Using getData() for reception of at least one complete data set from a sensor.
//
// getData() blocks until the data has been received or a timeout occurs.
// Basically the same as in BresserWeatherSensorWaiting, but invokes a callback
// function while waiting.
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
// 20220523 Created from https://github.com/matthias-bs/Bresser5in1-CC1101
// 20220524 Moved code to class WeatherSensor
// 20220810 Changed to modified WeatherSensor class; fixed Soil Moisture Sensor Handling
// 20220815 Changed to modified WeatherSensor class; added support of multiple sensors
// 20221227 Replaced DEBUG_PRINT/DEBUG_PRINTLN by Arduino logging functions
// 20240507 Added configuration of maximum number of sensors at run time
// 20250127 Added Globe Thermometer Temperature (8-in-1 Weather Sensor)
// 20250510 Changed hardcoded receive timeout to define
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "InitBoard.h"

// Set RX_TIMEOUT depending on
// - Configured number of sensors
// - Sensors' transmit duty cycles
// - Reception failure rate
#define RX_TIMEOUT 60000 // sensor receive timeout [ms]

WeatherSensor ws;

// Example for callback function which is executed while waiting for radio messages
void loopCallback(void)
{
    // Normally something really important would be done here
    Serial.print(".");
}

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    initBoard();
    ws.begin();
}

void loop()
{
    // Clear all sensor data
    ws.clearSlots();

    // Attempt to receive entire data set with timeout of <xxx> ms and callback function
    bool decode_ok = ws.getData(RX_TIMEOUT, DATA_COMPLETE, 0, &loopCallback);
    Serial.println();

    if (!decode_ok)
    {
        Serial.printf("Sensor timeout\n");
    }
    for (size_t i = 0; i < ws.sensor.size(); i++)
    {
        Serial.printf("Id: [%8X] Typ: [%X] Ch: [%d] St: [%d] Bat: [%-3s] RSSI: [%6.1fdBm] ",
                      (unsigned int)ws.sensor[i].sensor_id,
                      ws.sensor[i].s_type,
                      ws.sensor[i].chan,
                      ws.sensor[i].startup,
                      ws.sensor[i].battery_ok ? "OK " : "Low",
                      ws.sensor[i].rssi);

        if (ws.sensor[i].s_type == SENSOR_TYPE_LIGHTNING)
        {
            // Lightning Sensor
            Serial.printf("Lightning Counter: [%3d] ", ws.sensor[i].lgt.strike_count);
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
            Serial.printf("PM2.5: [%uµg/m³] ", ws.sensor[i].pm.pm_2_5);
            Serial.printf("PM10: [%uµg/m³]\n", ws.sensor[i].pm.pm_10);
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
                Serial.printf("UVidx: [%1.1f] ",
                              ws.sensor[i].w.uv);
            }
            else
            {
                Serial.printf("UVidx: [-.-%%] ");
            }
#endif
#ifdef BRESSER_7_IN_1
            if (ws.sensor[i].w.light_ok)
            {
                Serial.printf("Light: [%2.1fKlux] ",
                              ws.sensor[i].w.light_klx);
            }
            else
            {
                Serial.printf("Light: [--.-Klux] ");
            }
            if (ws.sensor[i].s_type == SENSOR_TYPE_WEATHER2) {
                if (ws.sensor[i].w.tglobe_ok) {
                    Serial.printf("T_globe: [%3.1fC] ",
                    ws.sensor[i].w.tglobe_c);
                }
                else {
                    Serial.printf("T_globe: [--.-C] ");
                }
            }
#endif
            Serial.printf("\n");

        } // if (decode_status == DECODE_OK)
    }
    delay(100);
} // loop()
