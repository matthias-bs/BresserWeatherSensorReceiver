///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorMQTTCustom.ino
//
// Example for BresserWeatherSensorReceiver -
// this is finally a useful application.
//
// At startup, first a WiFi connection and then a connection to the MQTT broker is established.
// (Edit secrets.h accordingly!)
//
// Then receiving data of all sensors (as defined in NUM_SENSORS, see WeatherSensorCfg.h)
// is tried periodically.
// If successful, sensor data is published as MQTT messages, one message per sensor.
// If the sensor ID can be mapped to a name (edit sensor_map[]), this name is used as the
// MQTT topic, otherwise the ID is used.
// From the sensor data, some additional data is calculated and published with the 'extra' topic.
//
// The data topics are published at an interval of >DATA_INTERVAL.
// The 'status' and the 'radio' topics are published at an interval of STATUS_INTERVAL.
//
// If sleep mode is enabled (SLEEP_EN), the device goes into deep sleep mode after data has
// been published. If AWAKE_TIMEOUT is reached before data has been published, deep sleep is
// entered, too. After SLEEP_INTERVAL, the controller is restarted.
//
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
// Based on:
// arduino-mqtt by Joël Gähwiler (256dpi) (https://github.com/256dpi/arduino-mqtt)
// ArduinoJson by Benoit Blanchon (https://arduinojson.org)
//
//
// MQTT publications:
//     <base_topic>/<ID|Name>/data                          sensor data as JSON string - see publishWeatherdata()
//     <base_topic>/<ID|Name>/rssi                          sensor specific RSSI
//     <base_topic>/extra                                   calculated data
//     <base_topic>/radio                                   radio transceiver info as JSON string - see publishRadio()
//     <base_topic>/status                                  "online"|"offline"|"dead"$
//     <base_topic>/sensors_inc                             sensors include list as JSON string;
//                                                          triggered by 'get_sensors_inc' MQTT topic
//     <base_topic>/sensors_exc                             sensors exclude list as JSON string;
//                                                          triggered by 'get_sensors_exc' MQTT topic
//
// MQTT subscriptions:
//     <base_topic>/reset <flags>                           reset rain counters (see RainGauge.h for <flags>)
//                                                          reset lightning post-processing (flags & 0x10)
//     <base_topic>/get_sensors_inc                         get sensors include list
//     <base_topic>/get_sensors_exc                         get sensors exclude list
//     <base_topic>/set_sensors_inc {"ids": [<id0>, ... ]}  set sensors include list, e.g. {"ids": ["0x89ABCDEF"]}
//     <base_topic>/set_sensors_exc {"ids": [<id0>, ... ]}  set sensors exclude list, e.g. {"ids": ["0x89ABCDEF"]}
//
// $ via LWT
//
//
// created: 02/2022
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
// 20220227 Created
// 20220424 Added deep sleep mode
// 20220425 Added conversion of wind speed from m/s to bft
//          Cleaned up code
// 20220517 Improved sleep mode
//          Added status LED option
// 20220527 Changed to use BresserWeatherSensorLib
// 20220810 Changed to modified WeatherSensor class; fixed Soil Moisture Sensor Handling
// 20220815 Changed to modified WeatherSensor class; added support of multiple sensors
//          Changed hostname to append chip ID
//          Added calculation of additional information using WeatherUtils.h/.cpp
// 20221006 Modified secure/non-secure client implementation
//          Modified string buffer size definitons
//          Added rain gauge statistics
//          Changed weatherSensor.getData() parameter 'flags' from DATA_ALL_SLOTS to DATA_COMPLETE
//          to provide data even if less sensors than expected (NUM_SENSORS) have been received.
// 20221227 Replaced DEBUG_PRINT/DEBUG_PRINTLN by Arduino logging functions
// 20230114 Fixed rain gauge update
// 20230124 Improved WiFi connection robustness
// 20230708 Changed MQTT payload and topic from char[] to String
// 20230709 Added lightning sensor
// 20230710 Added optional JSON output of floating point values as strings
//          Modified MQTT topics
// 20230711 Changed remaining MQTT topics from char[] to String
//          Fixed secure WiFi with CHECK_CA_ROOT for ESP32
//          Added define RX_STRATEGY
// 20230717 Added weather sensor startup handling to rain gauge
// 20230817 Added rain gauge reset via MQTT
// 20230826 Added hourly (past 60 minutes) rainfall as 'rain_h'
// 20231030 Fixed and improved mapping of sensor IDs to names
//          Refactored struct Sensor
// 20231103 Improved handling of time and date
// 20231105 Added lightning sensor data post-processing
// 20231228 Fixed entering sleep mode before sensor data was published
// 20240113 Added post-processed lightning data to payload
// 20240122 Added lightning post-processing reset
// 20240209 Added Leakage, Air Quality (HCHO/VOC) and CO2 Sensors
// 20240213 Added PM1.0 to Air Quality (Particulate Matter) Sensor decoder
// 20240504 Added board initialization
// 20240507 Added configuration of maximum number of sensors at run time
// 20240603 Modified for arduino-esp32 v3.0.0
// 20241113 Added getting/setting of sensor include/exclude lists via MQTT
//
// ToDo:
//
// -
//
// Notes:
//
// - To enable wakeup from deep sleep on ESP8266, GPIO16 (D0) must be connected to RST!
//   Add a jumper to remove this connection for programming!
// - MQTT code based on https://github.com/256dpi/arduino-mqtt
// - For secure MQTT (TLS server verifycation, check the following examples:
//   - ESP32:
//     https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFiClientSecure/examples/WiFiClientSecure/WiFiClientSecure.ino
//   - ESP8266:
//     https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/BearSSL_Validation/BearSSL_Validation.ino
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

// BEGIN User specific options
#define LED_EN                // Enable LED indicating successful data reception
#define LED_GPIO 2            // LED pin
#define TIMEZONE 1            // UTC + TIMEZONE
#define PAYLOAD_SIZE 300      // maximum MQTT message size
#define TOPIC_SIZE 60         // maximum MQTT topic size (debug output only)
#define HOSTNAME_SIZE 30      // maximum hostname size
#define RX_TIMEOUT 90000      // sensor receive timeout [ms]
#define STATUS_INTERVAL 30000 // MQTT status message interval [ms]
#define DATA_INTERVAL 15000   // MQTT data message interval [ms]
#define AWAKE_TIMEOUT 300000  // maximum time until sketch is forced to sleep [ms]
#define SLEEP_INTERVAL 300000 // sleep interval [ms]
#define WIFI_RETRIES 10       // WiFi connection retries
#define WIFI_DELAY 1000       // Delay between connection attempts [ms]
#define SLEEP_EN true         // enable sleep mode (see notes above!)
//#define USE_SECUREWIFI        // use secure WIFI
#define USE_WIFI              // use non-secure WIFI

// Enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

// Maximum number of sensors (override )
#define MAX_SENSORS 1

// Stop reception when data of at least one sensor is complete
#define RX_FLAGS DATA_COMPLETE

// Stop reception when data of all (max_sensors) is complete
//#define RX_FLAGS (DATA_COMPLETE | DATA_ALL_SLOTS)

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

// Enable to debug MQTT connection; will generate synthetic sensor data.
// #define _DEBUG_MQTT_

// Generate sensor data to test collecting data from multiple sources
// #define GEN_SENSOR_DATA

// END User specific configuration

#if (defined(USE_SECUREWIFI) && defined(USE_WIFI)) || (!defined(USE_SECUREWIFI) && !defined(USE_WIFI))
#error "Either USE_SECUREWIFI OR USE_WIFI must be defined!"
#endif

#if defined(ESP32)
#include <WiFi.h>
#if defined(USE_WIFI)
#elif defined(USE_SECUREWIFI)
#include <NetworkClientSecure.h>
#endif
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <string>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <time.h>
#include "src/WeatherSensorCfg.h"
#include "src/WeatherSensor.h"
#include "src/WeatherUtils.h"
#include "src/RainGauge.h"
#include "src/Lightning.h"
#include "src/InitBoard.h"

#if defined(JSON_FLOAT_AS_STRING)
#define JSON_FLOAT(x) String("\"") + x + String("\"")
#else
#define JSON_FLOAT(x) x
#endif

const char sketch_id[] = "BresserWeatherSensorMQTT 20231030";

// Map sensor IDs to Names
SensorMap sensor_map[] = {
    {0x39582376, "WeatherSensor"},
    //{0x83750871, "SoilMoisture-1"}
};

// enable only one of these below, disabling both is fine too.
//  #define CHECK_CA_ROOT
//  #define CHECK_PUB_KEY
//  Arduino 1.8.19 ESP32 WiFiClientSecure.h: "SHA1 fingerprint is broken now!"
//  #define CHECK_FINGERPRINT
////--------------------------////

#include "secrets.h"

#ifndef SECRETS
const char ssid[] = "WiFiSSID";
const char pass[] = "WiFiPassword";

#define HOSTNAME "ESPWeather"
#define APPEND_CHIP_ID

#define MQTT_PORT 8883 // checked by pre-processor!
const char MQTT_HOST[] = "xxx.yyy.zzz.com";
const char MQTT_USER[] = ""; // leave blank if no credentials used
const char MQTT_PASS[] = ""; // leave blank if no credentials used

#ifdef CHECK_CA_ROOT
static const char digicert[] PROGMEM = R"EOF(
    -----BEGIN CERTIFICATE-----
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    -----END CERTIFICATE-----
    )EOF";
#endif

#ifdef CHECK_PUB_KEY
// Extracted by: openssl x509 -pubkey -noout -in fullchain.pem
static const char pubkey[] PROGMEM = R"KEY(
    -----BEGIN PUBLIC KEY-----
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxx
    -----END PUBLIC KEY-----
    )KEY";
#endif

#ifdef CHECK_FINGERPRINT
// Extracted by: openssl x509 -fingerprint -in fullchain.pem
static const char fp[] PROGMEM = "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD";
#endif
#endif

WeatherSensor weatherSensor;
RainGauge rainGauge;
Lightning lightning;

// MQTT topics
const char MQTT_PUB_STATUS[] = "status";
const char MQTT_PUB_RADIO[] = "radio";
const char MQTT_PUB_DATA[] = "data";
const char MQTT_PUB_RSSI[] = "rssi";
const char MQTT_PUB_EXTRA[] = "extra";
const char MQTT_PUB_INC[] = "sensors_inc";
const char MQTT_PUB_EXC[] = "sensors_exc";
const char MQTT_SUB_RESET[] = "reset";
const char MQTT_SUB_GET_INC[] = "get_sensors_inc";
const char MQTT_SUB_GET_EXC[] = "get_sensors_exc";
const char MQTT_SUB_SET_INC[] = "set_sensors_inc";
const char MQTT_SUB_SET_EXC[] = "set_sensors_exc";

String mqttPubStatus;
String mqttPubRadio;
String mqttPubInc;
String mqttPubExc;
String mqttSubReset;
String mqttSubGetInc;
String mqttSubGetExc;
String mqttSubSetInc;
String mqttSubSetExc;
char Hostname[HOSTNAME_SIZE];

//////////////////////////////////////////////////////

#if (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_FINGERPRINT)) or (defined(CHECK_FINGERPRINT) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT) and defined(CHECK_FINGERPRINT))
#error "Can't have both CHECK_CA_ROOT and CHECK_PUB_KEY enabled"
#endif

// Generate WiFi network instance
#if defined(ESP32)
#if defined(USE_WIFI)
WiFiClient net;
#elif defined(USE_SECUREWIFI)
NetworkClientSecure net;
#endif
#elif defined(ESP8266)
#if defined(USE_WIFI)
WiFiClient net;
#elif defined(USE_SECUREWIFI)
BearSSL::WiFiClientSecure net;
#endif
#endif

//
// Generate MQTT client instance
// N.B.: Default message buffer size is too small!
//
MQTTClient client(PAYLOAD_SIZE);

uint32_t lastMillis = 0;
uint32_t statusPublishPreviousMillis = 0;

void publishWeatherdata(bool complete = false);
void mqtt_connect(void);

/*! 
 * \brief Set RTC
 *
 * \param epoch Time since epoch
 * \param ms unused
 */
void setTime(unsigned long epoch, int ms) {
  struct timeval tv;
  
  if (epoch > 2082758399){
	  tv.tv_sec = epoch - 2082758399;  // epoch time (seconds)
  } else {
	  tv.tv_sec = epoch;  // epoch time (seconds)
  }
  tv.tv_usec = ms;    // microseconds
  settimeofday(&tv, NULL);
}

/// Print date and time (i.e. local time)
void printDateTime(void) {
        struct tm timeinfo;
        char tbuf[25];
        
        time_t tnow;
        time(&tnow);
        localtime_r(&tnow, &timeinfo);
        strftime(tbuf, 25, "%Y-%m-%d %H:%M:%S", &timeinfo);
        log_i("%s", tbuf);
}

/*!
 * \brief Wait for WiFi connection
 *
 * \param wifi_retries   max. no. of retries
 * \param wifi_delay    delay in ms before each attemüt
 */
void wifi_wait(int wifi_retries, int wifi_delay)
{
    int count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(wifi_delay);
        if (++count == wifi_retries)
        {
            log_e("\nWiFi connection timed out, will restart after %d s", SLEEP_INTERVAL / 1000);
            ESP.deepSleep(SLEEP_INTERVAL * 1000);
        }
    }
}

/*!
 * \brief WiFiManager Setup
 *
 * Configures WiFi access point and MQTT connection parameters
 */
void mqtt_setup(void)
{
    log_i("Attempting to connect to SSID: %s", ssid);
    WiFi.hostname(Hostname);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    wifi_wait(WIFI_RETRIES, WIFI_DELAY);
    log_i("connected!");


    // Note: TLS security and rain/lightning statistics need correct time
    log_i("Setting time using SNTP");
    configTime(TIMEZONE * 3600, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    int retries = 10;
    while (now < 1510592825)
    {
        if (--retries == 0)
            break;
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    if (retries == 0) {
        log_w("\nSetting time using SNTP failed!");
    } else {
        log_i("\ndone!");
        setTime(time(nullptr), 0);
    }
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    log_i("Current time (GMT): %s", asctime(&timeinfo));

#ifdef USE_SECUREWIFI
#if defined(ESP8266)
#ifdef CHECK_CA_ROOT
    BearSSL::X509List cert(digicert);
    net.setTrustAnchors(&cert);
#endif
#ifdef CHECK_PUB_KEY
    BearSSL::PublicKey key(pubkey);
    net.setKnownKey(&key);
#endif
#ifdef CHECK_FINGERPRINT
    net.setFingerprint(fp);
#endif
#elif defined(ESP32)
#ifdef CHECK_CA_ROOT
    net.setCACert(digicert);
#endif
#ifdef CHECK_PUB_KEY
    error "CHECK_PUB_KEY: not implemented"
#endif
#ifdef CHECK_FINGERPRINT
        net.setFingerprint(fp);
#endif
#endif
#if (!defined(CHECK_PUB_KEY) and !defined(CHECK_CA_ROOT) and !defined(CHECK_FINGERPRINT))
    // do not verify tls certificate
    net.setInsecure();
#endif
#endif
    client.begin(MQTT_HOST, MQTT_PORT, net);

    // set up MQTT receive callback
    client.onMessage(messageReceived);
    client.setWill(mqttPubStatus.c_str(), "dead", true /* retained */, 1 /* qos */);
    mqtt_connect();
}

/*!
 * \brief (Re-)Connect to WLAN and connect MQTT broker
 */
void mqtt_connect(void)
{
    Serial.print(F("Checking wifi..."));
    wifi_wait(WIFI_RETRIES, WIFI_DELAY);

    Serial.print(F("\nMQTT connecting... "));
    while (!client.connect(Hostname, MQTT_USER, MQTT_PASS))
    {
        Serial.print(".");
        delay(1000);
    }

    log_i("\nconnected!");
    client.subscribe(mqttSubReset);
    client.subscribe(mqttSubGetInc);
    client.subscribe(mqttSubGetExc);
    client.subscribe(mqttSubSetInc);
    client.subscribe(mqttSubSetExc);
    log_i("%s: %s\n", mqttPubStatus.c_str(), "online");
    client.publish(mqttPubStatus, "online");
}

/*!
 * \brief MQTT message received callback
 */
void messageReceived(String &topic, String &payload)
{
    if (topic == mqttSubReset)
    {
        uint8_t flags = payload.toInt() & 0xFF;
        log_d("MQTT msg received: reset(0x%X)", flags);
        rainGauge.reset(flags);
        if (flags & 0x10) {
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

/*!
  \brief Publish weather data as MQTT message

  \param complete Indicate that entire data is complete, regardless of the flags temp_ok/wind_ok/rain_ok
                  (which reflect only the state of the last message)
*/
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
            #ifdef RAINGAUGE_OLD
            rainGauge.update(timeinfo, weatherSensor.sensor[i].w.rain_mm, weatherSensor.sensor[i].startup);
            #else
            rainGauge.update(now, weatherSensor.sensor[i].w.rain_mm, weatherSensor.sensor[i].startup);
            #endif
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
                weatherSensor.sensor[i].startup
            );
            mqtt_payload += String(",\"lightning_hr\":") + String(lightning.pastHour());
            int events;
            time_t timestamp;
            uint8_t distance;
            if (lightning.lastEvent(timestamp, events, distance)) {
                char tbuf[25];
                struct tm timeinfo;
                gmtime_r(&timestamp, &timeinfo);
                strftime(tbuf, 25, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
                mqtt_payload += String(",\"lightning_event_time\":\"") + String(tbuf) + String("\"");
                mqtt_payload += String(",\"lightning_event_count\":") + String(events);
                mqtt_payload += String(",\"lightning_event_distance_km\":") + String(distance);
            }
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_LEAKAGE) {
            // Water Leakage Sensor
            mqtt_payload += String(",\"leakage\":") + String(weatherSensor.sensor[i].leak.alarm);
      
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_AIR_PM) {
            // Air Quality (Particular Matter) Sensor
            if (!weatherSensor.sensor[i].pm.pm_1_0_init) {
                mqtt_payload += String(",\"pm1_0_ug_m3\":") + String(weatherSensor.sensor[i].pm.pm_1_0);
            }
            if (!weatherSensor.sensor[i].pm.pm_2_5_init) {
                mqtt_payload += String(",\"pm2_5_ug_m3\":") + String(weatherSensor.sensor[i].pm.pm_2_5);
            }
            if (!weatherSensor.sensor[i].pm.pm_10_init) {
                mqtt_payload += String(",\"pm10_ug_m3\":") + String(weatherSensor.sensor[i].pm.pm_10);
            }
            
        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_CO2) {
            // CO2 Sensor
            if (!weatherSensor.sensor[i].co2.co2_init) {
                mqtt_payload += String(",\"co2_ppm\":") + String(weatherSensor.sensor[i].co2.co2_ppm);
            }

        }
        else if (weatherSensor.sensor[i].s_type == SENSOR_TYPE_HCHO_VOC) {
            // HCHO / VOC Sensor
            if (!weatherSensor.sensor[i].voc.hcho_init) {
                mqtt_payload += String(",\"hcho_ppb\":") + String(weatherSensor.sensor[i].voc.hcho_ppb);
            }
            if (!weatherSensor.sensor[i].voc.voc_init) {
                mqtt_payload += String(",\"voc\":") + String(weatherSensor.sensor[i].voc.voc_level);
            }

        }
        else if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) || 
                 (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
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
            }
            if (weatherSensor.sensor[i].w.uv_ok || complete)
            {
                mqtt_payload += String(",\"uv\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.uv, 1));
            }
            if (weatherSensor.sensor[i].w.light_ok || complete)
            {
                mqtt_payload += String(",\"light_klx\":") + JSON_FLOAT(String(weatherSensor.sensor[i].w.light_klx, 1));
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
        String sensor_str = String(weatherSensor.sensor[i].sensor_id, HEX);
        if (sizeof(sensor_map) > 0) {
          for (size_t n = 0; n < sizeof(sensor_map)/sizeof(sensor_map[0]); n++) {
            if (sensor_map[n].id == weatherSensor.sensor[i].sensor_id) {
              sensor_str = String(sensor_map[n].name.c_str());
              break;
            }
          }
        }

        String mqtt_topic_base = String(Hostname) + String('/') + sensor_str + String('/');
        String mqtt_topic;

        // sensor data
        mqtt_topic = mqtt_topic_base + String(MQTT_PUB_DATA);

        log_i("%s: %s\n", mqtt_topic.c_str(), mqtt_payload.c_str());
        client.publish(mqtt_topic, mqtt_payload.substring(0, PAYLOAD_SIZE - 1), false, 0);

        // sensor specific RSSI
        mqtt_topic = mqtt_topic_base + String(MQTT_PUB_RSSI);
        client.publish(mqtt_topic, String(weatherSensor.sensor[i].rssi, 1), false, 0);

        // extra data
        mqtt_topic = String(Hostname) + String('/') + String(MQTT_PUB_EXTRA);

        if (mqtt_payload2.length() > 2)
        {
            log_i("%s: %s\n", mqtt_topic.c_str(), mqtt_payload2.c_str());
            client.publish(mqtt_topic, mqtt_payload2.substring(0, PAYLOAD_SIZE - 1), false, 0);
        }
    } // for (int i=0; i<weatherSensor.sensor.size(); i++)
}

//
// Publish radio receiver info as JSON string via MQTT
// - RSSI: Received Signal Strength Indication
//
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

//
// Setup
//
void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    initBoard();
    log_i("\n\n%s\n", sketch_id);

    // Set time zone
    setenv("TZ", TZ_INFO, 1);
    printDateTime();

#ifdef LED_EN
    // Configure LED output pins
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, HIGH);
#endif

    strncpy(Hostname, HOSTNAME, 20);
#if defined(APPEND_CHIP_ID) && defined(ESP32)
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    snprintf(&Hostname[strlen(Hostname)], HOSTNAME_SIZE, "-%06X", chipId);
#elif defined(APPEND_CHIP_ID) && defined(ESP8266)
    snprintf(&Hostname[strlen(Hostname)], HOSTNAME_SIZE, "-%06X", ESP.getChipId() & 0xFFFFFF);
#endif

    mqttPubStatus = String(Hostname) + String('/') + String(MQTT_PUB_STATUS);
    mqttPubRadio = String(Hostname) + String('/') + String(MQTT_PUB_RADIO);
    mqttPubInc = String(Hostname) + String('/') + String(MQTT_PUB_INC);
    mqttPubExc = String(Hostname) + String('/') + String(MQTT_PUB_EXC);
    mqttSubReset = String(Hostname) + String('/') + String(MQTT_SUB_RESET);
    mqttSubGetInc = String(Hostname) + String('/') + String(MQTT_SUB_GET_INC);
    mqttSubGetExc = String(Hostname) + String('/') + String(MQTT_SUB_GET_EXC);
    mqttSubSetInc = String(Hostname) + String('/') + String(MQTT_SUB_SET_INC);
    mqttSubSetExc = String(Hostname) + String('/') + String(MQTT_SUB_SET_EXC);

    mqtt_setup();
    weatherSensor.begin();
    weatherSensor.setSensorsCfg(MAX_SENSORS, RX_FLAGS);
}

/*!
  \brief Wrapper which allows passing of member function as parameter
*/
void clientLoopWrapper(void)
{
    client.loop();
}

//
// Main execution loop
//
void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(F("Checking wifi"));
        while (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
            WiFi.begin(ssid, pass);
            Serial.print(".");
            delay(10);
        }
        Serial.println(F("connected"));
    }
    else
    {
        if (!client.connected())
        {
            mqtt_connect();
        }
        else
        {
            client.loop();
        }
    }

    const uint32_t currentMillis = millis();
    if (currentMillis - statusPublishPreviousMillis >= STATUS_INTERVAL)
    {
        // publish a status message @STATUS_INTERVAL
        statusPublishPreviousMillis = currentMillis;
        log_i("%s: %s\n", mqttPubStatus.c_str(), "online");
        client.publish(mqttPubStatus, "online");
        publishRadio();
    }

    bool decode_ok = false;
    bool published_ok = false;

#ifdef _DEBUG_MQTT_
    decode_ok = weatherSensor.genMessage(0 /* slot */, 0x01234567 /* ID */, 1 /* type */, 0 /* channel */);
#else
    // Clear sensor data buffer
    weatherSensor.clearSlots();

#ifdef GEN_SENSOR_DATA
    weatherSensor.genMessage(1 /* slot */, 0xdeadbeef /* ID */, 1 /* type */, 7 /* channel */);
#endif

    // Attempt to receive data set with timeout of <xx> s
    decode_ok = weatherSensor.getData(RX_TIMEOUT, RX_FLAGS, 0, &clientLoopWrapper);
#endif

#ifdef LED_EN
    if (decode_ok)
    {
        digitalWrite(LED_GPIO, LOW);
    }
    else
    {
        digitalWrite(LED_GPIO, HIGH);
    }
#endif

    // publish a data message @DATA_INTERVAL
    if (millis() - lastMillis > DATA_INTERVAL)
    {
        lastMillis = millis();
        if (decode_ok) {
            publishWeatherdata(false);
            published_ok = true;
        }
    }

    bool force_sleep = millis() > AWAKE_TIMEOUT;

    // Go to sleep only after complete set of data has been sent
    if (SLEEP_EN && (published_ok || force_sleep))
    {
        if (force_sleep)
        {
            log_d("Awake time-out!");
        }
        else
        {
            log_d("Data forwarding completed.");
        }
        log_i("Sleeping for %d ms\n", SLEEP_INTERVAL);
        log_i("%s: %s\n", mqttPubStatus.c_str(), "offline");
        Serial.flush();
        client.publish(mqttPubStatus, "offline", true /* retained */, 0 /* qos */);
        client.loop();
        client.disconnect();
        net.stop();
#ifdef LED_EN
        pinMode(LED_GPIO, INPUT);
#endif
	weatherSensor.sleep();
        ESP.deepSleep(SLEEP_INTERVAL * 1000);
    }
} // loop()