///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorMQTTWifiMgr.ino
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
// WiFiManager by tzapu (https://github.com/tzapu/WiFiManager)
// ESP_DoubleResetDetector by Khoi Hoang (https://github.com/khoih-prog/ESP_DoubleResetDetector)
//
// MQTT subscriptions:
//     - none -
//
// MQTT publications:
//     <base_topic>/data/<ID|Name>  sensor data as JSON string - see publishWeatherdata()
//     <base_topic>/extra           calculated data
//     <base_topic>/radio           radio transceiver info as JSON string - see publishRadio()
//     <base_topic>/status          "online"|"offline"|"dead"$
//
// $ via LWT
//
//
// created: 06/2023
//
//
// MIT License
//
// Copyright (c) 2023 Matthias Prinke
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
// 20230619 Created from BresserWeatherSensorMQTT
// 20230709 Added lightning sensor
// 20230708 Changed MQTT payload and topic from char[] to String
// 20230710 Added optional JSON output of floating point values as strings
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
// - WiFiManager code based on example AutoConnectwithFSParameters
// - Using ESP_DoubleResetDetector (https://github.com/khoih-prog/ESP_DoubleResetDetector)
//   to force WiFiManager reconfiguration
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

// Library Defines - Need to be defined before library import
#define ESP_DRD_USE_SPIFFS true

// BEGIN User specific options
//#define LED_EN                  // Enable LED indicating successful data reception
#define LED_GPIO        LED_BUILTIN      // LED pin
//#define NUM_SENSORS     1       // Number of sensors to be received
#define TIMEZONE        1       // UTC + TIMEZONE
#define PAYLOAD_SIZE    255     // maximum MQTT message size
#define TOPIC_SIZE      60      // maximum MQTT topic size
#define HOSTNAME_SIZE   30      // maximum hostname size
#define RX_TIMEOUT      90000   // sensor receive timeout [ms]
#define STATUS_INTERVAL 30000   // MQTT status message interval [ms]
#define DATA_INTERVAL   15000   // MQTT data message interval [ms]
#define AWAKE_TIMEOUT   300000  // maximum time until sketch is forced to sleep [ms]
#define SLEEP_INTERVAL  300000  // sleep interval [ms]
#define WIFI_RETRIES    10      // WiFi connection retries
#define WIFI_DELAY      1000    // Delay between connection attempts [ms]
#define SLEEP_EN        true    // enable sleep mode (see notes above!)
//#define USE_SECUREWIFI          // use secure WIFI
#define USE_WIFI              // use non-secure WIFI

#define JSON_CONFIG_FILE "/wifimanager_config.json"

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

// See
// https://stackoverflow.com/questions/19554972/json-standard-floating-point-numbers
// and
// https://stackoverflow.com/questions/35709595/why-would-you-use-a-string-in-json-to-represent-a-decimal-number
//
// Summary:
// A string representation of a float (e.g. "temp_c":"21.5") is recommended if the value shall displayed with the specified number of decimals.
// Otherwise the float value can be output as a numerical value (e.g. "temp_c":21.5).
//
//#define JSON_FLOAT_AS_STRING


// Enable to debug MQTT connection; will generate synthetic sensor data.
//#define _DEBUG_MQTT_

// Generate sensor data to test collecting data from multiple sources
//#define GEN_SENSOR_DATA

// END User specific configuration

#if ( defined(USE_SECUREWIFI) && defined(USE_WIFI) ) || ( !defined(USE_SECUREWIFI) && !defined(USE_WIFI) )
    #error "Either USE_SECUREWIFI OR USE_WIFI must be defined!"
#endif

#if defined(ESP32)
    #if defined(USE_WIFI)
        #include <WiFi.h>
    #elif defined(USE_SECUREWIFI)
        #include <WiFiClientSecure.h>
    #endif
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
#endif


#include <string>
#include <MQTT.h>
#include <FS.h>
#ifdef ESP32
    #include <SPIFFS.h>
#endif
#include <WiFiManager.h>
#include <ESP_DoubleResetDetector.h>
#include <ArduinoJson.h>
#include <time.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "WeatherUtils.h"
#include "RainGauge.h"

#if defined(JSON_FLOAT_AS_STRING)
    #define JSON_FLOAT(x) String("\"") + x + String("\"")
#else
    #define JSON_FLOAT(x) x
#endif

const char sketch_id[] = "BresserWeatherSensorMQTTWifiMgr";

// Map sensor IDs to Names
SensorMap sensor_map[NUM_SENSORS] = {
    {0x39582376, "WeatherSensor"}
    //,{0x83750871, "SoilMoisture-1"}
};

//enable only one of these below, disabling both is fine too.
// #define CHECK_CA_ROOT
// #define CHECK_PUB_KEY
// Arduino 1.8.19 ESP32 WiFiClientSecure.h: "SHA1 fingerprint is broken now!"
// #define CHECK_FINGERPRINT
////--------------------------////

#ifndef SECRETS
    const char ssid[] = "WiFiSSID";
    const char pass[] = "WiFiPassword";

    #define HOSTNAME "ESPWeather"
    #define APPEND_CHIP_ID

    // define your default values here, if there are different values in config.json, they are overwritten.
    char mqtt_host[40];
    char mqtt_port[6] = "1883";
    char mqtt_user[21] = "";
    char mqtt_pass[21] = "";


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
    // Extracted by: openssl x509 -fingerprint -in fillchain.pem
    static const char fp[] PROGMEM = "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD";
    #endif
#endif

DoubleResetDetector *drd;

// flag for saving data
bool shouldSaveConfig = false;

// flag for forcing WiFiManager re-config
bool forceConfig = false;

WeatherSensor weatherSensor;
RainGauge     rainGauge;

// MQTT topics
const char MQTT_PUB_STATUS[]      = "/status";
const char MQTT_PUB_RADIO[]       = "/radio";
const char MQTT_PUB_DATA[]        = "/data";
const char MQTT_PUB_EXTRA[]       = "/extra";

char mqttPubStatus[TOPIC_SIZE];
char mqttPubRadio[TOPIC_SIZE];
char mqttPubData[TOPIC_SIZE];
char mqttPubExtra[TOPIC_SIZE];
char Hostname[HOSTNAME_SIZE];

//////////////////////////////////////////////////////

#if (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_FINGERPRINT)) or (defined(CHECK_FINGERPRINT) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT) and defined(CHECK_FINGERPRINT))
#error "cant have both CHECK_CA_ROOT and CHECK_PUB_KEY enabled"
#endif

// Generate WiFi network instance
#if defined(ESP32)
    #if defined(USE_WIFI)
        WiFiClient net;
    #elif defined(USE_SECUREWIFI)
        WiFiClientSecure net;
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

void publishWeatherdata(bool complete = false);
void mqtt_connect(void);

/*!
 * \brief Callback notifying us of the need to save config
 */
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


/*!
 * \brief Wait for WiFi connection
 */
void wifi_wait(int wifi_retries, int wifi_delay)
{
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(wifi_delay);
        if (++count == wifi_retries) {
            Serial.printf("WiFi connection timed out, will restart after %d s\n", SLEEP_INTERVAL/1000);
            ESP.deepSleep(SLEEP_INTERVAL * 1000);
        }
    }
}


/*!
 * \brief WiFiManager Setup
 *
 * Configures WiFi access point and MQTT connection parameters
 */
void wifimgr_setup(void)
{

  // clean FS, for testing
  //SPIFFS.format();

  // read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      // file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);

        if ( ! deserializeError ) {
          Serial.println("\nparsed json");
          strcpy(mqtt_host, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  //WiFi.disconnect();
  WiFi.hostname(Hostname);
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  delay(10);

  // wm.resetSettings(); // wipe settings

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server (Broker)", mqtt_host, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT Username", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", mqtt_pass, 20);

  WiFiManager wifiManager;

  if (forceConfig) {
    wifiManager.resetSettings();
  }

  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 1, 99), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

  // add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);

  // Options
  // reset settings - for testing
  //wifiManager.resetSettings();

  // set minimum quality of signal so it ignores AP's under that quality
  // defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  // sets timeout until configuration portal gets turned off
  // useful to make it all retry or go to sleep
  // in seconds
  wifiManager.setTimeout(120);

  // fetches ssid and pass and tries to connect
  // if it does not connect it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(Hostname, "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  // if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  // read updated parameters
  strcpy(mqtt_host, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  Serial.println("The values in the file are: ");
  Serial.println("\tmqtt_server : " + String(mqtt_host));
  Serial.println("\tmqtt_port : " + String(mqtt_port));
  Serial.println("\tmqtt_user : " + String(mqtt_user));
  Serial.println("\tmqtt_pass : ***");

  // save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
 
    DynamicJsonDocument json(1024);
    json["mqtt_server"] = mqtt_host;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}


/*!
 * \brief Setup secure WiFi (if enabled) and MQTT client
 */
 void mqtt_setup(void)
{
    #ifdef USE_SECUREWIFI
        // Note: TLS security needs correct time
        Serial.print("Setting time using SNTP ");
        configTime(TIMEZONE * 3600, 0, "pool.ntp.org", "time.nist.gov");
        now = time(nullptr);
        while (now < 1510592825)
        {
            delay(500);
            Serial.print(".");
            now = time(nullptr);
        }
        Serial.println("done!");
        struct tm timeinfo;
        gmtime_r(&now, &timeinfo);
        Serial.print("Current time: ");
        Serial.println(asctime(&timeinfo));

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
        #if (!defined(CHECK_PUB_KEY) and !defined(CHECK_CA_ROOT) and !defined(CHECK_FINGERPRINT))
            // do not verify tls certificate
            net.setInsecure();
        #endif
    #endif
    client.begin(mqtt_host, atoi(mqtt_port), net);

    // set up MQTT receive callback (if required)
    //client.onMessage(messageReceived);
    client.setWill(mqttPubStatus, "dead", true /* retained*/, 1 /* qos */);
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
    while (!client.connect(Hostname, mqtt_user, mqtt_pass))
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.println(F("connected!"));
    //client.subscribe(MQTT_SUB_IN);
    Serial.printf("%s: %s\n", mqttPubStatus, "online");
    client.publish(mqttPubStatus, "online");
}


//
// MQTT message received callback
//
/*
void messageReceived(String &topic, String &payload)
{
}
*/



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

    for (int i=0; i<NUM_SENSORS; i++) {
      // Reset string buffers
      mqtt_payload  = "";
      mqtt_payload2 = "";

      if (!weatherSensor.sensor[i].valid)
          continue;

      if (weatherSensor.sensor[i].rain_ok) {
          struct tm timeinfo;
          gmtime_r(&now, &timeinfo);
          rainGauge.update(timeinfo, weatherSensor.sensor[i].rain_mm);
      }

      // Example:
      // {"ch":0,"battery_ok":1,"humidity":44,"wind_gust":1.2,"wind_avg":1.2,"wind_dir":150,"rain":146}
      mqtt_payload  = "{";
      mqtt_payload2 = "{";
      mqtt_payload  += String("\"id\":") + String(weatherSensor.sensor[i].sensor_id);
      #ifdef BRESSER_6_IN_1
          mqtt_payload += String(",\"ch\":") + String(weatherSensor.sensor[i].chan);
      #endif
      mqtt_payload += String(",\"battery_ok\":") + (weatherSensor.sensor[i].battery_ok ? String("1") : String("0"));
      if (weatherSensor.sensor[i].temp_ok || complete) {
          mqtt_payload += String(",\"temp_c\":") + JSON_FLOAT(String(weatherSensor.sensor[i].temp_c, 1));
      }
      if (weatherSensor.sensor[i].humidity_ok || complete) {
          mqtt_payload += String(",\"humidity\":") + String(weatherSensor.sensor[i].humidity);
      }
      if (weatherSensor.sensor[i].wind_ok || complete) {
          mqtt_payload += String(",\"wind_gust\":") + JSON_FLOAT(String(weatherSensor.sensor[i].wind_gust_meter_sec, 1));
          mqtt_payload += String(",\"wind_avg\":")  + JSON_FLOAT(String(weatherSensor.sensor[i].wind_avg_meter_sec, 1));
          mqtt_payload += String(",\"wind_dir\":")  + JSON_FLOAT(String(weatherSensor.sensor[i].wind_direction_deg, 1));
      }
      if (weatherSensor.sensor[i].wind_ok) {
          char buf[4];
          mqtt_payload2 += String("\"wind_dir_txt\":\"") + String(winddir_flt_to_str(weatherSensor.sensor[i].wind_direction_deg, buf)) + "\"";
          mqtt_payload2 += String(",\"wind_gust_bft\":") + String(windspeed_ms_to_bft(weatherSensor.sensor[i].wind_gust_meter_sec));
          mqtt_payload2 += String(",\"wind_avg_bft\":")  + String(windspeed_ms_to_bft(weatherSensor.sensor[i].wind_avg_meter_sec));
      }
      if ((weatherSensor.sensor[i].temp_ok) && (weatherSensor.sensor[i].humidity_ok)) {
        mqtt_payload2 += String(",\"dewpoint_c\":")
            + JSON_FLOAT(String(calcdewpoint(weatherSensor.sensor[i].temp_c, weatherSensor.sensor[i].humidity), 1));

        if (weatherSensor.sensor[i].wind_ok) {
          mqtt_payload2 += String(",\"perceived_temp_c\":") 
              + JSON_FLOAT(String(perceived_temperature(weatherSensor.sensor[i].temp_c, weatherSensor.sensor[i].wind_avg_meter_sec, weatherSensor.sensor[i].humidity), 1));
        }
      }
      if (weatherSensor.sensor[i].uv_ok || complete) {
          mqtt_payload += String(",\"uv\":") + JSON_FLOAT(String(weatherSensor.sensor[i].uv, 1);
      }
      if (weatherSensor.sensor[i].light_ok || complete) {
          mqtt_payload += String(",\"light_klx\":") + JSON_FLOAT(String(weatherSensor.sensor[i].light_klx, 1));
      }
      if (weatherSensor.sensor[i].rain_ok || complete) {
          mqtt_payload += String(",\"rain\":")   + JSON_FLOAT(String(weatherSensor.sensor[i].rain_mm, 1));
          mqtt_payload += String(",\"rain_d\":") + JSON_FLOAT(String(rainGauge.currentDay(), 1));
          mqtt_payload += String(",\"rain_w\":") + JSON_FLOAT(String(rainGauge.currentWeek(), 1));
          mqtt_payload += String(",\"rain_m\":") + JSON_FLOAT(String(rainGauge.currentMonth(), 1));
      }
      if (weatherSensor.sensor[i].moisture_ok || complete) {
          mqtt_payload += String(",\"moisture\":") + String(weatherSensor.sensor[i].moisture);
      }
      if (weatherSensor.sensor[i].lightning_ok || complete) {
          mqtt_payload  += String(",\"lightning_count\":")       + String(weatherSensor.sensor[i].lightning_count);
          mqtt_payload  += String(",\"lightning_distance_km\":") + String(weatherSensor.sensor[i].lightning_distance_km);
          mqtt_payload  += String(",\"lightning_unknown1\":\"0x") 
              + String(weatherSensor.sensor[i].lightning_unknown1, HEX) + String("\"");
          mqtt_payload  += String(",\"lightning_unknown2\":\"0x") 
              + String(weatherSensor.sensor[i].lightning_unknown2, HEX) + String("\"");
      }
      mqtt_payload  += String("}");
      mqtt_payload2 += String("}");

      if (mqtt_payload.length() > PAYLOAD_SIZE) {
        log_e("mqtt_payload (%d) > PAYLOAD_SIZE (%d). Payload will be truncated!", mqtt_payload.length(), PAYLOAD_SIZE);
      }
      if (mqtt_payload2.length() > PAYLOAD_SIZE) {
        log_e("mqtt_payload2 (%d) > PAYLOAD_SIZE (%d). Payload will be truncated!", mqtt_payload2.length(), PAYLOAD_SIZE);
      }
    
      // Try to map sensor ID to name to make MQTT topic explanatory
      for (int n=0; n<NUM_SENSORS; n++) {
        mqtt_topic = String(mqttPubData);
        if (sensor_map[n].id == weatherSensor.sensor[i].sensor_id) {
          mqtt_topic += String("/") + String(sensor_map[n].name.c_str());
        }
        else {
          mqtt_topic += String("/") + String(weatherSensor.sensor[i].sensor_id, HEX);
        }
      }

      // sensor data
      #if CORE_DEBUG_LEVEL != ARDUHAL_LOG_LEVEL_NONE
        char mqtt_topic_tmp[TOPIC_SIZE];
        char mqtt_payload_tmp[PAYLOAD_SIZE];
        snprintf(mqtt_topic_tmp,   TOPIC_SIZE,   mqtt_topic.c_str());
        snprintf(mqtt_payload_tmp, PAYLOAD_SIZE, mqtt_payload.c_str());
      #endif
      log_i("%s: %s\n", mqtt_topic_tmp, mqtt_payload_tmp);
      client.publish(mqtt_topic, mqtt_payload.substring(0, PAYLOAD_SIZE-1), false, 0);

      // extra data
      #if CORE_DEBUG_LEVEL != ARDUHAL_LOG_LEVEL_NONE
        snprintf(mqtt_payload_tmp, PAYLOAD_SIZE, mqtt_payload2.c_str());
      #endif
      if (mqtt_payload2.length() > 2) {
        log_i("%s: %s\n", mqttPubExtra, mqtt_payload_tmp);
        client.publish(mqttPubExtra, mqtt_payload2.substring(0, PAYLOAD_SIZE-1), false, 0);
      }
    } // for (int i=0; i<NUM_SENSORS; i++)
}


//
// Publish radio receiver info as JSON string via MQTT
// - RSSI: Received Signal Strength Indication
//
void publishRadio(void)
{
    DynamicJsonDocument payload(PAYLOAD_SIZE);
    char mqtt_payload[PAYLOAD_SIZE];

    payload["rssi"] = weatherSensor.rssi;
    serializeJson(payload, mqtt_payload);
    Serial.printf("%s: %s\n", mqttPubRadio, mqtt_payload);
    client.publish(mqttPubRadio, mqtt_payload, false, 0);
    payload.clear();
}


//
// Setup
//
void setup() {
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
        for(int i=0; i<17; i=i+8) {
            chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
        }
        snprintf(&Hostname[strlen(Hostname)], HOSTNAME_SIZE, "-%06X", chipId);
    #elif defined(APPEND_CHIP_ID) && defined(ESP8266)
        snprintf(&Hostname[strlen(Hostname)], HOSTNAME_SIZE, "-%06X", ESP.getChipId() & 0xFFFFFF);
    #endif

    snprintf(mqttPubStatus, TOPIC_SIZE, "%s%s", Hostname, MQTT_PUB_STATUS);
    snprintf(mqttPubRadio,  TOPIC_SIZE, "%s%s", Hostname, MQTT_PUB_RADIO);
    snprintf(mqttPubData,   TOPIC_SIZE, "%s%s", Hostname, MQTT_PUB_DATA);
    snprintf(mqttPubExtra,  TOPIC_SIZE, "%s%s", Hostname, MQTT_PUB_EXTRA);

    drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
    if (drd->detectDoubleReset())
    {
        Serial.println(F("Forcing config mode as there was a Double reset detected"));
        forceConfig = true;
    }
/*
    bool spiffsSetup = loadConfigFile();
    if (!spiffsSetup)
    {
        Serial.println(F("Forcing config mode as there is no saved config"));
        forceConfig = true;
    }
*/
    wifimgr_setup();
    mqtt_setup();
    weatherSensor.begin();
}


/*!
  \brief Wrapper which allows passing of member function as parameter
*/
void clientLoopWrapper(void)
{
    client.loop();
    drd->loop();
}


//
// Main execution loop
//
void loop() {
    drd->loop();
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
    if (currentMillis - statusPublishPreviousMillis >= STATUS_INTERVAL) {
        // publish a status message @STATUS_INTERVAL
        statusPublishPreviousMillis = currentMillis;
        Serial.printf("%s: %s\n", mqttPubStatus, "online");
        client.publish(mqttPubStatus, "online");
        publishRadio();
    }

    bool decode_ok = false;
    #ifdef _DEBUG_MQTT_
        decode_ok = weatherSensor.genMessage(0 /* slot */, 0x01234567 /* ID */, 1 /* type */, 0 /* channel */);
    #else
        // Clear sensor data buffer
        weatherSensor.clearSlots();

        #ifdef GEN_SENSOR_DATA
            weatherSensor.genMessage(1 /* slot */, 0xdeadbeef /* ID */, 1 /* type */, 7 /* channel */);
        #endif

        // Attempt to receive data set with timeout of <xx> s
        decode_ok = weatherSensor.getData(RX_TIMEOUT, DATA_COMPLETE, 0, &clientLoopWrapper);
    #endif

    #ifdef LED_EN
        if (decode_ok) {
          digitalWrite(LED_GPIO, LOW);
        } else {
          digitalWrite(LED_GPIO, HIGH);
        }
    #endif

    // publish a data message @DATA_INTERVAL
    if (millis() - lastMillis > DATA_INTERVAL) {
        lastMillis = millis();
        if (decode_ok)
            publishWeatherdata(false);
    }

    bool force_sleep = millis() > AWAKE_TIMEOUT;

    // Go to sleep only after complete set of data has been sent
    if (SLEEP_EN && (decode_ok || force_sleep)) {
        #ifdef _DEBUG_MODE_
            if (force_sleep) {
                Serial.println(F("Awake time-out!"));
            } else {
                Serial.println(F("Data forwarding completed."));
            }
        #endif
        Serial.printf("Sleeping for %d ms\n", SLEEP_INTERVAL);
        Serial.printf("%s: %s\n", mqttPubStatus, "offline");
        Serial.flush();
        client.publish(mqttPubStatus, "offline", true /* retained */, 0 /* qos */);
        client.loop();
        client.disconnect();
        client.loop();
        #ifdef LED_EN
            pinMode(LED_GPIO, INPUT);
        #endif
        ESP.deepSleep(SLEEP_INTERVAL * 1000);
    }
} // loop()
