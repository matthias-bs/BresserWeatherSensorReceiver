///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorCanvasGauges.ino
//
// Example for BresserWeatherSensorReceiver
//
// This sketch provides a web server to display sensor readings in gauges. Two different tpes of
// gauges are used: linear and radial. The gauges are implemented using the JavaScript library
// canvas-gauges (https://github.com/Mikhus/canvas-gauges).
//
// The web server serves a simple HTML page with CSS and embedded JavaScript stored in the ESP
// SPIFFS file system to fetch the sensor readings. The readings are updated automatically on
// the web page using Server-Sent Events (SSE).
// See "ESP32 Web Server: Display Sensor Readings in Gauges" by Rui Santos
// on Random Nerd Tutorials (https://randomnerdtutorials.com/esp32-web-server-gauges/) for details.
//
// Notes:
// - Set your WiFi credentials in "secrets.h"
// - Enable WiFi Access Point mode by uncommenting WIFI_AP_MODE if desired
// - Open http://weatherdashboard.local in your web browser (or the IP address shown in the
//   serial monitor) to access the web page
// - Press the on-board button during power-up to reset rain gauge data
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
// Based on:
// Rui Santos & Sara Santos - Random Nerd Tutorials
// Complete instructions at https://RandomNerdTutorials.com/esp32-web-server-gauges/
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files.
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
//
// created: 10/2025
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
// 20251003 Created
//
// To Do:
// - Improved page layout
// - SSL/TLS support (optional)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h> // https://github.com/ESP32Async/AsyncTCP
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h> // https://github.com/ESP32Async/ESPAsyncWebServer
#include <time.h>
#include <LittleFS.h>
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "RainGauge.h"
#include "secrets.h"

#define MAX_SENSORS 2
#define RX_TIMEOUT 180000 // sensor receive timeout [ms]

// Raingauge reset button definition - unfortunately a common definition does not exist!
#if defined(ARDUINO_ARCH_ESP32)
#if defined(ARDUINO_LILYGO_T3S3_SX1262) || \
    defined(ARDUINO_LILYGO_T3S3_SX1276) || \
    defined(ARDUINO_LILYGO_T3S3_LR1121)
const uint8_t KEY_RAINGAUGE_RESET = (BUTTON_1);
#elif defined(ARDUINO_DFROBOT_FIREBEETLE_ESP32)
const uint8_t KEY_RAINGAUGE_RESET = 0;
#elif defined(ARDUINO_HELTEC_WIRELESS_STICK) || \
      defined(ARDUINO_HELTEC_WIFI_LORA_32_V3) || \
      defined(ARDUINO_HELTEC_VISION_MASTER_T190)
// Check if this GPIO pin is available/connected to a key on your board
const uint8_t KEY_RAINGAUGE_RESET = 0;
#elif defined(ARDUINO_FEATHER_ESP32) || \
      defined(ARDUINO_THINGPULSE_EPULSE_FEATHER) || \
      defined(ARDUINO_ADAFRUIT_FEATHER_ESP32_V2) || \
      defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
// Check if this GPIO pin is available/connected to a key on your board
const uint8_t KEY_RAINGAUGE_RESET = 4;
#else
const uint8_t KEY_RAINGAUGE_RESET = KEY_BUILTIN;
#endif
#else
// Check if this GPIO pin is available/connected to a key on your board
const uint8_t KEY_RAINGAUGE_RESET = 5;
#endif

// Stop reception when data of at least one sensor is complete
// #define RX_FLAGS DATA_COMPLETE

// Stop reception when data of all (max_sensors) is complete
#define RX_FLAGS (DATA_COMPLETE | DATA_ALL_SLOTS)

// #define WIFI_AP_MODE // Uncomment to enable WiFi Access Point mode

// Replace network credentials in secrets.h

// Station Mode - connect to existing WiFi network
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// Access Point Mode - create own WiFi network
const char *ap_ssid = WIFI_AP_SSID;
const char *ap_password = WIFI_AP_PASSWORD;

// Set web server hostname (max. 31 characters)
const char *hostname = "WeatherDashboard";

// Time zone string (POSIX format)
// Example: "CET-1CEST,M3.5.0/02:00:00,M10.5.0/03:00:00"
// - CET: Central European Time
// - -1: Offset from UTC (UTC+1)
// - CEST: Central European Summer Time
// - M3.5.0/02:00:00: DST starts on the last Sunday of March at 2:00 AM
// - M10.5.0/03:00:00: DST ends on the last Sunday of October at 3:00 AM
const char *tz = "CET-1CEST,M3.5.0/02:00:00,M10.5.0/03:00:00";

// NTP server
const char *ntpServer = "pool.ntp.org";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// JSON Variable to hold Sensor Readings
JsonDocument readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

// Create weather sensor receiver object
WeatherSensor weatherSensor;

// Create rain gauge object
RainGauge rainGauge;

// Get Sensor Readings and return JSON object
String getSensorReadingsBWS()
{
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

    log_i("%d: type=%d", i, weatherSensor.sensor[i].s_type);
    if ((weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
        (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
        (weatherSensor.sensor[i].s_type == SENSOR_TYPE_WEATHER2))
    {
      if (weatherSensor.sensor[i].w.temp_ok)
        readings["ws_temp_c"] = String(weatherSensor.sensor[i].w.temp_c);
      if (weatherSensor.sensor[i].w.humidity_ok)
        readings["ws_humidity"] = String(weatherSensor.sensor[i].w.humidity);
      if (weatherSensor.sensor[i].w.wind_ok)
      {
        readings["ws_wind_gust_ms"] = String(weatherSensor.sensor[i].w.wind_gust_meter_sec);
        readings["ws_wind_avg_ms"] = String(weatherSensor.sensor[i].w.wind_avg_meter_sec);
        readings["ws_wind_dir_deg"] = String(weatherSensor.sensor[i].w.wind_direction_deg);
      }
    }

    readings["ws_rain_h"] = String(rainGauge.pastHour());
    readings["ws_rain_d"] = String(rainGauge.currentDay());
    readings["ws_rain_w"] = String(rainGauge.currentWeek());
    readings["ws_rain_m"] = String(rainGauge.currentMonth());
  }
  String jsonString;
  serializeJson(readings, jsonString);
  return jsonString;
}

// Initialize LittleFS
void initLittleFS()
{
  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Initialize WiFi
void initWiFi()
{
#ifdef WIFI_AP_MODE
  if (!WiFi.softAP(ap_ssid, ap_password))
  {
    log_e("Soft AP creation failed.");
    while (1)
      delay(100);
    ;
  }
  IPAddress myIP = WiFi.softAPIP();
  log_i("AP IP: %s", myIP.toString().c_str());
#else
  WiFi.hostname(hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  log_i("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  log_i("Local IP: %s", WiFi.localIP().toString().c_str());
  if (!MDNS.begin(hostname))
  {
    log_e("Error setting up MDNS responder!");
  }
#endif
}


// Send ping message during weather sensor reception
void sendPing()
{
  static unsigned long lastTime;
  if ((millis() - lastTime) > timerDelay)
  {
    events.send("ping", NULL, millis());
    lastTime = millis();
  }
  yield();
}


// Helper in place of getLocalTime usage (ESP32/ESP8266 compatibility)
bool getLocalTimeCompat(struct tm *info, uint32_t timeout_ms = 2000)
{
  uint32_t start = millis();
  while (millis() - start < timeout_ms)
  {
    time_t now = time(nullptr);
    if (now > 100000)
    { // NTP synced
#if defined(ARDUINO_ARCH_ESP8266)
      struct tm *tmp = localtime(&now);
      if (!tmp)
      {
        delay(10);
        continue;
      }
      *info = *tmp;
#else
      localtime_r(&now, info);
#endif
      return true;
    }
    delay(10);
  }
  return false;
}

// Print local time
void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTimeCompat(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  char buf[64];
  strftime(buf, sizeof(buf), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  Serial.println(buf);
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(KEY_RAINGAUGE_RESET, INPUT);

  if (digitalRead(KEY_RAINGAUGE_RESET) == LOW)
  {
    log_i("Resetting rain gauge data");
    rainGauge.reset();
  }

  initWiFi();
  initLittleFS();

  setenv("TZ", tz, 1); // Set the time zone
  tzset();             // Apply the time zone

#ifndef WIFI_AP_MODE
  // Configure time with NTP
  configTime(0, 0, ntpServer); // 0, 0: Use time zone string for offset

  // Wait for time to be set
  log_i("Waiting for NTP time sync");
  while (time(nullptr) < 100000)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  log_i("Time synchronized");

  printLocalTime();
#endif

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });

  server.serveStatic("/", LittleFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              String json = getSensorReadingsBWS();
              request->send(200, "application/json", json);
              json = String();
            });

  // Endpoint to set time via query parameter "epoch" (unix time in seconds)
  // This is required for ESP in WiFi AP mode - no connection to internet and
  // thus NTP server.
  server.on("/settime", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
#ifdef WIFI_AP_MODE
              if (!request->hasParam("epoch"))
              {
                request->send(400, "text/plain", "missing epoch");
                return;
              }
              String s = request->getParam("epoch")->value();
              time_t epoch = (time_t)s.toInt();
              timeval tv = {.tv_sec = epoch, .tv_usec = 0};
              if (settimeofday(&tv, NULL) == 0)
                request->send(200, "text/plain", "OK");
              else
                request->send(500, "text/plain", "settimeofday failed");
#else
              request->send(200, "text/plain", "OK (ignored in STA mode)");
#endif
            });

  events.onConnect(
      [](AsyncEventSourceClient *client)
      {
        if (client->lastId())
        {
          Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
        }
        // send event with message "hello!", id current millis
        // and set reconnect delay to 1 second
        client->send("hello!", NULL, millis(), 10000);
      });
  server.addHandler(&events);

  weatherSensor.begin();
  weatherSensor.setSensorsCfg(MAX_SENSORS, RX_FLAGS);

  // Start server
  server.begin();
}

void loop()
{
  // Clear sensor data buffer
  weatherSensor.clearSlots();

  // Attempt to receive data set with timeout of <xx> s
  bool decode_ok = weatherSensor.getData(RX_TIMEOUT, RX_FLAGS, 0, &sendPing);
  log_i("decode_ok: %d", decode_ok);

  events.send("ping", NULL, millis());
  events.send(getSensorReadingsBWS().c_str(), "new_readings", millis());
}



