///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorDomoticz.ino
//
// Example for BresserWeatherSensorReceiver - based on BresserWeatherSensorMQTT
//
// Provides sensor data as MQTT messages via WiFi to Domoticz (https://domoticz.com/)
// (MQTT plugin for Domoticz required)
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
// The Domoticz topic is published at an interval of >DATA_INTERVAL.
// The 'status' and the 'radio' topics are published at an interval of STATUS_INTERVAL.
//
// If sleep mode is enabled (SLEEP_EN), the device goes into deep sleep mode after data has
// been published. If AWAKE_TIMEOUT is reached before data has been published, deep sleep is
// entered, too. After SLEEP_INTERVAL, the controller is restarted.
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
// Based on:
// arduino-mqtt by Joël Gähwiler (256dpi) (https://github.com/256dpi/arduino-mqtt)
// ArduinoJson by Benoit Blanchon (https://arduinojson.org)
//
// MQTT subscriptions:
//     - none -
//
// MQTT publications:
//     <base_topic>/radio   radio transceiver info as JSON string - see publishRadio()
//     <base_topic>/status  "online"|"offline"|"dead"$
//     domoticz/in          topic required by Domoticz
// $ via LWT
//
//
// created: 08/2022
//
//
// MIT License
//
// Copyright (c) 2022 franki29 & Matthias Prinke
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
// 20220815 Created from BresserWeatherSensorMQTT
// 20221006 Modified secure/non-secure client implementation
//          Modified string buffer size handling
// 20221227 Replaced DEBUG_PRINT/DEBUG_PRINTLN by Arduino logging functions
// 20230124 Improved WiFi connection robustness
// 20231028 Refactored struct Sensor
//          Changed MQTT payload and topic from char[] to String
// 20240325 domoticz virtual rain sensor: added hourly rain rate
// 20240504 Added board initialization
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

// Enable LED indicating successful data reception
#define LED_EN

// LED pin
#define LED_GPIO 2

// BEGIN User specific options
// Please change Domoticz IDX settings according to your configuration
#define DOMO_WIND_IDX 877     // IDX of Domoticz virtual wind sensor
#define DOMO_RAIN_IDX 878     // IDX of Domoticz virtual rain sensor
#define DOMO_TH_IDX 876       // IDX of Domoticz virtual temperature/humidity sensor
#define TIMEZONE 1            // UTC + TIMEZONE
#define PAYLOAD_SIZE 256      // maximum MQTT message size
#define HOSTNAME_SIZE 30      // maximum hostname size
#define RX_TIMEOUT 60000      // sensor receive timeout [ms]
#define STATUS_INTERVAL 30000 // MQTT status message interval [ms]
#define DATA_INTERVAL 15000   // MQTT data message interval [ms]
#define AWAKE_TIMEOUT 300000  // maximum time until sketch is forced to sleep [ms]
#define SLEEP_INTERVAL 300000 // sleep interval [ms]
#define WIFI_RETRIES 10       // WiFi connection retries
#define WIFI_DELAY 1000       // Delay between connection attempts [ms]
#define SLEEP_EN true         // enable sleep mode (see notes above!)
//#define USE_SECUREWIFI        // use secure WIFI
#define USE_WIFI              // use non-secure WIFI

#if (defined(USE_SECUREWIFI) && defined(USE_WIFI)) || (!defined(USE_SECUREWIFI) && !defined(USE_WIFI))
#error "Either USE_SECUREWIFI OR USE_WIFI must be defined!"
#endif

// Enable to debug MQTT connection; will generate synthetic sensor data.
// #define _DEBUG_MQTT_

#include <Arduino.h>

#if defined(ESP32)
#include <WiFi.h>
#if defined(USE_WIFI)
#elif defined(USE_SECUREWIFI)
<NetworkClientSecure.h>
#endif
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <string>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <time.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "WeatherUtils.h"
#include "RainGauge.h"
#include "InitBoard.h"

const char sketch_id[] = "BresserWeatherSensorDomoticz 20231028";

// enable only one of these below, disabling both is fine too.
//  #define CHECK_CA_ROOT
//  #define CHECK_PUB_KEY
//  Arduino 1.8.19 ESP32 WiFiClientSecure.h: "SHA1 fingerprint is broken now!"
//  #define CHECK_FINGERPRINT
////--------------------------////

#include "secrets.h"

#ifndef SECRETS
// Optionally copy everything between BEGIN secrets / END secrets to secrets.h
// Otherwise, leave secrets.h as an empty file and edit the contents below.

// BEGIN secrets
#define SECRETS
const char ssid[] = "SSID";
const char pass[] = "password";

#define HOSTNAME "BresserDomo"
#define APPEND_CHIP_ID

const int MQTT_PORT = 8883; // typically 8883 with TLS / 1883 without TLS
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

// MQTT topics
const char MQTT_PUB_STATUS[] = "/status";
const char MQTT_PUB_RADIO[] = "/radio";
const char MQTT_PUB_DOMO[] = "domoticz/in"; // mqtt plugin for domoticz needed

String mqttPubStatus;
String mqttPubRadio;
String mqttSubReset;
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
time_t now;

void publishWeatherdata(void);
void mqtt_connect(void);

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

#ifdef USE_SECUREWIFI
    // Note: TLS security needs correct time
    log_i("Setting time using SNTP");
    configTime(TIMEZONE * 3600, 0, "pool.ntp.org", "time.nist.gov");
    now = time(nullptr);
    while (now < 1510592825)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    log_i("\ndone!");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    log_i("Current time: %s", asctime(&timeinfo));

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

    // set up MQTT receive callback (if required)
    // client.onMessage(messageReceived);

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
    // client.subscribe(mqttSubReset);
    log_i("%s: %s\n", mqttPubStatus.c_str(), "online");
    client.publish(mqttPubStatus.c_str(), "online");
}

/*!
 * \brief MQTT message received callback
 */

// void messageReceived(String &topic, String &payload)
// {
// }

/*!
  \brief Publish weather data as MQTT message

  \param complete Indicate that entire data is complete, regardless of the flags temp_ok/wind_ok/rain_ok
                  (which reflect only the state of the last message)
*/
void publishWeatherdata(void)
{
    String domo_payload;
    String domo2_payload;
    String domo3_payload;
    int const i = 0;

    // ArduinoJson does not allow to set number of decimals for floating point data -
    // neither does MQTT Dashboard...
    // Therefore the JSON string is created manually.

    // domoticz virtual wind sensor
    if (weatherSensor.sensor[i].w.wind_ok && weatherSensor.sensor[i].w.temp_ok)
    {
        domo_payload = String("{\"idx\":") + String(DOMO_WIND_IDX) + String(",\"nvalue\":0,\"svalue\":\"") + String(weatherSensor.sensor[i].w.wind_direction_deg, 1);
        char buf[4];
        winddir_flt_to_str(weatherSensor.sensor[i].w.wind_direction_deg, buf);
        domo_payload += String(";") + String(buf);
        domo_payload += String(";") + String(weatherSensor.sensor[i].w.wind_avg_meter_sec * 10, 1);
        domo_payload += String(";") + String(weatherSensor.sensor[i].w.wind_gust_meter_sec * 10, 1);
        domo_payload += String(";") + String(weatherSensor.sensor[i].w.temp_c, 1);
        domo_payload += String(";") + String(perceived_temperature(weatherSensor.sensor[i].w.temp_c, weatherSensor.sensor[i].w.wind_avg_meter_sec, weatherSensor.sensor[i].w.humidity), 1);
        domo_payload += String("\"}");
        Serial.printf("%s: %s\n", MQTT_PUB_DOMO, domo_payload.c_str());
        client.publish(MQTT_PUB_DOMO, domo_payload.c_str(), false, 0);
    }

    // domoticz virtual rain sensor
    if (weatherSensor.sensor[i].w.rain_ok)
    {
        rainGauge.update(now, weatherSensor.sensor[i].w.rain_mm, weatherSensor.sensor[i].startup);
        
        domo2_payload = String("{\"idx\":") + String(DOMO_RAIN_IDX) + String(",\"nvalue\":0,\"svalue\":\"") + String(rainGauge.pastHour() * 100, 0);
        domo2_payload += String(";") + String(weatherSensor.sensor[i].w.rain_mm, 1);
        domo2_payload += String("\"}");
        Serial.printf("%s: %s\n", MQTT_PUB_DOMO, domo2_payload.c_str());
        client.publish(MQTT_PUB_DOMO, domo2_payload.c_str(), false, 0);
    }

    // domoticz virtual temp & humidity sensor
    if (weatherSensor.sensor[i].w.temp_ok && weatherSensor.sensor[i].w.humidity_ok)
    {
        domo3_payload = String("{\"idx\":") + String(DOMO_TH_IDX) + String(",\"nvalue\":0,\"svalue\":\"") + String(weatherSensor.sensor[i].w.temp_c, 1);
        domo3_payload += String(";") + String(weatherSensor.sensor[i].w.humidity);
        domo3_payload += String(";0\"}");
        Serial.printf("%s: %s\n", MQTT_PUB_DOMO, domo3_payload.c_str());
        client.publish(MQTT_PUB_DOMO, domo3_payload.c_str(), false, 0);
    }
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
    Serial.printf("%s: %s\n", mqttPubRadio.c_str(), mqtt_payload.c_str());
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
    Serial.println();
    Serial.println();
    Serial.println(sketch_id);
    Serial.println();

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

    mqttPubStatus = String(Hostname) + String(MQTT_PUB_STATUS);
    mqttPubRadio = String(Hostname) + String(MQTT_PUB_RADIO);

    mqtt_setup();

    weatherSensor.begin();
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
        Serial.printf("%s: %s\n", mqttPubStatus.c_str(), "online");
        client.publish(mqttPubStatus, "online");
        publishRadio();
    }

    bool decode_ok = false;
#ifdef _DEBUG_MQTT_
    decode_ok = weatherSensor.genMessage(0 /* slot */, 0x01234567 /* ID */, 1 /* type */, 0 /* channel */);
#else
    // Clear sensor data buffer
    weatherSensor.clearSlots();

    // Attempt to receive entire data set with timeout of RX_TIMEOUT
    decode_ok = weatherSensor.getData(RX_TIMEOUT, DATA_TYPE | DATA_COMPLETE, SENSOR_TYPE_WEATHER1, &clientLoopWrapper);
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
        if (decode_ok)
            publishWeatherdata();
    }

    bool force_sleep = millis() > AWAKE_TIMEOUT;

    // Go to sleep only after complete set of data has been sent
    if (SLEEP_EN && (decode_ok || force_sleep))
    {
#ifdef _DEBUG_MODE_
        if (force_sleep)
        {
            Serial.println(F("Awake time-out!"));
        }
        else
        {
            Serial.println(F("Data forwarding completed."));
        }
#endif
        Serial.printf("Sleeping for %d ms\n", SLEEP_INTERVAL);
        Serial.printf("%s: %s\n", mqttPubStatus.c_str(), "offline");
        Serial.flush();
        client.publish(mqttPubStatus, "offline", true /* retained */, 0 /* qos */);
        client.disconnect();
        client.loop();
#ifdef LED_EN
        pinMode(LED_GPIO, INPUT);
#endif
        weatherSensor.sleep();
        ESP.deepSleep(SLEEP_INTERVAL * 1000);
    }
} // loop()
