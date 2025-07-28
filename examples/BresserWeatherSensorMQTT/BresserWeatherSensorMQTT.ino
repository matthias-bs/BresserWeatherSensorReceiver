///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorMQTT.ino
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
// Furthermore, Home Assistant auto-discovery messages are published at an interval of
// DISCOVERY_INTERVAL.
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
//     homeassistant/sensor/<sensor_id>_<json_ele>/config   Home Assistand auto discovery for sensor data
//     homeassistant/sensor/<hostname>_<json_ele>/config    Home Assistand auto discovery for receiver control/status
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
// 20250127 Added Globe Thermometer Temperature (8-in-1 Weather Sensor)
// 20250129 Added calculated WBGT (Wet Bulb Globe Temperature)
// 20250220 Added Home Assistant auto discovery
// 20250223 Moved MQTT functions to src/mqtt_comm.h/.cpp
// 20250712 Removed TLS fingerprint option (insecure)
//          Improved MQTT "offline" status message handling (avoid inadvertent LWT message)
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
#define RX_TIMEOUT 90000      // sensor receive timeout [ms]
#define STATUS_INTERVAL 30000 // MQTT status message interval [ms]
#define DATA_INTERVAL 15000   // MQTT data message interval [ms]
#define DISCOVERY_INTERVAL 30 // Home Assistant auto discovery interval [min]
#define AWAKE_TIMEOUT 300000  // maximum time until sketch is forced to sleep [ms]
#define SLEEP_INTERVAL 300000 // sleep interval [ms]
#define WIFI_RETRIES 10       // WiFi connection retries
#define WIFI_DELAY 1000       // Delay between connection attempts [ms]
#define SLEEP_EN true         // enable sleep mode (see notes above!)
//#define AUTO_DISCOVERY        // enable Home Assistant auto discovery
//#define USE_SECUREWIFI        // use secure WIFI
 #define USE_WIFI // use non-secure WIFI

// Enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char *TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

// Maximum number of sensors (override)
#define MAX_SENSORS 1

// Stop reception when data of at least one sensor is complete
#define RX_FLAGS DATA_COMPLETE

// Stop reception when data of all (max_sensors) is complete
//#define RX_FLAGS (DATA_COMPLETE | DATA_ALL_SLOTS)

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
#include <vector>
#include <MQTT.h>
#include <time.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "WeatherUtils.h"
#include "RainGauge.h"
#include "Lightning.h"
#include "InitBoard.h"
#include "src/mqtt_comm.h"

const char sketch_id[] = "BresserWeatherSensorMQTT 20250228";

// Map sensor IDs to Names - replace by your own IDs!
std::vector<SensorMap> sensor_map = {
    {0x39582376, "WeatherSensor"},
    {0x21103427, "WeatherSensor"},
    {0x67566300, "SoilSensor"},
    {0x5680, "AirQualitySensor"},
    {0x28966796, "LeakageSensor"},
    {0xeefb, "LightningSensor"},
    {0x22400873, "PoolThermometer"}
    //{0x83750871, "SoilMoisture-1"}
};

// enable only one of these below, disabling both is fine too.
//  #define CHECK_CA_ROOT
//  #define CHECK_PUB_KEY
////--------------------------////

#include "secrets.h"

#ifndef SECRETS
const char ssid[] = "WiFiSSID";
const char pass[] = "WiFiPassword";

const char HOSTNAME[] = "ESPWeather";
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
#endif

WeatherSensor weatherSensor;
RainGauge rainGauge;
Lightning lightning;

// MQTT topics - change if needed
String Hostname = String(HOSTNAME);
String mqttPubStatus = "status";
String mqttPubRadio = "radio";
String mqttPubData = "data";
String mqttPubRssi = "rssi";
String mqttPubExtra = "extra";
String mqttPubInc = "sensors_inc";
String mqttPubExc = "sensors_exc";
String mqttSubReset = "reset";
String mqttSubGetInc = "get_sensors_inc";
String mqttSubGetExc = "get_sensors_exc";
String mqttSubSetInc = "set_sensors_inc";
String mqttSubSetExc = "set_sensors_exc";

//////////////////////////////////////////////////////

#if (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT))
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
#if defined(AUTO_DISCOVERY)
uint32_t discoveryPublishPreviousMillis = 0;
#endif

/*!
 * \brief Set RTC
 *
 * \param epoch Time since epoch
 * \param ms unused
 */
void setTime(unsigned long epoch, int ms)
{
    struct timeval tv;

    if (epoch > 2082758399)
    {
        tv.tv_sec = epoch - 2082758399; // epoch time (seconds)
    }
    else
    {
        tv.tv_sec = epoch; // epoch time (seconds)
    }
    tv.tv_usec = ms; // microseconds
    settimeofday(&tv, NULL);
}

/// Print date and time (i.e. local time)
void printDateTime(void)
{
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
    WiFi.hostname(Hostname.c_str());
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
    if (retries == 0)
    {
        log_w("\nSetting time using SNTP failed!");
    }
    else
    {
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
#elif defined(ESP32)
#ifdef CHECK_CA_ROOT
    net.setCACert(digicert);
#endif
#ifdef CHECK_PUB_KEY
    #error "CHECK_PUB_KEY: not implemented"
#endif
#endif
#if (!defined(CHECK_PUB_KEY) and !defined(CHECK_CA_ROOT))
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
    while (!client.connect(Hostname.c_str(), MQTT_USER, MQTT_PASS))
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

    char ChipID[8] = "";

#if defined(APPEND_CHIP_ID) && defined(ESP32)
    uint32_t chip_id = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chip_id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    sprintf(ChipID, "-%06lX", chip_id);
#elif defined(APPEND_CHIP_ID) && defined(ESP8266)
    sprintf(ChipID, "-%06lX", ESP.getChipId() & 0xFFFFFF);
#endif

    Hostname = Hostname + ChipID;
    // Prepend Hostname to MQTT topics
    mqttPubStatus = Hostname + "/" + mqttPubStatus;
    mqttPubRadio = Hostname + "/" + mqttPubRadio;
    mqttPubExtra = Hostname + "/" + mqttPubExtra;
    mqttPubInc = Hostname + "/" + mqttPubInc;
    mqttPubExc = Hostname + "/" + mqttPubExc;
    mqttSubReset = Hostname + "/" + mqttSubReset;
    mqttSubGetInc = Hostname + "/" + mqttSubGetInc;
    mqttSubGetExc = Hostname + "/" + mqttSubGetExc;
    mqttSubSetInc = Hostname + "/" + mqttSubSetInc;
    mqttSubSetExc = Hostname + "/" + mqttSubSetExc;

    weatherSensor.begin();
    weatherSensor.setSensorsCfg(MAX_SENSORS, RX_FLAGS);
    mqtt_setup();
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
        publishWeatherdata(false);
        client.loop();
        published_ok = true;
    }

#if defined(AUTO_DISCOVERY)
    // publish a discovery message @DISCOVERY_INTERVAL
    if ((discoveryPublishPreviousMillis == 0) ||
        (millis() - discoveryPublishPreviousMillis > DISCOVERY_INTERVAL * 60000))
    {
        discoveryPublishPreviousMillis = millis();
        haAutoDiscovery();
        client.loop();
    }
#endif

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
        for (int i = 0; i < 5; i++) // Retry loop to ensure message delivery
        {
            client.loop();
            delay(500); // Allow time for the message to be sent
        }
        client.disconnect();
        delay(1000); // Allow time for the client to disconnect properly
        net.stop();
#ifdef LED_EN
        pinMode(LED_GPIO, INPUT);
#endif
        // Note:
        // Further reduction of sleep current might be possible by
        // controlling the GPIO pins (including SPI CS) appropriately.
        // This depends on the actual board/radio chip used.
        // See
        // https://github.com/jgromes/RadioLib/discussions/1375#discussioncomment-11763846
        weatherSensor.sleep();
        ESP.deepSleep(SLEEP_INTERVAL * 1000);
    }
} // loop()
