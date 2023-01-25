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
// arduino-mqtt Joël Gähwiler (256dpi) (https://github.com/256dpi/arduino-mqtt)
// ArduinoJson by Benoit Blanchon (https://arduinojson.org)

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
//   - ESP8288: 
//     https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/BearSSL_Validation/BearSSL_Validation.ino
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

// BEGIN User specific options
#define LED_EN                  // Enable LED indicating successful data reception
#define LED_GPIO        2       // LED pin
#define NUM_SENSORS     1       // Number of sensors to be received
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
#define USE_SECUREWIFI          // use secure WIFI
//#define USE_WIFI              // use non-secure WIFI

 
// Enable to debug MQTT connection; will generate synthetic sensor data.
//#define _DEBUG_MQTT_

// Generate sensor data to test collecting data from multiple sources
//#define GEN_SENSOR_DATA

int const num_sensors = 1;

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


#include <MQTT.h>
#include <ArduinoJson.h>
#include <time.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "WeatherUtils.h"
#include "RainGauge.h"

const char sketch_id[] = "BresserWeatherSensorMQTT 20221024";

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

#include "secrets.h"

#ifndef SECRETS
    const char ssid[] = "WiFiSSID";
    const char pass[] = "WiFiPassword";

    #define HOSTNAME "ESPWeather"
    #define APPEND_CHIP_ID

    #define    MQTT_PORT     8883 // checked by pre-processor!
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
    // Extracted by: openssl x509 -fingerprint -in fillchain.pem
    static const char fp[] PROGMEM = "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD";
    #endif
#endif

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

//
// Wait for WiFi connection
//
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

                          
// Setup WiFi in Station Mode
//
void mqtt_setup(void)
{
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.print(ssid);
    WiFi.hostname(Hostname);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    wifi_wait(WIFI_RETRIES, WIFI_DELAY);
    Serial.println(F("connected!"));
    

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
    client.begin(MQTT_HOST, MQTT_PORT, net);
    
    // set up MQTT receive callback (if required)
    //client.onMessage(messageReceived);
    client.setWill(mqttPubStatus, "dead", true /* retained*/, 1 /* qos */);
    mqtt_connect();
}


//
// (Re-)Connect to WLAN and connect MQTT broker
//
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
    char mqtt_payload[PAYLOAD_SIZE];  // sensor data
    char mqtt_payload2[PAYLOAD_SIZE]; // calculated extra data
    char mqtt_topic[TOPIC_SIZE+31];   // add space for ID/name

    // ArduinoJson does not allow to set number of decimals for floating point data -
    // neither does MQTT Dashboard...
    // Therefore the JSON string is created manually. 

    for (int i=0; i<NUM_SENSORS; i++) {
      // Reset string buffers
      mqtt_payload[0]  = '\0';
      mqtt_payload2[0] = '\0';
      
      if (!weatherSensor.sensor[i].valid)
          continue;

      if (weatherSensor.sensor[i].rain_ok) {
          struct tm timeinfo;
          gmtime_r(&now, &timeinfo);
          rainGauge.update(timeinfo, weatherSensor.sensor[i].rain_mm);
      }
      
      // Example:
      // {"ch":0,"battery_ok":true,"humidity":44,"wind_gust":1.2,"wind_avg":1.2,"wind_dir":150,"rain":146}
      sprintf(&mqtt_payload[strlen(mqtt_payload)], "{");
      sprintf(&mqtt_payload2[strlen(mqtt_payload2)], "{");
      sprintf(&mqtt_payload[strlen(mqtt_payload)], "\"id\":%u", weatherSensor.sensor[i].sensor_id);
      #ifdef BRESSER_6_IN_1
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"ch\":%d", weatherSensor.sensor[i].chan);
      #endif
      sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"battery_ok\":%d", weatherSensor.sensor[i].battery_ok ? 1 : 0);
      if (weatherSensor.sensor[i].temp_ok || complete) {
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"temp_c\":%.1f", weatherSensor.sensor[i].temp_c);
      }
      if (weatherSensor.sensor[i].humidity_ok || complete) {
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"humidity\":%d", weatherSensor.sensor[i].humidity);
      }
      if (weatherSensor.sensor[i].wind_ok || complete) {
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"wind_gust\":%.1f", weatherSensor.sensor[i].wind_gust_meter_sec);
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"wind_avg\":%.1f", weatherSensor.sensor[i].wind_avg_meter_sec);
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"wind_dir\":%.1f", weatherSensor.sensor[i].wind_direction_deg);
      }
      if (weatherSensor.sensor[i].wind_ok) {
          char buf[4];
          sprintf(&mqtt_payload2[strlen(mqtt_payload2)], "\"wind_dir_txt\":\"%s\"", 
            winddir_flt_to_str(weatherSensor.sensor[i].wind_direction_deg, buf));
          sprintf(&mqtt_payload2[strlen(mqtt_payload2)], ",\"wind_gust_bft\":%d", 
            windspeed_ms_to_bft(weatherSensor.sensor[i].wind_gust_meter_sec));
          sprintf(&mqtt_payload2[strlen(mqtt_payload2)], ",\"wind_avg_bft\":%d", 
            windspeed_ms_to_bft(weatherSensor.sensor[i].wind_avg_meter_sec));          
      }
      if ((weatherSensor.sensor[i].temp_ok) && (weatherSensor.sensor[i].humidity_ok)) {
        sprintf(&mqtt_payload2[strlen(mqtt_payload2)], ",\"dewpoint_c\":%.1f", 
          calcdewpoint(weatherSensor.sensor[i].temp_c, weatherSensor.sensor[i].humidity));
        
        if (weatherSensor.sensor[i].wind_ok) {
          sprintf(&mqtt_payload2[strlen(mqtt_payload2)], ",\"perceived_temp_c\":%.1f", 
            perceived_temperature(weatherSensor.sensor[i].temp_c, weatherSensor.sensor[i].wind_avg_meter_sec, weatherSensor.sensor[i].humidity)); 
        }
      }
      if (weatherSensor.sensor[i].uv_ok || complete) {
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"uv\":%.1f", weatherSensor.sensor[i].uv);
      }
      if (weatherSensor.sensor[i].rain_ok || complete) {
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"rain\":%.1f", weatherSensor.sensor[i].rain_mm);
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"rain_d\":%.1f", rainGauge.currentDay());
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"rain_w\":%.1f", rainGauge.currentWeek());
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"rain_m\":%.1f", rainGauge.currentMonth());
      }
      if (weatherSensor.sensor[i].moisture_ok || complete) {
          sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"moisture\":%d", weatherSensor.sensor[i].moisture);
      }
      sprintf(&mqtt_payload[strlen(mqtt_payload)], "}");
      sprintf(&mqtt_payload2[strlen(mqtt_payload2)], "}");
      
      // Try to map sensor ID to name to make MQTT topic explanatory
      for (int n=0; n<NUM_SENSORS; n++) {
        if (sensor_map[n].id == weatherSensor.sensor[i].sensor_id) {
          snprintf(mqtt_topic, TOPIC_SIZE+31, "%s/%s", mqttPubData, sensor_map[n].name.substr(0, 30).c_str());
          break;
        }
        else {
          snprintf(mqtt_topic, TOPIC_SIZE+31, "%s/%8X", mqttPubData, weatherSensor.sensor[i].sensor_id);
        }
      }
      
      // sensor data
      Serial.printf("%s: %s\n", mqtt_topic, mqtt_payload);
      client.publish(mqtt_topic, mqtt_payload, false, 0);

      // extra data
      if (strlen(mqtt_payload2) > 2) {
        Serial.printf("%s: %s\n", mqttPubExtra, mqtt_payload2);
        client.publish(mqttPubExtra, mqtt_payload2, false, 0);
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
void loop() {
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
        client.disconnect();
        client.loop();
        #ifdef LED_EN
            pinMode(LED_GPIO, INPUT);
        #endif
        ESP.deepSleep(SLEEP_INTERVAL * 1000);
    }
} // loop()
