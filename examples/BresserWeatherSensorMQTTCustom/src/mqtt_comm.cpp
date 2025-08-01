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
// 20250227 Added publishControlDiscovery()
// 20250228 Added publishStatusDiscovery(), fixed sensorName()
// 20250420 Added timestamp to measurement data, fixed base-topic in extra data
// 20250728 Added combined (Weather & Soil Sensor) MQTT payload
// 20250801 Added Lightning Sensor to combined MQTT payload
// 20250802 Refactored publishWeatherdata() to use ArduinoJson
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "mqtt_comm.h"

extern String Hostname;
extern String mqttPubData;
extern String mqttPubCombined;
extern String mqttPubRssi;
extern String mqttPubStatus;
extern String mqttPubRadio;
extern String mqttPubExtra;
extern String mqttPubInc;
extern String mqttPubExc;
extern String mqttSubReset;
extern String mqttSubGetInc;
extern String mqttSubGetExc;
extern String mqttSubSetInc;
extern String mqttSubSetExc;

extern MQTTClient client;
extern WeatherSensor weatherSensor;
extern RainGauge rainGauge;
extern Lightning lightning;
extern std::vector<SensorMap> sensor_map;

String sensorName(uint32_t sensor_id)
{
    String sensor_str = String(sensor_id, HEX);

    for (size_t n = 0; n < sensor_map.size(); n++)
    {
        if (sensor_map[n].id == sensor_id)
        {
            sensor_str = sensor_map[n].name;
            break;
        }
    }
    return sensor_str;
}

// MQTT message received callback
void messageReceived(String &topic, String &payload)
{
    if (topic == mqttSubReset)
    {
        uint8_t flags = payload.toInt() & 0xFF;
        log_d("MQTT msg received: reset(0x%X)", flags);
        rainGauge.reset(flags);
        if (flags & 0x10)
        {
            lightning.reset();
        }
    }
    else if (topic == mqttSubGetInc)
    {
        log_d("MQTT msg received: get_sensors_inc");
        client.publish(mqttPubInc, weatherSensor.getSensorsIncJson());
    }
    else if (topic == mqttSubGetExc)
    {
        log_d("MQTT msg received: get_sensors_exc");
        client.publish(mqttPubExc, weatherSensor.getSensorsExcJson());
    }
    else if (topic == mqttSubSetInc)
    {
        log_d("MQTT msg received: set_sensors_inc");
        weatherSensor.setSensorsIncJson(payload);
    }
    else if (topic == mqttSubSetExc)
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

    String payloadSensor;   // sensor data
    String payloadExtra;    // calculated extra data
    String payloadCombined; // combined payload for ESP32-e-Paper-Weather-Display
    String mqtt_topic;      // MQTT topic including ID/name

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
        JsonDocument jsonSensor;
        JsonDocument jsonExtra;

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
            sensorData["timestamp"] = tbuf;
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
            jsonSensor["lightning_unknown1"] = String("0x") + String(weatherSensor.sensor[i].lgt.unknown1, HEX);
            jsonSensor["lightning_unknown2"] = String("0x") + String(weatherSensor.sensor[i].lgt.unknown2, HEX);

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
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER2) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_THERMO_HYGRO) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_POOL_THERMO))
        {
            if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
                (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
                (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER2))
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
                jsonExtra["wind_dir_txt"] = winddir_flt_to_str(weatherSensor.sensor[i].w.wind_direction_deg, buf);
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
                jsonSensor["light_klx"] = weatherSensor.sensor[i].w.light_lux;
                jsonCombined["ws_light_klx"] = weatherSensor.sensor[i].w.light_lux;
            }
            if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER2)
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
                jsonSensor["rain_d"] = rainGauge.currentDay();
                jsonSensor["rain_w"] = rainGauge.currentWeek();
                jsonSensor["rain_m"] = rainGauge.currentMonth();
                jsonCombined["ws_rain_mm"] = weatherSensor.sensor[i].w.rain_mm;
                jsonCombined["ws_rain_hourly_mm"] = rainGauge.pastHour();
                jsonCombined["ws_rain_daily_mm"] = rainGauge.currentDay();
                jsonCombined["ws_rain_weekly_mm"] = rainGauge.currentWeek();
                jsonCombined["ws_rain_monthly_mm"] = rainGauge.currentMonth();
            }
        }
        serializeJson(jsonSensor, payloadSensor);
        serializeJson(jsonExtra, payloadExtra);

        if (payloadSensor.length() >= PAYLOAD_SIZE)
        {
            log_e("payloadSensor (%d) > PAYLOAD_SIZE (%d). Payload will be truncated!", payloadSensor.length(), PAYLOAD_SIZE);
        }
        if (payloadExtra.length() >= PAYLOAD_SIZE)
        {
            log_e("payloadExtra (%d) > PAYLOAD_SIZE (%d). Payload will be truncated!", payloadExtra.length(), PAYLOAD_SIZE);
        }

        // Try to map sensor ID to name to make MQTT topic explanatory
        String sensor_str = sensorName(weatherSensor.sensor[i].sensor_id);

        String mqtt_topic_base = Hostname + '/' + sensor_str + '/';
        String mqtt_topic;

        // sensor data
        mqtt_topic = mqtt_topic_base + mqttPubData;

        log_i("%s: %s\n", mqtt_topic.c_str(), payloadSensor.c_str());
        client.publish(mqtt_topic, payloadSensor.substring(0, PAYLOAD_SIZE - 1), retain, 0);

        // sensor specific RSSI
        mqtt_topic = mqtt_topic_base + mqttPubRssi;
        client.publish(mqtt_topic, String(weatherSensor.sensor[i].rssi, 1), false, 0);

        // extra data
        mqtt_topic = Hostname + '/' + mqttPubExtra;

        if (payloadExtra != "null")
        {
            // extra data
            log_i("%s: %s\n", mqtt_topic.c_str(), payloadExtra.c_str());
            client.publish(mqtt_topic, payloadExtra.substring(0, PAYLOAD_SIZE - 1), retain, 0);
        }
    } // for (int i=0; i<weatherSensor.sensor.size(); i++)

    serializeJson(jsonCombined, payloadCombined);
    if (payloadCombined.length() >= PAYLOAD_SIZE)
    {
        log_e("payloadCombined (%d) > PAYLOAD_SIZE (%d). Payload will be truncated!", payloadCombined.length(), PAYLOAD_SIZE);
    }
    mqtt_topic = Hostname + '/' + mqttPubCombined;
    log_i("%s: %s\n", mqtt_topic.c_str(), payloadCombined.c_str());
    client.publish(mqtt_topic, payloadCombined.substring(0, PAYLOAD_SIZE - 1), retain, 0);
}

// Publish radio receiver info as JSON string via MQTT
// - RSSI: Received Signal Strength Indication
void publishRadio(void)
{
    JsonDocument payload;
    String mqtt_payload;

    payload["rssi"] = weatherSensor.rssi;
    serializeJson(payload, mqtt_payload);
    log_i("%s: %s\n", mqttPubRadio.c_str(), mqtt_payload.c_str());
    client.publish(mqttPubRadio, mqtt_payload, false, 0);
    payload.clear();
}

// Home Assistant Auto-Discovery
void haAutoDiscovery(void)
{
    String topic;

    for (size_t i = 0; i < weatherSensor.sensor.size(); i++)
    {
        uint32_t sensor_id = weatherSensor.sensor[i].sensor_id;

        if (!weatherSensor.sensor[i].valid)
            continue;

        String sensor_str = sensorName(weatherSensor.sensor[i].sensor_id);
        String topic = Hostname + "/" + sensor_str + "/data";
        String rssi = Hostname + "/" + sensor_str + "/rssi";
        if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER2))
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Weather Sensor",
                .identifier = "weather_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "Outside Temperature", sensor_id, "temperature", "°C", topic.c_str(), "temp_c");
            publishAutoDiscovery(info, "Outside Humidity", sensor_id, "humidity", "%", topic.c_str(), "humidity");
            if (weatherSensor.sensor[i].w.tglobe_ok)
            {
                publishAutoDiscovery(info, "Globe Temperature", sensor_id, "temperature", "°C", topic.c_str(), "tglobe_c");
            }
            if (weatherSensor.sensor[i].w.uv_ok)
            {
                publishAutoDiscovery(info, "UV Index", sensor_id, NULL, "UV Index", topic.c_str(), "uv");
            }
            if (weatherSensor.sensor[i].w.light_ok)
            {
                publishAutoDiscovery(info, "Light Lux", sensor_id, "illuminance", "Lux", topic.c_str(), "light_lux");
            }
            if (weatherSensor.sensor[i].w.rain_ok)
            {
                publishAutoDiscovery(info, "Rainfall", sensor_id, "precipitation", "mm", topic.c_str(), "rain");
                publishAutoDiscovery(info, "Rainfall Hourly", sensor_id, "precipitation", "mm", topic.c_str(), "rain_h");
                publishAutoDiscovery(info, "Rainfall Daily", sensor_id, "precipitation", "mm", topic.c_str(), "rain_d");
                publishAutoDiscovery(info, "Rainfall Weekly", sensor_id, "precipitation", "mm", topic.c_str(), "rain_w");
                publishAutoDiscovery(info, "Rainfall Monthly", sensor_id, "precipitation", "mm", topic.c_str(), "rain_m");
            }
            if (weatherSensor.sensor[i].w.wind_ok)
            {
                publishAutoDiscovery(info, "Wind Direction", sensor_id, NULL, "°", topic.c_str(), "wind_dir");
                publishAutoDiscovery(info, "Wind Gust Speed", sensor_id, "wind_speed", "m/s", topic.c_str(), "wind_gust");
                publishAutoDiscovery(info, "Wind Average Speed", sensor_id, "wind_speed", "m/s", topic.c_str(), "wind_avg");
                String topic = Hostname + "/extra";
                publishAutoDiscovery(info, "Wind Gust Speed (Beaufort)", sensor_id, "wind_speed", "Beaufort", topic.c_str(), "wind_gust_bft");
                publishAutoDiscovery(info, "Wind Average Speed (Beaufort)", sensor_id, "wind_speed", "Beaufort", topic.c_str(), "wind_avg_bft");
                publishAutoDiscovery(info, "Wind Direction (Cardinal)", sensor_id, "enum", "", topic.c_str(), "wind_dir_txt");
            }
            if (weatherSensor.sensor[i].w.wind_ok &&
                weatherSensor.sensor[i].w.temp_ok &&
                weatherSensor.sensor[i].w.humidity_ok)
            {
                String topic = Hostname + "/extra";
                publishAutoDiscovery(info, "Dewpoint", sensor_id, "temperature", "°C", topic.c_str(), "dewpoint_c");
                publishAutoDiscovery(info, "Perceived Temperature", sensor_id, "temperature", "°C", topic.c_str(), "perceived_temp_c");
                if (weatherSensor.sensor[i].w.tglobe_ok)
                {
                    publishAutoDiscovery(info, "WGBT", sensor_id, "temperature", "°C", topic.c_str(), "wgbt");
                }
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_SOIL)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Soil Sensor",
                .identifier = "soil_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "Soil Temperature", sensor_id, "temperature", "°C", topic.c_str(), "temp_c");
            publishAutoDiscovery(info, "Soil Moisture", sensor_id, "moisture", "%", topic.c_str(), "moisture");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_THERMO_HYGRO)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Thermo-Hygrometer Sensor",
                .identifier = "thermo_hygrometer_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "Temperature", sensor_id, "temperature", "°C", topic.c_str(), "temp_c");
            publishAutoDiscovery(info, "Humidity", sensor_id, "humidity", "%", topic.c_str(), "humidity");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_POOL_THERMO)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Pool Thermometer",
                .identifier = "pool_thermometer_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "Pool Temperature", sensor_id, "temperature", "°C", topic.c_str(), "temp_c");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_AIR_PM)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Air Quality (PM) Sensor",
                .identifier = "air_quality_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "PM1.0", sensor_id, "pm1", "µg/m³", topic.c_str(), "pm1_0_ug_m3");
            publishAutoDiscovery(info, "PM2.5", sensor_id, "pm25", "µg/m³", topic.c_str(), "pm2_5_ug_m3");
            publishAutoDiscovery(info, "PM10", sensor_id, "pm10", "µg/m³", topic.c_str(), "pm10_ug_m3");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LIGHTNING)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Lightning Sensor",
                .identifier = "lightning_sensor"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "Lightning Count", sensor_id, NULL, "", topic.c_str(), "lightning_count");
            publishAutoDiscovery(info, "Lightning Distance", sensor_id, "distance", "km", topic.c_str(), "lightning_distance_km");
            publishAutoDiscovery(info, "Lightning Hour", sensor_id, NULL, "", topic.c_str(), "lightning_hr");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LEAKAGE)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Leakage Sensor",
                .identifier = "leakage_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "Leakage Alarm", sensor_id, "enum", "", topic.c_str(), "leakage");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_CO2)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "CO2 Sensor",
                .identifier = "co2_sensor_1"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "CO2", sensor_id, "co2", "ppm", topic.c_str(), "co2_ppm");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_HCHO_VOC)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Air Quality (HCHO/VOC) Sensor",
                .identifier = "air_quality_sensor_2"};

            publishAutoDiscovery(info, "Battery", sensor_id, "battery", "%", topic.c_str(), "battery_ok");
            publishAutoDiscovery(info, "RSSI", sensor_id, "signal_strength", "dBm", rssi.c_str(), "rssi");
            publishAutoDiscovery(info, "HCHO", sensor_id, "hcho", "ppb", topic.c_str(), "hcho_ppb");
            publishAutoDiscovery(info, "VOC", sensor_id, "voc", "", topic.c_str(), "voc");
        }
    } // for (int i=0; i<weatherSensor.sensor.size(); i++)

    publishControlDiscovery("Sensor Exclude List", "sensors_exc");
    publishControlDiscovery("Sensor Include List", "sensors_inc");
    publishStatusDiscovery("Receiver Status", "status");
}

// Publish discovery message for MQTT node status
void publishStatusDiscovery(String name, String topic)
{
    String discoveryTopic = "homeassistant/sensor/" + Hostname + "/" + topic + "/config";
    String discoveryPayload = R"({
        "name": ")" + name + R"(",
        "unique_id":")" + Hostname +
                              "_" + topic + R"(",
        "state_topic": ")" + Hostname +
                              R"(/)" + topic + R"(",
        "value_template": "{{ value }}",
        "icon": "mdi:wifi",
        "device": {
            "identifiers": ")" +
                              Hostname + R"(_1",
            "name": "Weather Sensor Receiver"
        }
    })";
    log_d("%s: %s", discoveryTopic.c_str(), discoveryPayload.c_str());
    client.publish(discoveryTopic.c_str(), discoveryPayload.c_str(), false, 0);
}

// Publish discovery messages for receiver control
void publishControlDiscovery(String name, String topic)
{
    String discoveryTopic = "homeassistant/sensor/" + Hostname + "/" + topic + "/config";
    String discoveryPayload = R"({
        "name": ")" + name + R"(",
        "unique_id":")" + Hostname +
                              "_" + topic + R"(",
        "state_topic": ")" + Hostname +
                              R"(/)" + topic + R"(",
        "value_template": "{{ value_json.ids }}",
        "icon": "mdi:code-array",
        "device": {
            "identifiers": ")" +
                              Hostname + R"(_1",
            "name": "Weather Sensor Receiver"
        }
    })";
    log_d("%s: %s", discoveryTopic.c_str(), discoveryPayload.c_str());
    client.publish(discoveryTopic.c_str(), discoveryPayload.c_str(), true, 0);

    discoveryTopic = "homeassistant/button/" + Hostname + "/get_" + topic + "/config";
    discoveryPayload = R"({
        "name": "Get )" +
                       name + R"(",
        "platform": "button",
        "unique_id": ")" +
                       Hostname + "_get_" + topic + R"(",
        "command_topic": ")" +
                       Hostname + "/get_" + topic + R"(",
        "icon": "mdi:information",
        "retain": true,
        "qos": 1,
        "device": {
            "identifiers": ")" +
                       Hostname + R"(_1",
            "name": "Weather Sensor Receiver"
        }
    })";
    log_d("%s: %s", discoveryTopic.c_str(), discoveryPayload.c_str());
    client.publish(discoveryTopic.c_str(), discoveryPayload.c_str(), false, 0);
}

// Publish auto-discovery configuration for Home Assistant
void publishAutoDiscovery(const struct sensor_info info, const char *sensor_name, const uint32_t sensor_id, const char *device_class, const char *unit, const char *state_topic, const char *value_json)
{
    JsonDocument doc;

    doc["name"] = sensor_name;
    if (device_class != NULL)
        doc["device_class"] = device_class;
    doc["unique_id"] = String(sensor_id, HEX) + String("_") + String(value_json);
    doc["state_topic"] = state_topic;
    doc["availability_topic"] = Hostname + "/status";
    doc["payload_not_available"] = "dead"; // default: "offline"
    doc["unit_of_measurement"] = unit;
    if (device_class != NULL)
    {
        if (strcmp(device_class, "battery") == 0)
        {
            doc["value_template"] = String("{{ (value_json.") + value_json + " | float) * 100.0 }}";
        }
        else if (strcmp(device_class, "signal_strength") == 0)
        {
            doc["value_template"] = "{{ value }}";
        }
        else
        {
            doc["value_template"] = String("{{ value_json.") + value_json + " }}";
        }
    }
    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"] = info.identifier;
    device["name"] = info.manufacturer + " " + info.model;
    if (info.model != "")
        device["model"] = info.model;
    if (info.manufacturer != "")
        device["manufacturer"] = info.manufacturer;

    char buffer[512];
    serializeJson(doc, buffer);

    String topic = String("homeassistant/sensor/") + String(sensor_id, HEX) + "_" + String(value_json) + "/config";
    log_d("Publishing auto-discovery configuration: %s: %s", topic.c_str(), buffer);
    client.publish(topic, buffer, true /* retained */, 0 /* qos */);
    log_d("Published auto-discovery configuration for %s", sensor_name);
}
