///////////////////////////////////////////////////////////////////////////////////////////////////
// WeatherSensorConfig.cpp
//
// Run-time configuration functions
//
// Bresser 5-in-1/6-in-1/7-in1 868 MHz Weather Sensor Radio Receiver
// based on CC1101 or SX1276/RFM95W and ESP32/ESP8266
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 05/2024
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
// 20240513 Created from WeatherSensor.cpp
// 20240608 Modified implementation of maximum number of sensors
// 20240609 Fixed implementation of maximum number of sensors
// 20240702 Fixed handling of empty list of IDs / 0x00000000 in Preferences
// 20241113 Added getting/setting of sensor include/exclude list from JSON strings
//
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <ArduinoJson.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

// Initialize list of sensor IDs
void WeatherSensor::initList(std::vector<uint32_t> &list, const std::vector<uint32_t> list_def, const char *key)
{
    list.clear();
    cfgPrefs.begin("BWS-CFG", false);
    log_d("Key %s in preferences? %d", key, cfgPrefs.isKey(key));

    if (cfgPrefs.isKey(key))
    {
        size_t size = cfgPrefs.getBytesLength(key);
        log_d("Using sensor_ids_%s list from Preferences (%d bytes)", key, size);
        uint8_t buf[48];
        cfgPrefs.getBytes(key, buf, size);
        if ((buf[0] | buf[1] | buf[2] | buf[3]) == 0)
        {
            size = 0;
            log_d("Empty list");
        }
        for (size_t i = 0; i < size; i += 4)
        {
            list.push_back(
                (buf[i] << 24) |
                (buf[i + 1] << 16) |
                (buf[i + 2] << 8) |
                buf[i + 3]
            );
        }
    }
    
    if (list.size() == 0)
    {
        log_d("Using sensor_ids_%s list from WeatherSensorCfg.h", key);
        list = list_def;
    }
    cfgPrefs.end();

    for (size_t i = 0; i < list.size(); i++)
    {
        log_d("%08X", list[i]);
    }
}

// Set sensors include list in Preferences
void WeatherSensor::setSensorsInc(uint8_t *buf, uint8_t size)
{
    log_d("size: %d", size);
    cfgPrefs.begin("BWS-CFG", false);
    cfgPrefs.putBytes("inc", buf, size);
    cfgPrefs.end();

    sensor_ids_inc.clear();
    if ((buf[0] | buf[1] | buf[2] | buf[3]) == 0)
    {
        size = 0;
    }
    for (size_t i = 0; i < size; i += 4)
    {
        sensor_ids_inc.push_back(
            (buf[i] << 24) |
            (buf[i + 1] << 16) |
            (buf[i + 2] << 8) |
            buf[i + 3]);
    }
}

// Get sensors include list from Preferences
uint8_t WeatherSensor::getSensorsInc(uint8_t *payload)
{
    for (const uint32_t &id : sensor_ids_inc)
    {
        for (int i = 3; i >= 0; i--)
        {
            *payload++ = (id >> (i * 8)) & 0xFF;
        }
    }

    return sensor_ids_inc.size() * 4;
}

// Set sensors exclude list in Preferences
void WeatherSensor::setSensorsExc(uint8_t *buf, uint8_t size)
{
    log_d("size: %d", size);
    cfgPrefs.begin("BWS-CFG", false);
    cfgPrefs.putBytes("exc", buf, size);
    cfgPrefs.end();

    sensor_ids_exc.clear();
    if ((buf[0] | buf[1] | buf[2] | buf[3]) == 0)
    {
        size = 0;
    }
    for (size_t i = 0; i < size; i += 4)
    {
        sensor_ids_exc.push_back(
            (buf[i] << 24) |
            (buf[i + 1] << 16) |
            (buf[i + 2] << 8) |
            buf[i + 3]);
    }
}

// Get sensors exclude list
uint8_t WeatherSensor::getSensorsExc(uint8_t *payload)
{
    for (const uint32_t &id : sensor_ids_exc)
    {
        for (int i = 3; i >= 0; i--)
        {
            *payload++ = (id >> (i * 8)) & 0xFF;
        }
    }

    return sensor_ids_exc.size() * 4;
}

// Get sensors include/exclude list as JSON string
String WeatherSensor::getSensorsJson(std::vector<uint32_t> &ids)
{
    JsonDocument doc;

    JsonArray data = doc["ids"].to<JsonArray>();
    for (size_t i = 0; i < ids.size(); i++)
    {
        String str = String(ids[i], HEX);
        // Ensure the hex number has 8 digits
        while (str.length() < 8)
        {
            str = "0" + str;
        }

        // Add "0x" prefix and enclose in double quotes
        str = "0x" + str;

        // Add to JSON array
        data.add(str);
    }

    String json;
    serializeJson(doc, json);
    return json;
}

// Get sensors include list as JSON string
String WeatherSensor::getSensorsIncJson(void)
{
    return getSensorsJson(sensor_ids_inc);
}

// Get sensors exclude list as JSON string
String WeatherSensor::getSensorsExcJson(void)
{
    return getSensorsJson(sensor_ids_exc);
}

// Convert JSON string to sensor IDs as byte array
uint8_t WeatherSensor::convSensorsJson(std::vector<uint32_t> &ids, String json, uint8_t *buf)
{
    JsonDocument doc;
    deserializeJson(doc, json);

    JsonArray data = doc["ids"].as<JsonArray>();
    ids.clear();
    for (size_t i = 0; (i < data.size()) && (i < MAX_SENSOR_IDS); i++)
    {
        String str = data[i].as<String>();
        log_d("ID: %s", str.c_str());
        for (size_t j=2; j < 10; j += 2) {
            String hexStr = str.substring(j, j + 2);
            *buf++ = (uint8_t)strtol(hexStr.c_str(), NULL, 16);
        }
    }
    return data.size() * 4;
}

// Set sensors include list from JSON string
void WeatherSensor::setSensorsIncJson(String json)
{
    uint8_t buf[MAX_SENSOR_IDS * 4];
    uint8_t size = convSensorsJson(sensor_ids_inc, json, buf);
    setSensorsInc(buf, size);
}

// Set sensors exclude list from JSON string
void WeatherSensor::setSensorsExcJson(String json)
{
    uint8_t buf[MAX_SENSOR_IDS * 4];
    uint8_t size = convSensorsJson(sensor_ids_exc, json, buf);
    setSensorsExc(buf, size);
}

// Set sensor configuration and store in in Preferences
void WeatherSensor::setSensorsCfg(uint8_t max_sensors, uint8_t rx_flags, uint8_t en_decoders)
{
    rxFlags = rx_flags;
    enDecoders = en_decoders;
    cfgPrefs.begin("BWS-CFG", false);
    cfgPrefs.putUChar("maxsensors", max_sensors);
    cfgPrefs.putUChar("rxflags", rx_flags);
    cfgPrefs.putUChar("endec", en_decoders);
    cfgPrefs.end();
    log_d("max_sensors: %u", max_sensors);
    log_d("rx_flags: %u", rxFlags);
    log_d("enabled_decoders: %u", enDecoders);
    sensor.resize(max_sensors);
}

// Get sensor configuration from Preferences
void WeatherSensor::getSensorsCfg(uint8_t &max_sensors, uint8_t &rx_flags, uint8_t &en_decoders)
{
    cfgPrefs.begin("BWS-CFG", false);
    max_sensors = cfgPrefs.getUChar("maxsensors", max_sensors);
    rx_flags = cfgPrefs.getUChar("rxflags", DATA_COMPLETE);
    en_decoders = cfgPrefs.getUChar("endec", 0xFF);
    cfgPrefs.end();
}