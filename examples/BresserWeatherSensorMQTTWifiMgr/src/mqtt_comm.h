///////////////////////////////////////////////////////////////////////////////////////////////////
// mqtt_comm.h
//
// MQTT communication
// Code shared between BresserWeaterSensor<MQTT|MQTTCustom|MQTTWifiMgr>.ino
//
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 02/2025
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
//
// 20250221 Created from BresserWeatherSensorMQTT.ino
// 20250226 Added parameter 'retain' to publishWeatherdata()
// 20250227 Added publishControlDiscovery()
// 20250420 removed AUTO_DISCOVERY here, as it is defined in sketch
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MQTT_COMM_H
#define MQTT_COMM_H

#define PAYLOAD_SIZE 300      // maximum MQTT message size
//#define AUTO_DISCOVERY        // enable Home Assistant auto discovery

#include <Arduino.h>
#include <string>
#include <vector>
#include <time.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "WeatherUtils.h"
#include "RainGauge.h"
#include "Lightning.h"

// See
// https://stackoverflow.com/questions/19554972/json-standard-floating-point-numbers
// and
// https://stackoverflow.com/questions/35709595/why-would-you-use-a-string-in-json-to-represent-a-decimal-number
//
// Summary:
// A string representation of a float (e.g. "temp_c":"21.5") is recommended if the value shall displayed with the specified number of decimals.
// Otherwise the float value can be output as a numerical value (e.g. "temp_c":21.5).
//
// #define JSON_FLOAT_AS_STRING

#if defined(JSON_FLOAT_AS_STRING)
#define JSON_FLOAT(x) String("\"") + x + String("\"")
#else
#define JSON_FLOAT(x) x
#endif

extern void mqtt_setup(void);

// Sensor information for Home Assistant auto discovery
struct sensor_info
{
    String manufacturer;
    String model;
    String identifier;
};

/*!
 * \brief (Re-)Connect to WLAN and connect MQTT broker
 */
//void mqtt_connect(void)

/*!
 * \brief MQTT message received callback
 *
 * \param topic   MQTT topic
 * \param payload MQTT payload
 */
void messageReceived(String &topic, String &payload);

/*!
 * \brief Publish weather data as MQTT message
 *
 * \param complete Indicate that entire data is complete, regardless of the flags temp_ok/wind_ok/rain_ok
 *                 (which reflect only the state of the last message)
 */
void publishWeatherdata(bool complete = false, bool retain = false);

/*!
 * \brief Publish radio receiver info as JSON string via MQTT
 *
 * Publish RSSI: Received Signal Strength Indication
 */
void publishRadio(void);

#if defined(AUTO_DISCOVERY)
/*!
 * \brief Home Assistant Auto-Discovery
 *
 * Create and publish MQTT messages for Home Assistant auto-discovery.
 */
void haAutoDiscovery(void);

/*!
 * \brief Publish auto-discovery configuration for Home Assistant
 *
 * \param info          Sensor information (manufacturer, model, identifier)
 * \param sensor_name   Sensor name (e.g. "Outside Temperature")
 * \param sensor_id     Sensor ID (unique)
 * \param device_class  Device class (e.g. temperature, humidity, etc.)
 * \param unit          Unit of measurement
 * \param state_topic   State topic; MQTT topic where sensor data is published
 * \param value_json    Sensor value in MQTT message JSON string
 */
void publishAutoDiscovery(const struct sensor_info info, const char *sensor_name, const uint32_t sensor_id, const char *device_class, const char *unit, const char *state_topic, const char *value_json);

/*!
 * \brief Publish Home Assistant auto discovery for MQTT node status
 *
 * \param name  Control name
 * \param topic MQTT topic
 */
void publishStatusDiscovery(String name, String topic);

/*!
 * \brief Publish Home Assistant auto discovery for receiver control
 *
 * \param name  Control name
 * \param topic MQTT topic
 */
void publishControlDiscovery(String name, String topic);

#endif // AUTO_DISCOVERY
#endif // MQTT_COMM_H
