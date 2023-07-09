///////////////////////////////////////////////////////////////////////////////////////////////////
// WeatherSensorCfg.h
//
// Bresser 5-in-1/6-in-1/7-in-1 868 MHz Weather Sensor Radio Receiver
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
// 20230124 Added some default settings based on selected boards in Arduino IDE
// 20230207 Added pin definitions for ARDUINO_TTGO_LoRa32_v21new
// 20230208 Added pin definitions for ARDUINO_TTGO_LoRa32_V2
// 20230301 Added pin definitions for Wireless_Stick (from Heltec)
// 20230316 Added pin definitions for Adafruit Feather ESP32 with RFM95W "FeatherWing" ADA3232
// 20230330 Added pin definitions and changes for Adafruit Feather 32u4 (AVR) RFM95 LoRa Radio 
// 20230412 Added workaround for Professional Wind Gauge / Anemometer, P/N 7002531
// 20230420 Added pin definitions for DFRobot FireBeetle ESP32 with FireBeetle Cover LoRa
// 20230607 Added pin definitions for Heltec WiFi LoRa 32(V2)
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(WEATHER_SENSOR_CFG_H)
#define WEATHER_SENSOR_CFG_H

#include <Arduino.h>

// ------------------------------------------------------------------------------------------------
// --- Board ---
// ------------------------------------------------------------------------------------------------
// Use pinning for LoRaWAN Node


// LILIGO TTGO LoRaP32 board with integrated RF tranceiver (SX1276)
// See pin definitions in
// https://github.com/espressif/arduino-esp32/tree/master/variants/ttgo-lora32-*
// and
// https://www.thethingsnetwork.org/forum/t/big-esp32-sx127x-topic-part-2/11973

// This define is set by selecting "Board: TTGO LoRa32-OLED" / "Board Revision: TTGO LoRa32 V1 (No TFCard)"
// in the Arduino IDE:
//#define ARDUINO_TTGO_LoRa32_V1

// This define is set by selecting "Board: TTGO LoRa32-OLED" / "Board Revision: TTGO LoRa32 V2"
// in the Arduino IDE:
//#define ARDUINO_TTGO_LoRa32_V2

// This define is set by selecting "Board: TTGO LoRa32-OLED" / "Board Revision: TTGO LoRa32 V2.1 (1.6.1)"
// in the Arduino IDE:
//#define ARDUINO_TTGO_LoRa32_V21new

// This define is set by selecting "Board: Heltec Wireless Stick"
// in the Arduino IDE:
//#define ARDUINO_heltec_wireless_stick

// This define is set by selecting "Board: Heltec WiFi LoRa 32(V2)"
// in the Adruino IDE:
//#define ARDUINO_heltec_wifi_lora_32_V2

// Adafruit Feather ESP32S2 with RFM95W "FeatherWing" ADA3232
// https://github.com/espressif/arduino-esp32/blob/master/variants/adafruit_feather_esp32s2/pins_arduino.h
//
// This define is set by selecting "Adafruit Feather ESP32-S2" in the Arduino IDE:
//#define ARDUINO_ADAFRUIT_FEATHER_ESP32S2

// Adafruit Feather ESP32 with RFM95W "FeatherWing" ADA3232
// https://github.com/espressif/arduino-esp32/blob/master/variants/feather_esp32/pins_arduino.h
//
// This define is set by selecting "Adafruit ESP32 Feather" in the Arduino IDE:
//#define ARDUINO_FEATHER_ESP32

// DFRobot Firebeetle32
// https://github.com/espressif/arduino-esp32/tree/master/variants/firebeetle32/pins_arduino.h
//
// This define (not very specific...) is set by selecting "FireBeetle-ESP32" in the Arduino IDE:
//#define ARDUINO_ESP32_DEV

#if defined(ARDUINO_TTGO_LoRa32_V1)
    #pragma message("ARDUINO_TTGO_LoRa32_V1 defined; using on-board transceiver")
    #define USE_SX1276

#elif defined(ARDUINO_TTGO_LoRa32_V2)
    #pragma message("ARDUINO_TTGO_LoRa32_V2 defined; using on-board transceiver")
    #pragma message("LoRa DIO1 must be wired to GPIO33 manually!")
    #define USE_SX1276

#elif defined(ARDUINO_TTGO_LoRa32_v21new)
    #pragma message("ARDUINO_TTGO_LoRa32_V21new defined; using on-board transceiver")
    #define USE_SX1276

#elif defined(ARDUINO_heltec_wireless_stick)
    #pragma message("ARDUINO_heltec_wireless_stick defined; using on-board transceiver")
    #define USE_SX1276

#elif defined(ARDUINO_heltec_wifi_lora_32_V2)
    #pragma message("ARDUINO_heltec_wifi_lora_32_V2 defined; using on-board transceiver")
    #define USE_SX1276

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_ESP32S2 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276

#elif defined(ARDUINO_FEATHER_ESP32)
    #pragma message("ARDUINO_FEATHER_ESP32 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")

#elif defined(ARDUINO_AVR_FEATHER32U4)
    #pragma message("ARDUINO_AVR_FEATHER32U4 defined; assuming this is the Adafruit Feather 32u4 RFM95 LoRa Radio")
    #define USE_SX1276
    
#elif defined(ARDUINO_ESP32_DEV)
    //#define LORAWAN_NODE
    #define FIREBEETLE_ESP32_COVER_LORA
    
    #if !defined(LORAWAN_NODE) && !defined(FIREBEETLE_ESP32_COVER_LORA)
        #pragma message("ARDUINO_ESP32_DEV defined; select either LORAWAN_NODE or FIREBEETLE_ESP32_COVER_LORA manually!")
        
    #elif defined(LORAWAN_NODE) 
        #pragma message("LORAWAN_NODE defined; assuming this is the LoRaWAN_Node board (DFRobot Firebeetle32 + Adafruit RFM95W LoRa Radio)")
        #define USE_SX1276

    #elif defined(FIREBEETLE_ESP32_COVER_LORA)
        #define USE_SX1276
        #pragma message("FIREBEETLE_ESP32_COVER_LORA defined; assuming this is a FireBeetle ESP32 with FireBeetle Cover LoRa")
        #pragma message("Required wiring: D2 to RESET, D3 to DIO0, D4 to CS, D5 to DIO1")
    
    #endif
#endif


// ------------------------------------------------------------------------------------------------
// --- Radio Transceiver ---
// ------------------------------------------------------------------------------------------------
// Select type of receiver module (if not yet defined based on the assumptions above)
#if ( !defined(USE_CC1101) && !defined(USE_SX1276) )
    #define USE_CC1101
    //#define USE_SX1276
#endif


// ------------------------------------------------------------------------------------------------
// --- Weather Sensors ---
// ------------------------------------------------------------------------------------------------
#define NUM_SENSORS     1       // Number of sensors to be received

// List of sensor IDs to be excluded - can be empty
#define SENSOR_IDS_EXC {}
//#define SENSOR_IDS_EXC { 0x39582376 }

// List of sensor IDs to be included - if empty, handle all available sensors
#define SENSOR_IDS_INC {}
//#define SENSOR_IDS_INC { 0x83750871 }

// List of sensor IDs of the model "BRESSER 3-in-1 Professional Wind Gauge / Anemometer"
// P/N 7002531 - requiring special heandling in decodeBresser5In1Payload()
//#define SENSOR_IDS_DECODE3IN1 {}
#define SENSOR_IDS_DECODE3IN1 { 0x2C100512 }

// ------------------------------------------------------------------------------------------------
// --- Debug Logging Output ---
// ------------------------------------------------------------------------------------------------
// - ESP32:
//   CORE_DEBUG_LEVEL is set in Adruino IDE:
//   Tools->Core Debug Level: "<None>|<Error>|<Warning>|<Info>|<Debug>|<Verbose>"
//   https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h
//
// - ESP8266:
//   DEBUG_ESP_PORT is set in Arduino IDE:
//   Tools->Debug port: "<None>|<Serial>|<Serial1>"
//
//   Replacement for
//   https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h
//   on ESP8266:
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


//   Replacement for
//   https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h
//   on Arduino AVR:
#if defined(ARDUINO_ARCH_AVR)
    #define ARDUHAL_LOG_LEVEL_NONE      0
    #define ARDUHAL_LOG_LEVEL_ERROR     1
    #define ARDUHAL_LOG_LEVEL_WARN      2
    #define ARDUHAL_LOG_LEVEL_INFO      3
    #define ARDUHAL_LOG_LEVEL_DEBUG     4
    #define ARDUHAL_LOG_LEVEL_VERBOSE   5

    // Set desired level here!
    #define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_NONE
        #define log_e(...) { printf(__VA_ARGS__); println(); }
     #else
        #define log_e(...) {}
     #endif
    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_ERROR
        #define log_w(...) { printf(__VA_ARGS__); println(); }
     #else
        #define log_w(...) {}
     #endif
    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_WARN
        #define log_i(...) { printf(__VA_ARGS__); println(); }
     #else
        #define log_i(...) {}
     #endif
    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_INFO
        #define log_d(...) { printf(__VA_ARGS__); println(); }
     #else
        #define log_d(...) {}
     #endif
    #if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_DEBUG
        #define log_v(...) { printf(__VA_ARGS__); println(); }
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

// Disable data type which will not be used to save RAM
#define WIND_DATA_FLOATINGPOINT
#define WIND_DATA_FIXEDPOINT

// Select appropriate sensor message format(s)
#define BRESSER_5_IN_1
#define BRESSER_6_IN_1
#define BRESSER_7_IN_1

#if ( !defined(BRESSER_5_IN_1) && !defined(BRESSER_6_IN_1) && !defined(BRESSER_7_IN_1) )
    #error "Either BRESSER_5_IN_1 and/or BRESSER_6_IN_1 and/or BRESSER_7_IN_1 must be defined!"
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
    // Use pinning for LoRaWAN_Node (https://github.com/matthias-bs/LoRaWAN_Node)
    #define PIN_RECEIVER_CS   14

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  4

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 16

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  12

#elif defined(FIREBEETLE_ESP32_COVER_LORA)
    #define PIN_RECEIVER_CS   27 // D4

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  26 // D3

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 9  // D5

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  25 // D2

#elif defined(ARDUINO_TTGO_LoRa32_V1) || defined(ARDUINO_TTGO_LoRa32_V2)
    // Use pinning for LILIGO TTGO LoRa32-OLED
    #define PIN_RECEIVER_CS   LORA_CS

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  LORA_IRQ

    // CC1101: GDO2 / RFM95W/SX127x: G1
    // n.c. on v1/v2?, LORA_D1 on v21
    #define PIN_RECEIVER_GPIO 33

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  LORA_RST

#elif defined(ARDUINO_TTGO_LoRa32_v21new)
    // Use pinning for LILIGO TTGO LoRa32-OLED V2.1 (1.6.1)
    // Same pinout for Heltec Wireless Stick
    #define PIN_RECEIVER_CS   LORA_CS

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  LORA_IRQ

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO LORA_D1

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  LORA_RST

#elif defined(ARDUINO_heltec_wireless_stick) || defined(ARDUINO_heltec_wifi_lora_32_V2)
    // Use pinning for Heltec Wireless Stick or WiFi LoRa32 V2, respectively
    #define PIN_RECEIVER_CS   SS

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  DIO0

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO DIO1

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  RST_LoRa

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
    // Use pinning for Adafruit Feather ESP32S2 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   6

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  5

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 11

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  9

#elif defined(ARDUINO_FEATHER_ESP32)
    // Use pinning for Adafruit Feather ESP32 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   14

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  32

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 33

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  27

#elif defined(ESP32)
    // Generic pinning for ESP32 development boards
    #define PIN_RECEIVER_CS   27

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  21

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 33

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  32
    
#elif defined(ESP8266)
    // Generic pinning for ESP8266 development boards (e.g. LOLIN/WEMOS D1 mini)
    #define PIN_RECEIVER_CS   15

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  4

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 5

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  2
    
#elif defined(ARDUINO_AVR_FEATHER32U4)
    // Pinning for Adafruit Feather 32u4 
    #define PIN_RECEIVER_CS   8

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  7

    // CC1101: GDO2 / RFM95W/SX127x: G1 (not used)
    #define PIN_RECEIVER_GPIO 99

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  4

#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message("Receiver chip: " RECEIVER_CHIP)
#pragma message("Pin config: RST->" STR(PIN_RECEIVER_RST) ", CS->" STR(PIN_RECEIVER_CS) ", GD0/G0/IRQ->" STR(PIN_RECEIVER_IRQ) ", GDO2/G1/GPIO->" STR(PIN_RECEIVER_GPIO) )

#endif
