///////////////////////////////////////////////////////////////////////////////
// datacake_uplink_decoder.js
// 
// Datacake MQTT decoder for BresserWeatherSensorMQTT data
//
// This script decodes MQTT messages to Datacake's "data fields".
//
// - In the Datacake dashboard, copy the script to
//   <device>->Configuration->Product & Hardware->MQTT Configuration
// - Replace the topics and device_id with your own
//
// created: 07/2025
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
//
// History:
// 20250712 Created
//
///////////////////////////////////////////////////////////////////////////////
function Decoder(topic, payload) {
    var basetopic = "ESPWeather-789ABC/";
    var top_weather = "WeatherSensor/data";
    var top_soil = "SoilSensor/data";
    var top_lightning = "LightningSensor/data";

    // Serial Number or Device ID
    var device_id = "ac0fa860-75fd-49a0-9065-fdf740729ed1";

    payload = JSON.parse(payload);

    if (topic == basetopic + top_weather) {

        return [
            {
                device: device_id,
                field: "WS_BATTERY_OK",
                value: payload.battery_ok
            },
            {
                device: device_id,
                field: "WS_TEMP_C",
                value: payload.temp_c
            },
            {
                device: device_id,
                field: "WS_HUMIDITY",
                value: payload.humidity
            },
            {
                device: device_id,
                field: "WS_WIND_GUST_MS",
                value: payload.wind_gust
            },
            {
                device: device_id,
                field: "WS_WIND_AVG_MS",
                value: payload.wind_avg
            },
            {
                device: device_id,
                field: "WS_WIND_DIR_DEG",
                value: payload.wind_dir
            },
            {
                device: device_id,
                field: "WS_RAIN_MM",
                value: payload.rain
            },
            {
                device: device_id,
                field: "WS_RAIN_HOURLY_MM",
                value: payload.rain_h
            },
            {
                device: device_id,
                field: "WS_RAIN_DAILY_MM",
                value: payload.rain_d
            },
            {
                device: device_id,
                field: "WS_RAIN_WEEKLY_MM",
                value: payload.rain_w
            },
            {
                device: device_id,
                field: "WS_RAIN_MONTLY_MM",
                value: payload.rain_m
            }
        ];

    } else if (topic == basetopic + top_soil) {
        return [
            {
                device: device_id,
                field: "SOIL1_MOISTURE",
                value: payload.moisture
            },
            {
                device: device_id,
                field: "SOIL1_TEMP_C",
                value: payload.temp_c
            }
        ];

    } else if (topic == basetopic + top_lightning) {
        return [
            {
                device: device_id,
                field: "LGT_EV_EVENTS",
                value: payload.lightning_count
            },
            {
                device: device_id,
                field: "LGT_EV_DIST_KM",
                value: payload.lightning_distance_km
            },
            {
                device: device_id,
                field: "LGT_EV_HR",
                value: payload.lightning_hr
            }
        ];
    }
}
