///////////////////////////////////////////////////////////////////////////////////////////////////
// WeatherSensorCfg.h
//
// Bresser 5-in-1/6-in-1 868 MHz Weather Sensor Radio Receiver 
// based on CC1101 or SX1276/RFM95W and ESP32/ESP8266
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
// NOTE: Application/hardware specific configurations should be made in this file!
//
// created: 05/2022
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
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(WEATHER_SENSOR_CFG_H)
#define WEATHER_SENSOR_CFG_H

#include <Arduino.h>

// Replacement for
// https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h
// on ESP8266
//
// NOTE:
// DEBUG_ESP_PORT is set in Arduino IDE:
// Tools->Debug port: "<None>|<Serial>|<Serial1>"
#if defined(ESP8266)
    #define ARDUHAL_LOG_LEVEL_NONE      0
    #define ARDUHAL_LOG_LEVEL_ERROR     1
    #define ARDUHAL_LOG_LEVEL_WARN      2
    #define ARDUHAL_LOG_LEVEL_INFO      3
    #define ARDUHAL_LOG_LEVEL_DEBUG     4
    #define ARDUHAL_LOG_LEVEL_VERBOSE   5
    
    // Set desired level here!
    #define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE

    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_NONE
        #define log_e(...) { DEBUG_ESP_PORT.printf(__VA_ARGS__); DEBUG_ESP_PORT.println(); }
     #else
        #define log_e(...) {}
     #endif
    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_ERROR
        #define log_w(...) { DEBUG_ESP_PORT.printf(__VA_ARGS__); DEBUG_ESP_PORT.println(); }
     #else
        #define log_w(...) {}
     #endif
    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_WARN
        #define log_i(...) { DEBUG_ESP_PORT.printf(__VA_ARGS__); DEBUG_ESP_PORT.println(); }
     #else
        #define log_i(...) {}
     #endif
    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_INFO
        #define log_d(...) { DEBUG_ESP_PORT.printf(__VA_ARGS__); DEBUG_ESP_PORT.println(); }
     #else
        #define log_d(...) {}
     #endif
    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_DEBUG
        #define log_v(...) { DEBUG_ESP_PORT.printf(__VA_ARGS__); DEBUG_ESP_PORT.println(); }
     #else
        #define log_v(...) {}
     #endif
     
#endif

//#define _DEBUG_MODE_          // Enable debug output (serial console)
#define DEBUG_PORT Serial
#if defined(_DEBUG_MODE_)
    #define DEBUG_PRINT(...) { DEBUG_PORT.print(__VA_ARGS__); }
    #define DEBUG_PRINTLN(...) { DEBUG_PORT.println(__VA_ARGS__); }
#else
  #define DEBUG_PRINT(...) {}
  #define DEBUG_PRINTLN(...) {}
#endif


#define NUM_SENSORS     1       // Number of sensors to be received

// List of sensor IDs to be excluded - can be empty
#define SENSOR_IDS_EXC {}
//#define SENSOR_IDS_EXC { 0x39582376 }

// List of sensor IDs to be included - if empty, handle all available sensors
#define SENSOR_IDS_INC {}
//#define SENSOR_IDS_INC { 0x83750871 }

// Use pinning for LoRaWAN Node 
//#define LORAWAN_NODE

// Use pinning for TTGO ESP32 boards with integrated RF tranceiver (SX1276)
// https://github.com/espressif/arduino-esp32/tree/master/variants/ttgo-lora32-*
//#define TTGO_LORA32

// Use pinning for Adafruit Feather ESP32S2 with RFM95W "FeatherWing" ADA3232
//#define ADAFRUIT_FEATHER_ESP32S2

// Disable data type which will not be used to save RAM
#define WIND_DATA_FLOATINGPOINT
#define WIND_DATA_FIXEDPOINT

// Select appropriate sensor message format(s)
#define BRESSER_5_IN_1
#define BRESSER_6_IN_1

// Select type of receiver module
#define USE_CC1101
//#define USE_SX1276

#if ( !defined(BRESSER_5_IN_1) && !defined(BRESSER_6_IN_1) )
    #error "Either BRESSER_5_IN_1 and/or BRESSER_6_IN_1 must be defined!"
#endif

#if ( defined(USE_CC1101) && defined(USE_SX1276) )
    #error "Either USE_CC1101 OR USE_SX1276 must be defined!"
#endif

#if defined(USE_CC1101)
    #define RECEIVER_CHIP "[CC1101]"
#elif defined(USE_SX1276)
    #define RECEIVER_CHIP "[SX1276]"
#else
    #error "Either USE_CC1101 or USE_SX1276 must be defined!"
#endif


// Arduino default SPI pins
//
// Board   SCK   MOSI  MISO
// ESP8266 D5    D7    D6
// ESP32   D18   D23   D19
#if defined(LORAWAN_NODE)
    #define PIN_RECEIVER_CS   14
    
    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  4 
    
    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 16
    
    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  12
#elif defined(TTGO_LORA32)
    #define PIN_RECEIVER_CS   LORA_CS
    
    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  LORA_IRQ
    
    // CC1101: GDO2 / RFM95W/SX127x: G1
    // n.c. on v1/v2?, LORA_D1 on v21
    #define PIN_RECEIVER_GPIO 33
    
    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  LORA_RST
#elif defined(ADAFRUIT_FEATHER_ESP32S2)
    #define PIN_RECEIVER_CS   6
    
    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  5
    
    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 11
    
    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  9
#elif defined(ESP32)
    #define PIN_RECEIVER_CS   27
    
    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  21 
    
    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 33
    
    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  32

#elif defined(ESP8266)
    #define PIN_RECEIVER_CS   15
    
    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  4 
    
    // CC1101: GDO2 / RFM95W/SX127x: 
    #define PIN_RECEIVER_GPIO 5 
    
    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  2
#endif

#endif
