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
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "mqtt_comm.h"

extern String Hostname;
extern String mqttPubData;
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

extern const SensorMap *sensor_map;

String sensorName(uint32_t sensor_id)
{
    String sensor_str = String(sensor_id, HEX);
    if (sizeof(sensor_map) > 0)
    {
        for (size_t n = 0; n < sizeof(sensor_map) / sizeof(sensor_map[0]); n++)
        {
            if (sensor_map[n].id == sensor_id)
            {
                sensor_str = String(sensor_map[n].name.c_str());
                break;
            }
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
void publishWeatherdata(bool complete)
{
    String mqtt_payload;  // sensor data
    String mqtt_payload2; // calculated extra data
    String mqtt_topic;    // MQTT topic including ID/name

    // ArduinoJson does not allow to set number of decimals for floating point data -
    // neither does MQTT Dashboard...
    // Therefore the JSON string is created manually.

    for (size_t i = 0; i < weatherSensor.sensor.size(); i++)
    {
        // Reset string buffers
        mqtt_payload = "";
        mqtt_payload2 = "";

        if (!weatherSensor.sensor[i].valid)
            continue;

        if (weatherSensor.sensor[i].w.rain_ok)
        {
            struct tm timeinfo;
            time_t now = time(nullptr);
            localtime_r(&now, &timeinfo);
            rainGauge.update(now, weatherSensor.sensor[i].w.rain_mm, weatherSensor.sensor[i].startup);
        }

        // Example:
        // {"ch":0,"battery_ok":1,"humidity":44,"wind_gust":1.2,"wind_avg":1.2,"wind_dir":150,"rain":146}
        mqtt_payload = "{";
        mqtt_payload2 = "{";
        mqtt_payload += String("\"id\":") + String(weatherSensor.sensor[i].sensor_id);
        mqtt_payload += String(",\"ch\":") + String(weatherSensor.sensor[i].chan);
        mqtt_payload += String(",\"battery_ok\":") + (weatherSensor.sensor[i].battery_ok ? String("1") : String("0"));

        if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_SOIL)
        {
            mqtt_payload += String(",\"temp_c\":") + String(weatherSensor.sensor[i].soil.temp_c);
            mqtt_payload += String(",\"moisture\":") + String(weatherSensor.sensor[i].soil.moisture);
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LIGHTNING)
        {
            mqtt_payload += String(",\"lightning_count\":") + String(weatherSensor.sensor[i].lgt.strike_count);
            mqtt_payload += String(",\"lightning_distance_km\":") + String(weatherSensor.sensor[i].lgt.distance_km);
            mqtt_payload += String(",\"lightning_unknown1\":\"0x") + String(weatherSensor.sensor[i].lgt.unknown1, HEX) + String("\"");
            mqtt_payload += String(",\"lightning_unknown2\":\"0x") + String(weatherSensor.sensor[i].lgt.unknown2, HEX) + String("\"");
            struct tm timeinfo;
            time_t now = time(nullptr);
            localtime_r(&now, &timeinfo);
            lightning.update(
                now,
                weatherSensor.sensor[i].lgt.strike_count,
                weatherSensor.sensor[i].lgt.distance_km,
                weatherSensor.sensor[i].startup);
            mqtt_payload += String(",\"lightning_hr\":") + String(lightning.pastHour());
            int events;
            time_t timestamp;
            uint8_t distance;
            if (lightning.lastEvent(timestamp, events, distance))
            {
                char tbuf[25];
                struct tm timeinfo;
                gmtime_r(&timestamp, &timeinfo);
                strftime(tbuf, 25, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
                mqtt_payload += String(",\"lightning_event_time\":\"") + String(tbuf) + String("\"");
                mqtt_payload += String(",\"lightning_event_count\":") + String(events);
                mqtt_payload += String(",\"lightning_event_distance_km\":") + String(distance);
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LEAKAGE)
        {
            // Water Leakage Sensor
            mqtt_payload += String(",\"leakage\":") + String(weatherSensor.sensor[i].leak.alarm);
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_AIR_PM)
        {
            // Air Quality (Particular Matter) Sensor
            if (!weatherSensor.sensor[i].pm.pm_1_0_init)
            {
                mqtt_payload += String(",\"pm1_0_ug_m3\":") + String(weatherSensor.sensor[i].pm.pm_1_0);
            }
            if (!weatherSensor.sensor[i].pm.pm_2_5_init)
            {
                mqtt_payload += String(",\"pm2_5_ug_m3\":") + String(weatherSensor.sensor[i].pm.pm_2_5);
            }
            if (!weatherSensor.sensor[i].pm.pm_10_init)
            {
                mqtt_payload += String(",\"pm10_ug_m3\":") + String(weatherSensor.sensor[i].pm.pm_10);
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_CO2)
        {
            // CO2 Sensor
            if (!weatherSensor.sensor[i].co2.co2_init)
            {
                mqtt_payload += String(",\"co2_ppm\":") + String(weatherSensor.sensor[i].co2.co2_ppm);
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_HCHO_VOC)
        {
            // HCHO / VOC Sensor
            if (!weatherSensor.sensor[i].voc.hcho_init)
            {
                mqtt_payload += String(",\"hcho_ppb\":") + String(weatherSensor.sensor[i].voc.hcho_ppb);
            }
            if (!weatherSensor.sensor[i].voc.voc_init)
            {
                mqtt_payload += String(",\"voc\":") + String(weatherSensor.sensor[i].voc.voc_level);
            }
        }
        else if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER2) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_THERMO_HYGRO) ||
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_POOL_THERMO))
        {

            if (weatherSensor.sensor[i].w.temp_ok || complete)
            {
                mqtt_payload += String(",\"temp_c\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.temp_c, 1));
            }
            if (weatherSensor.sensor[i].w.humidity_ok || complete)
            {
                mqtt_payload += String(",\"humidity\":") + String(weatherSensor.sensor[i].w.humidity);
            }
            if (weatherSensor.sensor[i].w.wind_ok || complete)
            {
                mqtt_payload += String(",\"wind_gust\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.wind_gust_meter_sec, 1));
                mqtt_payload += String(",\"wind_avg\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.wind_avg_meter_sec, 1));
                mqtt_payload += String(",\"wind_dir\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.wind_direction_deg, 1));
            }
            if (weatherSensor.sensor[i].w.wind_ok)
            {
                char buf[4];
                mqtt_payload2 += String("\"wind_dir_txt\":\"") + String(winddir_flt_to_str(weatherSensor.sensor[i].w.wind_direction_deg, buf)) + "\"";
                mqtt_payload2 += String(",\"wind_gust_bft\":") + String(windspeed_ms_to_bft(weatherSensor.sensor[i].w.wind_gust_meter_sec));
                mqtt_payload2 += String(",\"wind_avg_bft\":") + String(windspeed_ms_to_bft(weatherSensor.sensor[i].w.wind_avg_meter_sec));
            }
            if ((weatherSensor.sensor[i].w.temp_ok) && (weatherSensor.sensor[i].w.humidity_ok))
            {
                mqtt_payload2 += String(",\"dewpoint_c\":") + JSON_FLOAT(String(calcdewpoint(weatherSensor.sensor[i].w.temp_c, weatherSensor.sensor[i].w.humidity), 1));

                if (weatherSensor.sensor[i].w.wind_ok)
                {
                    mqtt_payload2 += String(",\"perceived_temp_c\":") + JSON_FLOAT(String(perceived_temperature(weatherSensor.sensor[i].w.temp_c, weatherSensor.sensor[i].w.wind_avg_meter_sec, weatherSensor.sensor[i].w.humidity), 1));
                }
                if (weatherSensor.sensor[i].w.tglobe_ok)
                {
                    float t_wet = calcnaturalwetbulb(weatherSensor.sensor[i].w.temp_c, weatherSensor.sensor[i].w.humidity);
                    float wbgt = calcwbgt(t_wet, weatherSensor.sensor[i].w.tglobe_c, weatherSensor.sensor[i].w.temp_c);
                    mqtt_payload2 += String(",\"wgbt\":") + JSON_FLOAT(String(wbgt, 1));
                }
            }
            if (weatherSensor.sensor[i].w.uv_ok || complete)
            {
                mqtt_payload += String(",\"uv\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.uv, 1));
            }
            if (weatherSensor.sensor[i].w.light_ok || complete)
            {
                mqtt_payload += String(",\"light_klx\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.light_klx, 1));
            }
            if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER2)
            {
                if (weatherSensor.sensor[i].w.tglobe_ok || complete)
                {
                    mqtt_payload += String(",\"t_globe_c\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.tglobe_c, 1));
                }
            }
            if (weatherSensor.sensor[i].w.rain_ok || complete)
            {
                mqtt_payload += String(",\"rain\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.rain_mm, 1));
                mqtt_payload += String(",\"rain_h\":") + JSON_FLOAT(String(rainGauge.pastHour(), 1));
                mqtt_payload += String(",\"rain_d\":") + JSON_FLOAT(String(rainGauge.currentDay(), 1));
                mqtt_payload += String(",\"rain_w\":") + JSON_FLOAT(String(rainGauge.currentWeek(), 1));
                mqtt_payload += String(",\"rain_m\":") + JSON_FLOAT(String(rainGauge.currentMonth(), 1));
            }
        }
        mqtt_payload += String("}");
        mqtt_payload2 += String("}");

        if (mqtt_payload.length() >= PAYLOAD_SIZE)
        {
            log_e("mqtt_payload (%d) > PAYLOAD_SIZE (%d). Payload will be truncated!", mqtt_payload.length(), PAYLOAD_SIZE);
        }
        if (mqtt_payload2.length() >= PAYLOAD_SIZE)
        {
            log_e("mqtt_payload2 (%d) > PAYLOAD_SIZE (%d). Payload will be truncated!", mqtt_payload2.length(), PAYLOAD_SIZE);
        }

        // Try to map sensor ID to name to make MQTT topic explanatory
        String sensor_str = sensorName(weatherSensor.sensor[i].sensor_id);

        String mqtt_topic_base = Hostname + '/' + sensor_str + '/';
        String mqtt_topic;

        // sensor data
        mqtt_topic = mqtt_topic_base + mqttPubData;

        log_i("%s: %s\n", mqtt_topic.c_str(), mqtt_payload.c_str());
        client.publish(mqtt_topic, mqtt_payload.substring(0, PAYLOAD_SIZE - 1), false, 0);

        // sensor specific RSSI
        mqtt_topic = mqtt_topic_base + mqttPubRssi;
        client.publish(mqtt_topic, String(weatherSensor.sensor[i].rssi, 1), false, 0);

        // extra data
        mqtt_topic = mqttPubExtra;

        if (mqtt_payload2.length() > 2)
        {
            log_i("%s: %s\n", mqtt_topic.c_str(), mqtt_payload2.c_str());
            client.publish(mqtt_topic, mqtt_payload2.substring(0, PAYLOAD_SIZE - 1), false, 0);
        }
    } // for (int i=0; i<weatherSensor.sensor.size(); i++)
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

#if defined(AUTO_DISCOVERY)
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
        if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
            (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER2))
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Weather Sensor",
                .identifier = "weather_sensor_1"};

            publishAutoDiscovery(info, "Outside Temperature", sensor_id, "temperature", "°C", topic.c_str(), "temp_c");
            publishAutoDiscovery(info, "Outside Humidity", sensor_id, "humidity", "%", topic.c_str(), "humidity");
            if (weatherSensor.sensor[i].w.tglobe_ok)
            {
                publishAutoDiscovery(info, "Globe Temperature", sensor_id, "temperature", "°C", topic.c_str(), "tglobe_c");
            }
            if (weatherSensor.sensor[i].w.uv_ok)
            {
                publishAutoDiscovery(info, "UV Index", sensor_id, "uv", "UV Index", topic.c_str(), "uv");
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
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_SOIL)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Soil Sensor",
                .identifier = "soil_sensor_1"};

            publishAutoDiscovery(info, "Soil Temperature", sensor_id, "temperature", "°C", topic.c_str(), "temp_c");
            publishAutoDiscovery(info, "Soil Moisture", sensor_id, "moisture", "%", topic.c_str(), "moisture");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_THERMO_HYGRO)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Thermo-Hygrometer Sensor",
                .identifier = "thermo_hygrometer_sensor_1"};

            publishAutoDiscovery(info, "Temperature", sensor_id, "temperature", "°C", topic.c_str(), "temp_c");
            publishAutoDiscovery(info, "Humidity", sensor_id, "humidity", "%", topic.c_str(), "humidity");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_POOL_THERMO)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Pool Thermometer",
                .identifier = "pool_thermometer_1"};

            publishAutoDiscovery(info, "Pool Temperature", sensor_id, "temperature", "°C", topic.c_str(), "temp_c");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_AIR_PM)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Air Quality (PM) Sensor",
                .identifier = "air_quality_sensor_1"};

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

            publishAutoDiscovery(info, "Leakage Alarm", sensor_id, "enum", "", topic.c_str(), "leakage");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_CO2)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "CO2 Sensor",
                .identifier = "co2_sensor_1"};

            publishAutoDiscovery(info, "CO2", sensor_id, "co2", "ppm", topic.c_str(), "co2_ppm");
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_HCHO_VOC)
        {
            struct sensor_info info = {
                .manufacturer = "Bresser",
                .model = "Air Quality (HCHO/VOC) Sensor",
                .identifier = "air_quality_sensor_2"};

            publishAutoDiscovery(info, "HCHO", sensor_id, "hcho", "ppb", topic.c_str(), "hcho_ppb");
            publishAutoDiscovery(info, "VOC", sensor_id, "voc", "", topic.c_str(), "voc");
        }
    } // for (int i=0; i<weatherSensor.sensor.size(); i++)
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
    doc["unit_of_measurement"] = unit;
    doc["value_template"] = String("{{ value_json.") + value_json + " }}";

    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"] = info.identifier;
    device["name"] = info.manufacturer + " " + info.model;
    device["model"] = info.model;
    device["manufacturer"] = info.manufacturer;

    char buffer[512];
    serializeJson(doc, buffer);

    String topic = String("homeassistant/sensor/") + String(sensor_id, HEX) + "_" + String(value_json) + "/config";
    log_d("Publishing auto-discovery configuration: %s: %s", topic.c_str(), buffer);
    client.publish(topic, buffer, true /* retained */, 0 /* qos */);
    log_d("Published auto-discovery configuration for %s", sensor_name);
}
#endif // AUTO_DISCOVERY
