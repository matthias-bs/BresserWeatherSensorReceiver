///////////////////////////////////////////////////////////////////////////////////////////////////
// mqtt_comm.cpp
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
// Copyright (c) 2026 Matthias Prinke
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
// 20250227 Added publishControlDiscovery()
// 20250228 Added publishStatusDiscovery(), fixed sensorName()
// 20250420 Added timestamp to measurement data, fixed base-topic in extra data
// 20250728 Added combined (Weather & Soil Sensor) MQTT payload
// 20250801 Added Lightning Sensor to combined MQTT payload
// 20250802 Refactored publishWeatherdata() to use ArduinoJson
// 20260113 Fixed HA auto-discovery for UV Index, Light Lux, Wind Direction, 
//          Wind Direction (Cardinal) and Wind Average/Gust Speed (Beaufort)
//          Changed JSON keys from light_klx/ws_light_klx to light_lx/ws_light_lx
//          (values are in Lux)
// 20260221 Refactored publishStatusDiscovery() and publishControlDiscovery()
//          to use JsonDocument and snprintf instead of raw string concatenation
//          Refactored MQTT topic building in publishWeatherdata() to use snprintf
//          instead of String concatenation to reduce temporary object allocations
//          Changed global MQTT topic strings from String to const char* to reduce
//          persistent heap memory usage (~600-750 bytes savings)
//          Refactored MQTT topic declarations using MQTTTopics struct for cleaner
//          organization and maintainability
// 20260403 Fixed latent bug: DATA_TIMESTAMP block used undefined 'sensorData' instead of 'jsonSensor'
//          Removed unused/shadowed outer 'String topic' in haAutoDiscovery()
//          Removed redundant payload.clear() in publishRadio() (local JsonDocument auto-destroyed)
//          Replaced String payloadSensor/Extra/Combined with stack-allocated char[] to eliminate
//          heap fragmentation from repeated substring() copies on every publish cycle// 20260403 Issue 9: Eliminated all String heap allocations in haAutoDiscovery() and helper
//          functions (publishAutoDiscovery, publishStatusDiscovery, publishControlDiscovery).
//          Replaced String topic/rssi with stack char[]+snprintf, changed sensor_info members
//          to const char*, updated function signatures to const char* where applicable.//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "mqtt_comm.h"

extern String Hostname;
extern MQTTTopics mqttTopics;

extern MQTTClient client;
extern WeatherSensor weatherSensor;
extern RainGauge rainGauge;
extern Lightning lightning;
extern std::vector<SensorMap> sensor_map;

void sensorName(uint32_t sensor_id, char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "%x", (unsigned)sensor_id);
    for (size_t n = 0; n < sensor_map.size(); n++)
    {
        if (sensor_map[n].id == sensor_id)
        {
            snprintf(buf, buf_size, "%s", sensor_map[n].name.c_str());
            break;
        }
    }
}

// MQTT message received callback
void messageReceived(String &topic, String &payload)
{
    if (topic == mqttTopics.subReset)
    {
        uint8_t flags = payload.toInt() & 0xFF;
        log_d("MQTT msg received: reset(0x%X)", flags);
        rainGauge.reset(flags);
        if (flags & 0x10)
        {
            lightning.reset();
        }
    }
    else if (topic == mqttTopics.subGetInc)
    {
        log_d("MQTT msg received: get_sensors_inc");
        client.publish(mqttTopics.pubInc, weatherSensor.getSensorsIncJson());
    }
    else if (topic == mqttTopics.subGetExc)
    {
        log_d("MQTT msg received: get_sensors_exc");
        client.publish(mqttTopics.pubExc, weatherSensor.getSensorsExcJson());
    }
    else if (topic == mqttTopics.subSetInc)
    {
        log_d("MQTT msg received: set_sensors_inc");
        weatherSensor.setSensorsIncJson(payload);
    }
    else if (topic == mqttTopics.subSetExc)
    {
        log_d("MQTT msg received: set_sensors_exc");
        weatherSensor.setSensorsExcJson(payload);
    }
    else
    {
        log_d("MQTT msg received: %s", topic.c_str());
    }
}


// Publish weather data as MQTT message
void publishWeatherdata(bool complete, bool retain)
{
    JsonDocument jsonCombined;
    JsonObject combinedStatus = jsonCombined["status"].to<JsonObject>();
    JsonDocument jsonSensor;
    JsonDocument jsonExtra;

    // Stack-allocated buffers avoid heap fragmentation from repeated String alloc/free.
    // Constraints:
    //   - Stack usage: PAYLOAD_SIZE*2 + PAYLOAD_EXTRA_SIZE bytes (~960 B). Safe on ESP32 (8 KB stack).
    //     Do not increase PAYLOAD_SIZE when compiling for ESP8266 (4 KB stack limit).
    //   - Adding fields to jsonExtra must not exceed PAYLOAD_EXTRA_SIZE. See estimate in mqtt_comm.h.
    //   - serializeJson() with a size-limited buffer truncates silently; overflow detected via return value.
    char payloadSensor[PAYLOAD_SIZE];           // sensor data
    char payloadExtra[PAYLOAD_EXTRA_SIZE];      // calculated extra data (wind/temp/dewpoint derived values)
    char payloadCombined[PAYLOAD_SIZE];         // combined payload for ESP32-e-Paper-Weather-Display
    char mqtt_topic[256];   // MQTT topic including ID/name (increased size for all uses)

    for (size_t i = 0; i < weatherSensor.sensor.size(); i++)
    {

        if (!weatherSensor.sensor[i].valid)
            continue;

        if (weatherSensor.sensor[i].w.rain_ok)
        {
            struct tm timeinfo;
            time_t now = time(nullptr);
            localtime_r(&now, &timeinfo);
            rainGauge.update(now, weatherSensor.sensor[i].w.rain_mm, weatherSensor.sensor[i].startup);
        }
        jsonSensor.clear();
        jsonExtra.clear();

        // Example:
        // {"ch":0,"battery_ok":1,"humidity":44,"wind_gust":1.2,"wind_avg":1.2,"wind_dir":150,"rain":146}
        jsonSensor["id"] = weatherSensor.sensor[i].sensor_id;
        jsonSensor["ch"] = weatherSensor.sensor[i].chan;
        jsonSensor["battery_ok"] = weatherSensor.sensor[i].battery_ok;

#if defined(DATA_TIMESTAMP)
        {
            // Generate timestamp in ISO 8601 format
            time_t now = time(nullptr);
            struct tm timeinfo;
            gmtime_r(&now, &timeinfo); // Convert to UTC time
            char tbuf[25];
            strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo); // Format as ISO 8601
            jsonSensor["timestamp"] = tbuf;
        }
#endif // DATA_TIMESTAMP

        if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_SOIL)
        {
            jsonSensor["temp_c"] = weatherSensor.sensor[i].soil.temp_c;
            jsonSensor["moisture"] = weatherSensor.sensor[i].soil.moisture;

            jsonCombined["soil1_temp_c"] = weatherSensor.sensor[i].soil.temp_c;
            jsonCombined["soil1_moisture"] = weatherSensor.sensor[i].soil.moisture;
            combinedStatus["soil1_batt_ok"] = weatherSensor.sensor[i].battery_ok ? 1 : 0;
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LIGHTNING)
        {
            jsonSensor["lightning_count"] = weatherSensor.sensor[i].lgt.strike_count;
            jsonSensor["lightning_distance_km"] = weatherSensor.sensor[i].lgt.distance_km;
            char lgtUnknown1[12], lgtUnknown2[12];
            snprintf(lgtUnknown1, sizeof(lgtUnknown1), "0x%x", weatherSensor.sensor[i].lgt.unknown1);
            snprintf(lgtUnknown2, sizeof(lgtUnknown2), "0x%x", weatherSensor.sensor[i].lgt.unknown2);
            jsonSensor["lightning_unknown1"] = lgtUnknown1;
            jsonSensor["lightning_unknown2"] = lgtUnknown2;

            struct tm timeinfo;
            time_t now = time(nullptr);
            localtime_r(&now, &timeinfo);
            lightning.update(
                now,
                weatherSensor.sensor[i].lgt.strike_count,
                weatherSensor.sensor[i].lgt.distance_km,
                weatherSensor.sensor[i].startup);
            jsonSensor["lightning_hr"] = lightning.pastHour();
            int events;
            time_t timestamp;
            uint8_t distance;
            if (lightning.lastEvent(timestamp, events, distance))
            {
                char tbuf[25];
                struct tm timeinfo;
                gmtime_r(&timestamp, &timeinfo);
                strftime(tbuf, 25, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
                jsonSensor["lightning_event_time"] = tbuf;
                jsonSensor["lightning_event_count"] = events;
                jsonSensor["lightning_event_distance_km"] = distance;

                jsonCombined["lgt_ev_time"] = timestamp;
                jsonCombined["lgt_ev_events"] = events;
                jsonCombined["lgt_ev_dist_km"] = distance;
                combinedStatus["ls_batt_ok"] = weatherSensor.sensor[i].battery_ok ? 1 : 0;
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LEAKAGE)
        {
            // Water Leakage Sensor
            jsonSensor["leakage"] = weatherSensor.sensor[i].leak.alarm ? 1 : 0;
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_AIR_PM)
        {
            // Air Quality (Particular Matter) Sensor
            if (!weatherSensor.sensor[i].pm.pm_1_0_init)
            {
                jsonSensor["pm1_0_ug_m3"] = weatherSensor.sensor[i].pm.pm_1_0;
            }
            if (!weatherSensor.sensor[i].pm.pm_2_5_init)
            {
                jsonSensor["pm2_5_ug_m3"] = weatherSensor.sensor[i].pm.pm_2_5;
            }
            if (!weatherSensor.sensor[i].pm.pm_10_init)
            {
                jsonSensor["pm10_ug_m3"] = weatherSensor.sensor[i].pm.pm_10;
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_CO2)
        {
            // CO2 Sensor
            if (!weatherSensor.sensor[i].co2.co2_init)
            {
                jsonSensor["co2_ppm"] = weatherSensor.sensor[i].co2.co2_ppm;
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_HCHO_VOC)
        {
            // HCHO / VOC Sensor
            if (!weatherSensor.sensor[i].voc.hcho_init)
            {
                jsonSensor["hcho_ppb"] = weatherSensor.sensor[i].voc.hcho_ppb;
            }
            if (!weatherSensor.sensor[i].voc.voc_init)
            {
                jsonSensor["voc"] = weatherSensor.sensor[i].voc.voc_level;
            }
        }
        else if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER3) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER8) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_THERMO_HYGRO) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_POOL_THERMO))
        {
            if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
                (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
                (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER3) ||
                (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER8))
            {
                combinedStatus["ws_batt_ok"] = weatherSensor.sensor[i].battery_ok ? 1 : 0;
            }
            if (weatherSensor.sensor[i].w.temp_ok || complete)
            {
                jsonSensor["temp_c"] = weatherSensor.sensor[i].w.temp_c;
                jsonCombined["ws_temp_c"] = weatherSensor.sensor[i].w.temp_c;
            }
            if (weatherSensor.sensor[i].w.humidity_ok || complete)
            {
                jsonSensor["humidity"] = weatherSensor.sensor[i].w.humidity;
                jsonCombined["ws_humidity"] = weatherSensor.sensor[i].w.humidity;
            }
            if (weatherSensor.sensor[i].w.wind_ok || complete)
            {
                jsonSensor["wind_gust"] = weatherSensor.sensor[i].w.wind_gust_meter_sec;
                jsonSensor["wind_avg"] = weatherSensor.sensor[i].w.wind_avg_meter_sec;
                jsonSensor["wind_dir"] = weatherSensor.sensor[i].w.wind_direction_deg;
                jsonCombined["ws_wind_gust_ms"] = weatherSensor.sensor[i].w.wind_gust_meter_sec;
                jsonCombined["ws_wind_avg_ms"] = weatherSensor.sensor[i].w.wind_avg_meter_sec;
                jsonCombined["ws_wind_dir_deg"] = weatherSensor.sensor[i].w.wind_direction_deg;
            }
            if (weatherSensor.sensor[i].w.wind_ok)
            {
                char buf[4];
                jsonExtra["wind_dir_txt"] = winddir_flt_to_str(weatherSensor.sensor[i].w.wind_direction_deg, buf, sizeof(buf));
                jsonExtra["wind_gust_bft"] = windspeed_ms_to_bft(weatherSensor.sensor[i].w.wind_gust_meter_sec);
                jsonExtra["wind_avg_bft"] = windspeed_ms_to_bft(weatherSensor.sensor[i].w.wind_avg_meter_sec);
            }
            if ((weatherSensor.sensor[i].w.temp_ok) && (weatherSensor.sensor[i].w.humidity_ok))
            {
                jsonExtra["dewpoint_c"] = calcdewpoint(weatherSensor.sensor[i].w.temp_c, weatherSensor.sensor[i].w.humidity);

                if (weatherSensor.sensor[i].w.wind_ok)
                {
                    jsonExtra["perceived_temp_c"] = perceived_temperature(weatherSensor.sensor[i].w.temp_c, weatherSensor.sensor[i].w.wind_avg_meter_sec, weatherSensor.sensor[i].w.humidity);
                }
                if (weatherSensor.sensor[i].w.tglobe_ok)
                {
                    float t_wet = calcnaturalwetbulb(weatherSensor.sensor[i].w.temp_c, weatherSensor.sensor[i].w.humidity);
                    jsonExtra["wgbt"] = calcwbgt(t_wet, weatherSensor.sensor[i].w.tglobe_c, weatherSensor.sensor[i].w.temp_c);

                }
            }
            if (weatherSensor.sensor[i].w.uv_ok || complete)
            {
                jsonSensor["uv"] = weatherSensor.sensor[i].w.uv;
                jsonCombined["ws_uv"] = weatherSensor.sensor[i].w.uv;
            }
            if (weatherSensor.sensor[i].w.light_ok || complete)
            {
                jsonSensor["light_lx"] = weatherSensor.sensor[i].w.light_lux;
                jsonCombined["ws_light_lx"] = weatherSensor.sensor[i].w.light_lux;
            }
            if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER8)
            {
                if (weatherSensor.sensor[i].w.tglobe_ok || complete)
                {
                    jsonSensor["t_globe_c"] = weatherSensor.sensor[i].w.tglobe_c;
                    jsonCombined["ws_t_globe_c"] = weatherSensor.sensor[i].w.tglobe_c;
                }
            }
            if (weatherSensor.sensor[i].w.rain_ok || complete)
            {
                jsonSensor["rain"] = weatherSensor.sensor[i].w.rain_mm;
                jsonSensor["rain_h"] = rainGauge.pastHour();
                jsonSensor["rain_d24h"] = rainGauge.past24Hours();
                jsonSensor["rain_d"] = rainGauge.currentDay();
                jsonSensor["rain_w"] = rainGauge.currentWeek();
                jsonSensor["rain_m"] = rainGauge.currentMonth();
                jsonCombined["ws_rain_mm"] = weatherSensor.sensor[i].w.rain_mm;
                jsonCombined["ws_rain_hourly_mm"] = rainGauge.pastHour();
                jsonCombined["ws_rain_24h_mm"] = rainGauge.past24Hours();
                jsonCombined["ws_rain_daily_mm"] = rainGauge.currentDay();
                jsonCombined["ws_rain_weekly_mm"] = rainGauge.currentWeek();
                jsonCombined["ws_rain_monthly_mm"] = rainGauge.currentMonth();
            }
        }
        size_t json_size = serializeJson(jsonSensor, payloadSensor, sizeof(payloadSensor));
        size_t extra_size = serializeJson(jsonExtra, payloadExtra, sizeof(payloadExtra));

        if (json_size >= PAYLOAD_SIZE - 1)
        {
            log_e("payloadSensor (%d) >= PAYLOAD_SIZE (%d). Payload truncated!", json_size, PAYLOAD_SIZE);
        }
        if (extra_size >= PAYLOAD_EXTRA_SIZE - 1)
        {
            log_e("payloadExtra (%d) >= PAYLOAD_EXTRA_SIZE (%d). Payload truncated!", extra_size, PAYLOAD_EXTRA_SIZE);
        }

        // Try to map sensor ID to name to make MQTT topic explanatory
        char sensor_str[32];
        sensorName(weatherSensor.sensor[i].sensor_id, sensor_str, sizeof(sensor_str));

        // sensor data
        snprintf(mqtt_topic, sizeof(mqtt_topic), "%s/%s/%s", 
                 Hostname.c_str(), sensor_str, mqttTopics.pubData);
        log_i("%s: %s\n", mqtt_topic, payloadSensor);
        client.publish(mqtt_topic, payloadSensor, retain, 0);

        // sensor specific RSSI
        snprintf(mqtt_topic, sizeof(mqtt_topic), "%s/%s/%s",
                 Hostname.c_str(), sensor_str, mqttTopics.pubRssi);
        char rssiStr[12];
        snprintf(rssiStr, sizeof(rssiStr), "%.1f", weatherSensor.sensor[i].rssi);
        client.publish(mqtt_topic, rssiStr, false, 0);

        // extra data
        snprintf(mqtt_topic, sizeof(mqtt_topic), "%s/%s",
                 Hostname.c_str(), mqttTopics.pubExtra);

        if (strcmp(payloadExtra, "null") != 0)
        {
            // extra data
            log_i("%s: %s\n", mqtt_topic, payloadExtra);
            client.publish(mqtt_topic, payloadExtra, retain, 0);
        }
    } // for (int i=0; i<weatherSensor.sensor.size(); i++)

    size_t combined_size = serializeJson(jsonCombined, payloadCombined, sizeof(payloadCombined));
    if (combined_size >= PAYLOAD_SIZE - 1)
    {
        log_e("payloadCombined (%d) >= PAYLOAD_SIZE (%d). Payload truncated!", combined_size, PAYLOAD_SIZE);
    }
    snprintf(mqtt_topic, sizeof(mqtt_topic), "%s/%s",
             Hostname.c_str(), mqttTopics.pubCombined);
    log_i("%s: %s\n", mqtt_topic, payloadCombined);
    client.publish(mqtt_topic, payloadCombined, retain, 0);
}

// Publish radio receiver info as JSON string via MQTT
// - RSSI: Received Signal Strength Indication
void publishRadio(void)
{
    JsonDocument payload;
    char mqtt_payload[32]; // {"rssi":-XXX.X} fits comfortably in 32 bytes

    payload["rssi"] = weatherSensor.rssi;
    serializeJson(payload, mqtt_payload, sizeof(mqtt_payload));
    log_i("%s: %s\n", mqttTopics.pubRadio, mqtt_payload);
    client.publish(mqttTopics.pubRadio, mqtt_payload, false, 0);
}

// Home Assistant Auto-Discovery
void haAutoDiscovery(void)
{
    for (size_t i = 0; i < weatherSensor.sensor.size(); i++)
    {
        uint32_t sensor_id = weatherSensor.sensor[i].sensor_id;

        if (!weatherSensor.sensor[i].valid)
            continue;

        char sensor_str[32];
        sensorName(weatherSensor.sensor[i].sensor_id, sensor_str, sizeof(sensor_str));
        // Stack-allocated topic buffers avoid heap fragmentation from String concatenation.
        char topicData[128], topicRssi[128], topicExtra[128];
        snprintf(topicData,  sizeof(topicData),  "%s/%s/data",  Hostname.c_str(), sensor_str);
        snprintf(topicRssi,  sizeof(topicRssi),  "%s/%s/rssi",  Hostname.c_str(), sensor_str);
        snprintf(topicExtra, sizeof(topicExtra), "%s/extra",    Hostname.c_str());
        if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER3) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER8))
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Weather Sensor",
                .identifier = "weather_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "Outside Temperature", sensor_id, "temperature", "°C", topicData, "temp_c");
            publishAutoDiscovery(info, "Outside Humidity", sensor_id, "humidity", "%", topicData, "humidity");
            if (weatherSensor.sensor[i].w.tglobe_ok)
            {
                publishAutoDiscovery(info, "Globe Temperature", sensor_id, "temperature", "°C", topicData, "tglobe_c");
            }
            if (weatherSensor.sensor[i].w.uv_ok)
            {
                publishAutoDiscovery(info, "UV Index", sensor_id, NULL, "UV", topicData, "uv");
            }
            if (weatherSensor.sensor[i].w.light_ok)
            {
                publishAutoDiscovery(info, "Light Lux", sensor_id, "illuminance", "lx", topicData, "light_lx");
            }
            if (weatherSensor.sensor[i].w.rain_ok)
            {
                publishAutoDiscovery(info, "Rainfall", sensor_id, "precipitation", "mm", topicData, "rain");
                publishAutoDiscovery(info, "Rainfall Hourly", sensor_id, "precipitation", "mm", topicData, "rain_h");
                publishAutoDiscovery(info, "Rainfall 24h", sensor_id, "precipitation", "mm", topicData, "rain_d24h");
                publishAutoDiscovery(info, "Rainfall Daily", sensor_id, "precipitation", "mm", topicData, "rain_d");
                publishAutoDiscovery(info, "Rainfall Weekly", sensor_id, "precipitation", "mm", topicData, "rain_w");
                publishAutoDiscovery(info, "Rainfall Monthly", sensor_id, "precipitation", "mm", topicData, "rain_m");
            }
            if (weatherSensor.sensor[i].w.wind_ok)
            {
                publishAutoDiscovery(info, "Wind Direction", sensor_id, "wind_direction", "°", topicData, "wind_dir");
                publishAutoDiscovery(info, "Wind Gust Speed", sensor_id, "wind_speed", "m/s", topicData, "wind_gust");
                publishAutoDiscovery(info, "Wind Average Speed", sensor_id, "wind_speed", "m/s", topicData, "wind_avg");
                publishAutoDiscovery(info, "Wind Gust Speed (Beaufort)", sensor_id, NULL, "Beaufort", topicExtra, "wind_gust_bft");
                publishAutoDiscovery(info, "Wind Average Speed (Beaufort)", sensor_id, NULL, "Beaufort", topicExtra, "wind_avg_bft");
                publishAutoDiscovery(info, "Wind Direction (Cardinal)", sensor_id, NULL, NULL, topicExtra, "wind_dir_txt");
            }
            if (weatherSensor.sensor[i].w.wind_ok &&
                weatherSensor.sensor[i].w.temp_ok &&
                weatherSensor.sensor[i].w.humidity_ok)
            {
                publishAutoDiscovery(info, "Dewpoint", sensor_id, "temperature", "°C", topicExtra, "dewpoint_c");
                publishAutoDiscovery(info, "Perceived Temperature", sensor_id, "temperature", "°C", topicExtra, "perceived_temp_c");
                if (weatherSensor.sensor[i].w.tglobe_ok)
                {
                    publishAutoDiscovery(info, "WGBT", sensor_id, "temperature", "°C", topicExtra, "wgbt");
                }
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_SOIL)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Soil Sensor",
                .identifier = "soil_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "Soil Temperature", sensor_id, "temperature", "°C", topicData, "temp_c");
            publishAutoDiscovery(info, "Soil Moisture", sensor_id, "moisture", "%", topicData, "moisture");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_THERMO_HYGRO)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Thermo-Hygrometer Sensor",
                .identifier = "thermo_hygrometer_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "Temperature", sensor_id, "temperature", "°C", topicData, "temp_c");
            publishAutoDiscovery(info, "Humidity", sensor_id, "humidity", "%", topicData, "humidity");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_POOL_THERMO)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Pool Thermometer",
                .identifier = "pool_thermometer_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "Pool Temperature", sensor_id, "temperature", "°C", topicData, "temp_c");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_AIR_PM)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Air Quality (PM) Sensor",
                .identifier = "air_quality_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "PM1.0", sensor_id, "pm1", "µg/m³", topicData, "pm1_0_ug_m3");
            publishAutoDiscovery(info, "PM2.5", sensor_id, "pm25", "µg/m³", topicData, "pm2_5_ug_m3");
            publishAutoDiscovery(info, "PM10", sensor_id, "pm10", "µg/m³", topicData, "pm10_ug_m3");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LIGHTNING)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Lightning Sensor",
                .identifier = "lightning_sensor"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "Lightning Count", sensor_id, NULL, "", topicData, "lightning_count");
            publishAutoDiscovery(info, "Lightning Distance", sensor_id, "distance", "km", topicData, "lightning_distance_km");
            publishAutoDiscovery(info, "Lightning Hour", sensor_id, NULL, "", topicData, "lightning_hr");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LEAKAGE)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Leakage Sensor",
                .identifier = "leakage_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "Leakage Alarm", sensor_id, "enum", "", topicData, "leakage");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_CO2)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "CO2 Sensor",
                .identifier = "co2_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "CO2", sensor_id, "co2", "ppm", topicData, "co2_ppm");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_HCHO_VOC)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Air Quality (HCHO/VOC) Sensor",
                .identifier = "air_quality_sensor_2"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topicData, "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", topicRssi, "rssi");
            publishAutoDiscovery(info, "HCHO", sensor_id, "hcho", "ppb", topicData, "hcho_ppb");
            publishAutoDiscovery(info, "VOC", sensor_id, "voc", "", topicData, "voc");
        }
    } // for (int i=0; i<weatherSensor.sensor.size(); i++)

    publishControlDiscovery("Sensor Exclude List", "sensors_exc");
    publishControlDiscovery("Sensor Include List", "sensors_inc");
    publishStatusDiscovery("Receiver Status", "status");
}

// Publish discovery message for MQTT node status
void publishStatusDiscovery(const char* name, const char* topic)
{
    char discoveryTopic[256];
    snprintf(discoveryTopic, sizeof(discoveryTopic), "homeassistant/sensor/%s/%s/config",
             Hostname.c_str(), topic);

    JsonDocument doc;
    doc["name"] = name;
    char uniqueId[80];
    snprintf(uniqueId, sizeof(uniqueId), "%s_%s", Hostname.c_str(), topic);
    doc["unique_id"] = uniqueId;
    char stateTopic[80];
    snprintf(stateTopic, sizeof(stateTopic), "%s/%s", Hostname.c_str(), topic);
    doc["state_topic"] = stateTopic;
    doc["value_template"] = "{{ value }}";
    doc["icon"] = "mdi:wifi";
    JsonObject device = doc["device"].to<JsonObject>();
    char identifiers[48];
    snprintf(identifiers, sizeof(identifiers), "%s_1", Hostname.c_str());
    device["identifiers"] = identifiers;
    device["name"] = "Weather Sensor Receiver";

    char discoveryPayload[512];
    serializeJson(doc, discoveryPayload);
    log_d("%s: %s", discoveryTopic, discoveryPayload);
    client.publish(discoveryTopic, discoveryPayload, false, 0);
}

// Publish discovery messages for receiver control
void publishControlDiscovery(const char* name, const char* topic)
{
    char discoveryTopic[256];

    // Sensor discovery
    snprintf(discoveryTopic, sizeof(discoveryTopic), "homeassistant/sensor/%s/%s/config",
             Hostname.c_str(), topic);

    JsonDocument doc;
    doc["name"] = name;
    char uniqueId[80];
    snprintf(uniqueId, sizeof(uniqueId), "%s_%s", Hostname.c_str(), topic);
    doc["unique_id"] = uniqueId;
    char stateTopic[80];
    snprintf(stateTopic, sizeof(stateTopic), "%s/%s", Hostname.c_str(), topic);
    doc["state_topic"] = stateTopic;
    doc["value_template"] = "{{ value_json.ids }}";
    doc["icon"] = "mdi:code-array";
    JsonObject device = doc["device"].to<JsonObject>();
    char identifiers[48];
    snprintf(identifiers, sizeof(identifiers), "%s_1", Hostname.c_str());
    device["identifiers"] = identifiers;
    device["name"] = "Weather Sensor Receiver";

    char discoveryPayload[512];
    serializeJson(doc, discoveryPayload);
    log_d("%s: %s", discoveryTopic, discoveryPayload);
    client.publish(discoveryTopic, discoveryPayload, true, 0);

    // Button discovery
    snprintf(discoveryTopic, sizeof(discoveryTopic), "homeassistant/button/%s/get_%s/config",
             Hostname.c_str(), topic);

    JsonDocument docButton;
    char buttonName[80];
    snprintf(buttonName, sizeof(buttonName), "Get %s", name);
    docButton["name"] = buttonName;
    docButton["platform"] = "button";
    char buttonUniqueId[80];
    snprintf(buttonUniqueId, sizeof(buttonUniqueId), "%s_get_%s", Hostname.c_str(), topic);
    docButton["unique_id"] = buttonUniqueId;
    char buttonCmdTopic[80];
    snprintf(buttonCmdTopic, sizeof(buttonCmdTopic), "%s/get_%s", Hostname.c_str(), topic);
    docButton["command_topic"] = buttonCmdTopic;
    docButton["icon"] = "mdi:information";
    docButton["retain"] = true;
    docButton["qos"] = 1;
    JsonObject deviceBtn = docButton["device"].to<JsonObject>();
    deviceBtn["identifiers"] = identifiers;
    deviceBtn["name"] = "Weather Sensor Receiver";

    serializeJson(docButton, discoveryPayload);
    log_d("%s: %s", discoveryTopic, discoveryPayload);
    client.publish(discoveryTopic, discoveryPayload, false, 0);
}

// Publish auto-discovery configuration for Home Assistant
void publishAutoDiscovery(const struct sensor_info info, const char *sensor_name, const uint32_t sensor_id, const char *device_class, const char *unit, const char *state_topic, const char *value_json)
{
    JsonDocument doc;

    doc["name"] = sensor_name;
    if (device_class != NULL)
        doc["device_class"] = device_class;
    char uniqueId[64];
    snprintf(uniqueId, sizeof(uniqueId), "%08x_%s", (unsigned)sensor_id, value_json);
    doc["unique_id"] = uniqueId;
    doc["state_topic"] = state_topic;
    char availTopic[64];
    snprintf(availTopic, sizeof(availTopic), "%s/status", Hostname.c_str());
    doc["availability_topic"] = availTopic;
    doc["payload_not_available"] = "dead"; // default: "offline"
    if (unit != NULL)
        doc["unit_of_measurement"] = unit;
    char valTmpl[128];
    if (device_class != NULL)
    {
        if (strcmp(device_class, "battery") == 0)
        {
            snprintf(valTmpl, sizeof(valTmpl), "{{ (value_json.%s | float) * 100.0 }}", value_json);
            doc["value_template"] = valTmpl;
        }
        else if (strcmp(device_class, "signal_strength") == 0)
        {
            doc["value_template"] = "{{ value }}";
        }
        else
        {
            snprintf(valTmpl, sizeof(valTmpl), "{{ value_json.%s }}", value_json);
            doc["value_template"] = valTmpl;
        }
    } else {
        snprintf(valTmpl, sizeof(valTmpl), "{{ value_json.%s }}", value_json);
        doc["value_template"] = valTmpl;
    }

    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"] = info.identifier;
    char deviceName[80];
    snprintf(deviceName, sizeof(deviceName), "%s %s", info.manufacturer, info.model);
    device["name"] = deviceName;
    if (info.model[0] != '\0')
        device["model"] = info.model;
    if (info.manufacturer[0] != '\0')
        device["manufacturer"] = info.manufacturer;

    char buffer[512];
    serializeJson(doc, buffer);

    char discTopic[128];
    snprintf(discTopic, sizeof(discTopic), "homeassistant/sensor/%08x_%s/config", (unsigned)sensor_id, value_json);
    log_d("Publishing auto-discovery configuration: %s: %s", discTopic, buffer);
    client.publish(discTopic, buffer, true /* retained */, 0 /* qos */);
    log_d("Published auto-discovery configuration for %s", sensor_name);
}
