///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorMQTT.ino
//
// Bresser 5-in-1/6-in-1 868 MHz Weather Sensor Radio Receiver based on CC1101 or SX1276/RFM95W 
// and ESP32/ESP8266 - provides data via secure MQTT
//
// https://github.com/matthias-bs/BresserWeatherSensorLib
//
// Based on:
// Bresser5in1-CC1101 by Sean Siford (https://github.com/seaniefs/Bresser5in1-CC1101)
// RadioLib by Jan Gromeš (https://github.com/jgromes/RadioLib)
// arduino-mqtt Joël Gähwiler (256dpi) (https://github.com/256dpi/arduino-mqtt)
// ArduinoJson by Benoit Blanchon (https://arduinojson.org)
//
// MQTT subscriptions:
//     - none -
//
// MQTT publications:               
//     <base_topic>/data    sensor data as JSON string - see publishWeatherdata()
//     <base_topic>/radio   CC1101 radion transceiver info as JSON string - see publishRadio()
//     <base_topic>/status  "online"|"offline"|"dead"$
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
//
// ToDo:
// 
// - check option CHECK_CA_ROOT
//
// Notes:
//
// - To enable wakeup from deep sleep on ESP8266, GPIO16 (D0) must be connected to RST!
//   Add a jumper to remove this connection for programming!
//
///////////////////////////////////////////////////////////////////////////////////////////////////


// Enable LED indicating successful data reception
//#define LED_EN

// LED pin
#define LED_GPIO 2

// User specific options
#define PAYLOAD_SIZE    200     // maximum MQTT message size
#define STATUS_INTERVAL 30000   // MQTT status message interval [ms]
#define DATA_INTERVAL   15000   // MQTT data message interval [ms]
#define AWAKE_TIMEOUT   300000  // maximum time until sketch is forced to sleep [ms]
#define SLEEP_INTERVAL  300000  // sleep interval [ms]
#define SLEEP_EN        true   // enable sleep mode (see notes above!)


// Enable to debug MQTT connection; will generate synthetic sensor data.
//#define _DEBUG_MQTT_

// Print misc debug output
//#define _DEBUG_MODE_

// Enable RadioLib internal debug messages
//#define RADIOLIB_DEBUG


#include <Arduino.h>
#include <RadioLib.h>

#if defined(ESP32)
    #include <WiFi.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
#endif

#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <time.h>
#include "WeatherSensor.h"

#if defined(ESP32)
    #define PIN_CC1101_CS   5
    #define PIN_CC1101_GDO0 27
    #define PIN_CC1101_GDO2 4
#elif defined(ESP8266)
    #define PIN_CC1101_CS   15
    #define PIN_CC1101_GDO0 4
    #define PIN_CC1101_GDO2 5
#endif

const char sketch_id[] = "BresserWeatherSensorMQTT 20220527";

//enable only one of these below, disabling both is fine too.
// #define CHECK_CA_ROOT
// #define CHECK_PUB_KEY
// Arduino 1.8.19 ESP32 WiFiClientSecure.h: "SHA1 fingerprint is broken now!"
#define CHECK_FINGERPRINT
////--------------------------////

#include "secrets.h"

#ifndef SECRET
    const char ssid[] = "WiFiSSID";
    const char pass[] = "WiFiPassword";

    #define HOSTNAME "hostname"

    const char MQTT_HOST[] = "xxx.yyy.zzz.com";
    const int  MQTT_PORT = 8883;
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
    // Extracted by: openssl x509 -pubkey -noout -in ca.crt
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
    // Extracted by: openssl x509 -fingerprint -in ca.crt
    static const char fp[] PROGMEM = "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD";
    #endif
#endif

WeatherSensor weatherSensor(PIN_RECEIVER_CS, PIN_RECEIVER_IRQ, PIN_RECEIVER_RESET, PIN_RECEIVER_GPIO);


// MQTT topics
const char MQTT_PUB_STATUS[]      = HOSTNAME "/status";
const char MQTT_PUB_RADIO[]       = HOSTNAME "/radio";
const char MQTT_PUB_DATA[]        = HOSTNAME "/data";


//////////////////////////////////////////////////////

#if (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_FINGERPRINT)) or (defined(CHECK_FINGERPRINT) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT) and defined(CHECK_FINGERPRINT))
#error "cant have both CHECK_CA_ROOT and CHECK_PUB_KEY enabled"
#endif

// Generate WiFi network instance
#if defined(ESP32)
    WiFiClientSecure net;
#elif defined(ESP8266)
    BearSSL::WiFiClientSecure net;
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
void mqtt_connect();

//
// Setup WiFi in Station Mode
//
void mqtt_setup()
{
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.print(ssid);
    WiFi.hostname(HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println(F("connected!"));
    
    /*
    Serial.print("Setting time using SNTP ");
    configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
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
    Serial.print(asctime(&timeinfo));
    */

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
        net.setInsecure();
    #endif

    client.begin(MQTT_HOST, MQTT_PORT, net);
    
    // set up MQTT receive callback (if required)
    //client.onMessage(messageReceived);
    client.setWill(MQTT_PUB_STATUS, "dead", true /* retained*/, 1 /* qos */);
    mqtt_connect();
}


//
// (Re-)Connect to WLAN and connect MQTT broker
//
void mqtt_connect()
{
    Serial.print(F("Checking wifi..."));
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.print(F("\nMQTT connecting... "));
    while (!client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.println(F("connected!"));
    //client.subscribe(MQTT_SUB_IN);
    client.publish(MQTT_PUB_STATUS, "online");
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
    DynamicJsonDocument payload(PAYLOAD_SIZE);
    char mqtt_payload[PAYLOAD_SIZE];
    char sensor_id[11];

    // ArduinoJson does not allow to set number of decimals for floating point data -
    // neither does MQTT Dashboard...
    // Therefore the JSON string is created manually. 
    
    // Example:
    // {"sensor_id":"0x12345678","ch":0,"battery_ok":true,"humidity":44,"wind_gust":1.2,"wind_avg":1.2,"wind_dir":150,"rain":146}
    sprintf(mqtt_payload, "{\"sensor_id\": %8X", weatherSensor.sensor_id);
    #ifdef BRESSER_6_IN_1
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"ch:\":%d", weatherSensor.chan);
    #endif
    sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"battery_ok\":%d", weatherSensor.battery_ok ? 1 : 0);
    if (weatherSensor.temp_ok || complete) {
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"temp_c\":%.1f", weatherSensor.temp_c);
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"humidity\":%d", weatherSensor.humidity);
    }
    if (weatherSensor.wind_ok || complete) {
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"wind_gust\":%.1f", weatherSensor.wind_gust_meter_sec);
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"wind_avg\":%.1f", weatherSensor.wind_avg_meter_sec);
        #ifdef BFT_EN
            sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"wind_gust_bft\":%d", windspeed_ms_to_bft(weatherSensor.wind_gust_meter_sec));
            sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"wind_avg_bft\":%d", windspeed_ms_to_bft(weatherSensor.wind_avg_meter_sec));        
        #endif
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"wind_dir\":%.1f", weatherSensor.wind_direction_deg);
    }
    if (weatherSensor.uv_ok) {
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"uv\":%.1f,", weatherSensor.uv);
    }
    if (weatherSensor.rain_ok || complete) {
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"rain\":%.1f", weatherSensor.rain_mm);
    }
    if (weatherSensor.moisture_ok) {
        sprintf(&mqtt_payload[strlen(mqtt_payload)], ",\"moisture\":%d", weatherSensor.moisture);
    }
    sprintf(&mqtt_payload[strlen(mqtt_payload)], "}");
    Serial.println(mqtt_payload);
    client.publish(MQTT_PUB_DATA, &mqtt_payload[0], false, 0);
}


//
// Publish CC1101 radio receiver info as JSON string via MQTT
// - RSSI: Received Signal Strength Indication
// - LQI:  Link Quality Indicator
//
void publishRadio(void)
{
    DynamicJsonDocument payload(PAYLOAD_SIZE);
    char mqtt_payload[PAYLOAD_SIZE];
    
    payload["rssi"] = weatherSensor.moisture;
    serializeJson(payload, mqtt_payload);
    Serial.println(mqtt_payload);
    client.publish(MQTT_PUB_RADIO, &mqtt_payload[0], false, 0);
    payload.clear();
}


//
// Setup
//
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.println(sketch_id);
    Serial.println();

    mqtt_setup();
    weatherSensor.begin();
    
    #ifdef LED_EN
        // Configure LED output pins
        pinMode(LED_GPIO, OUTPUT);
        digitalWrite(LED_GPIO, HIGH);
    #endif
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
        client.publish(MQTT_PUB_STATUS, "online");
        publishRadio();
    }

    bool decode_ok = false;
    #ifdef _DEBUG_MQTT_
        decode_ok = genWeatherdata();
    #else

        // Attempt to receive entire data set with timeout of 30s and callback function
        decode_ok = weatherSensor.getData(30000, true, &clientLoopWrapper);
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
            publishWeatherdata(true);
    }

    bool force_sleep = millis() > AWAKE_TIMEOUT;

    // Go to sleep only after complete set of data has been sent
    if (SLEEP_EN && (decode_ok || force_sleep)) {
        #ifdef _DEBUG_MODE_
            if (force_sleep) {
                Serial.println(F("Awake time-out!"));
                Serial.printf("temp_ok: %d, rain_ok: %d\n", weatherSensor.temp_ok, weatherSensor.rain_ok);
            } else {
                Serial.println(F("Data forwarding completed.")); 
            }
        #endif
        Serial.printf("Sleeping for %d ms\n", SLEEP_INTERVAL); 
        Serial.flush();
        client.publish(MQTT_PUB_STATUS, "offline", true /* retained */, 0 /* qos */);
        client.disconnect();
        client.loop();
        #ifdef LED_EN
            pinMode(LED_GPIO, INPUT);
        #endif
        ESP.deepSleep(SLEEP_INTERVAL * 1000);
    }
} // loop()
