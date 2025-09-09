///////////////////////////////////////////////////////////////////////////////////////////////////
// WeatherSensorCfg.h
//
// Bresser 5-in-1/6-in-1/7-in-1 868 MHz Weather Sensor Radio Receiver
// based on CC1101, SX1276/RFM95W, SX1262 or LR1121 and ESP32/ESP8266
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
// 20230124 Added some default settings based on selected boards in Arduino IDE
// 20230207 Added pin definitions for ARDUINO_TTGO_LoRa32_v21new
// 20230208 Added pin definitions for ARDUINO_TTGO_LoRa32_V2
// 20230301 Added pin definitions for Wireless_Stick (from Heltec)
// 20230316 Added pin definitions for Adafruit Feather ESP32 with RFM95W "FeatherWing" ADA3232
// 20230330 Added pin definitions and changes for Adafruit Feather 32u4 (AVR) RFM95 LoRa Radio 
// 20230412 Added workaround for Professional Wind Gauge / Anemometer, P/N 7002531
// 20230420 Added pin definitions for DFRobot FireBeetle ESP32 with FireBeetle Cover LoRa
// 20230607 Added pin definitions for Heltec WiFi LoRa 32(V2)
// 20230624 Added Bresser Lightning Sensor decoder
// 20230804 Added Bresser Water Leakage Sensor decoder
// 20230926 Added pin definitions for Adafruit Feather RP2040 with RFM95W "FeatherWing" ADA3232
// 20230927 Removed _DEBUG_MODE_ (log_d() is used instead)
// 20231004 Added function names and line numbers to ESP8266/RP2040 debug logging
// 20231101 Added USE_SX1262 for Heltec Wireless Stick V3
// 20231121 Added Heltec WiFi LoRa32 V3
// 20231130 Bresser 3-in-1 Professional Wind Gauge / Anemometer, PN 7002531: Replaced workaround 
//          for negative temperatures by fix (6-in-1 decoder)
// 20231227 Added RAINGAUGE_USE_PREFS and LIGHTNING_USE_PREFS
// 20240122 Modified for unit testing
// 20240204 Added separate ARDUINO_heltec_wireless_stick_v2/v3
// 20240322 Added pin definitions for M5Stack Core2 with M5Stack Module LoRa868
// 20240415 Added pin definitions for ESP32-S3 PowerFeather with with RFM95W "FeatherWing" ADA3232
// 20240417 Modified SENSOR_IDS_INC
// 20240425 Added define variant ARDUINO_heltec_wifi_lora_32_V3
// 20240507 Renamed NUM_SENSORS to MAX_SENSORS_DEFAULT
//          NOTE: ARDUINO_ARCH_AVR no longer supported due to code size!!!
// 20240508 Updated board definitions after changes in arduino-esp32 v3.0.0
// 20240509 Fixed ARDUINO_HELTEC_WIRELESS_STICK_V3
// 20240904 Added ARDUINO_ESP32S3_DEV
// 20240910 Heltec: Fixed pin definitions
// 20241030 Added pin definitions for Maker Go ESP32C3 Supermini with Heltec HT-RA62
// 20241130 Added pin definitions for Heltec Vision Master T190
// 20241205 Added pin definitions for Lilygo T3-S3 (SX1262/SX1276/LR1121)
// 20241227 Improved maintainability of board definitions
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(WEATHER_SENSOR_CFG_H)
#define WEATHER_SENSOR_CFG_H

#include <Arduino.h>

// ------------------------------------------------------------------------------------------------
// --- Weather Sensors ---
// ------------------------------------------------------------------------------------------------
#define MAX_SENSORS_DEFAULT 1       // Maximum number of sensors to be received

// List of sensor IDs to be excluded - can be empty
#define SENSOR_IDS_EXC { 0x792882A2 }
//#define SENSOR_IDS_EXC { 0x792882A2 }

// List of sensor IDs to be included - if empty, handle all available sensors
#define SENSOR_IDS_INC { }
//#define SENSOR_IDS_INC { 0x83750871 }

// Maximum number of sensor IDs in include/exclude list
#define MAX_SENSOR_IDS 12

// Disable data type which will not be used to save RAM
#define WIND_DATA_FLOATINGPOINT
#define WIND_DATA_FIXEDPOINT

// Select appropriate sensor message format(s)
// Comment out unused decoders to save operation time/power
#define BRESSER_5_IN_1
#define BRESSER_6_IN_1
#define BRESSER_7_IN_1
#define BRESSER_LIGHTNING
#define BRESSER_LEAKAGE


// ------------------------------------------------------------------------------------------------
// --- Rain Gauge / Lightning sensor data retention during deep sleep ---
// ------------------------------------------------------------------------------------------------

#if !defined(INSIDE_UNITTEST)
    #if defined(ESP32)
        // Option: Comment out to save data in RTC RAM
        // N.B.:
        // ESP8266 has RTC RAM, too, but access is different from ESP32
        // and currently not implemented here
        #define RAINGAUGE_USE_PREFS
        #define LIGHTNING_USE_PREFS
    #else
        // Using Preferences is mandatory on other architectures (e.g. RP2040)
        #define RAINGAUGE_USE_PREFS
        #define LIGHTNING_USE_PREFS
    #endif
#endif

// ------------------------------------------------------------------------------------------------
// --- Board ---
// ------------------------------------------------------------------------------------------------

// LILYGO TTGO LoRaP32 board with integrated RF transceiver (SX1276)
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

// These defines are set by selecting
// "Board: LilyGo T3-S3" / "Board Revision: Radio-SX1262|Radio-SX1276|Radio-LR1121" in the Arduino IDE:
//#define ARDUINO_LILYGO_T3S3_SX1262
//#define ARDUINO_LILYGO_T3S3_SX1276
//#define ARDUINO_LILYGO_T3S3_LR1121

// This define is set by selecting "Board: Heltec Wireless Stick" (SX1276) in the Arduino IDE:
//#define ARDUINO_HELTEC_WIRELESS_STICK

// This define is set by selecting "Board: Heltec Wireless Stick(V3)" (SX1262) in the Arduino IDE:
//#define ARDUINO_HELTEC_WIRELESS_STICK_V3

// This define is set by selecting "Board: Heltec Vision Master T190" in the Arduino IDE:
//#define ARDUINO_HELTEC_VISION_MASTER_T190

// This define is set by selecting "Board: Heltec WiFi LoRa 32(V2)" in the Arduino IDE:
//#define ARDUINO_HELTEC_WIFI_LORA_32_V2

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

// Adafruit Feather RP2040 with RFM95W "FeatherWing" ADA3232
// https://github.com/espressif/arduino-esp32/blob/master/variants/feather_esp32/pins_arduino.h
//
// This define is set by selecting "Adafruit Feather RP2040" in the Arduino IDE:
//#define ARDUINO_ADAFRUIT_FEATHER_RP2040

// DFRobot Firebeetle32
// https://github.com/espressif/arduino-esp32/tree/master/variants/firebeetle32/pins_arduino.h
//
// This define is set by selecting "FireBeetle-ESP32" in the Arduino IDE:
//#define ARDUINO_DFROBOT_FIREBEETLE_ESP32

// M5Stack Core2
// https://github.com/espressif/arduino-esp32/blob/master/variants/m5stack_core2/pins_arduino.h
//
// This define is set by selecting "M5Core2" in the Arduino IDE
//#define ARDUINO_M5STACK_CORE2

#if defined(ARDUINO_TTGO_LoRa32_V1)
    #pragma message("ARDUINO_TTGO_LoRa32_V1 defined; using on-board transceiver")
    #define USE_SX1276
    // Use pinning for LILIGO TTGO LoRa32-OLED
    #define PIN_RECEIVER_CS   LORA_CS
    #define PIN_RECEIVER_IRQ  LORA_IRQ
    // n.c. on v1/v2?, LORA_D1 on v21
    #define PIN_RECEIVER_GPIO 33
    #define PIN_RECEIVER_RST  LORA_RST

#elif defined(ARDUINO_TTGO_LoRa32_V2)
    #pragma message("ARDUINO_TTGO_LoRa32_V2 defined; using on-board transceiver")
    #define USE_SX1276
    // Use pinning for LILIGO TTGO LoRa32-OLED
    #define PIN_RECEIVER_CS   LORA_CS
    #define PIN_RECEIVER_IRQ  LORA_IRQ
    // n.c. on v1/v2?, LORA_D1 on v21
    #define PIN_RECEIVER_GPIO 33
    #define PIN_RECEIVER_RST  LORA_RST

#elif defined(ARDUINO_TTGO_LoRa32_v21new)
    #pragma message("ARDUINO_TTGO_LoRa32_V21new defined; using on-board transceiver")
    #define USE_SX1276
    // Use pinning for LILIGO TTGO LoRa32-OLED V2.1 (1.6.1)
    #define PIN_RECEIVER_CS   LORA_CS
    #define PIN_RECEIVER_IRQ  LORA_IRQ
    #define PIN_RECEIVER_GPIO LORA_D1
    #define PIN_RECEIVER_RST  LORA_RST

#elif defined(ARDUINO_LILYGO_T3S3_SX1262)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/lilygo_t3_s3_sx1262/pins_arduino.h
    #pragma message("ARDUINO_LILYGO_T3S3_SX1262 defined; using on-board transceiver")
    #define USE_SX1262
    #define PIN_RECEIVER_CS   LORA_CS
    #define PIN_RECEIVER_IRQ  LORA_IRQ
    #define PIN_RECEIVER_GPIO LORA_BUSY
    #define PIN_RECEIVER_RST  LORA_RST

#elif defined(ARDUINO_LILYGO_T3S3_SX1276)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/lilygo_t3_s3_sx127x/pins_arduino.h
    #pragma message("ARDUINO_LILYGO_T3S3_SX1276 defined; using on-board transceiver")
    #define USE_SX1276
    #define PIN_RECEIVER_CS   LORA_CS
    #define PIN_RECEIVER_IRQ  LORA_IRQ
    #define PIN_RECEIVER_GPIO LORA_BUSY
    #define PIN_RECEIVER_RST  LORA_RST

#elif defined(ARDUINO_LILYGO_T3S3_LR1121)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/lilygo_t3_s3_lr1121/pins_arduino.h
    #pragma message("ARDUINO_LILYGO_T3S3_LR1121 defined; using on-board transceiver")
    #define USE_LR1121
    #define PIN_RECEIVER_CS   LORA_CS
    #define PIN_RECEIVER_IRQ  LORA_IRQ
    #define PIN_RECEIVER_GPIO LORA_BUSY
    #define PIN_RECEIVER_RST  LORA_RST

#elif defined(ARDUINO_HELTEC_WIRELESS_STICK)
    // Heltec Wireless Stick V2
    #pragma message("ARDUINO_HELTEC_WIRELESS_STICK defined; using on-board transceiver")
    #define USE_SX1276
    // Use pinning for Heltec Wireless Stick or WiFi LoRa32 V2, respectively
    #define PIN_RECEIVER_CS   SS
    #define PIN_RECEIVER_IRQ  DIO0
    #define PIN_RECEIVER_GPIO DIO1
    #define PIN_RECEIVER_RST  RST_LoRa

#elif defined(ARDUINO_HELTEC_WIRELESS_STICK_V3)
    #pragma message("ARDUINO_HELTEC_WIRELESS_STICK_V3 defined; using on-board transceiver")
    #define USE_SX1276
    #define PIN_RECEIVER_CS   SS
    #define PIN_RECEIVER_IRQ  DIO0
    #define PIN_RECEIVER_GPIO BUSY_LoRa
    #define PIN_RECEIVER_RST  RST_LoRa

#elif defined(ARDUINO_HELTEC_WIFI_LORA_32_V2)
    #pragma message("ARDUINO_HELTEC_WIFI_LORA_32_V2 defined; using on-board transceiver")
    #define USE_SX1276
    // Use pinning for Heltec Wireless Stick or WiFi LoRa32 V2, respectively
    #define PIN_RECEIVER_CS   SS
    #define PIN_RECEIVER_IRQ  DIO0
    #define PIN_RECEIVER_GPIO DIO1
    #define PIN_RECEIVER_RST  RST_LoRa

#elif defined(ARDUINO_HELTEC_WIFI_LORA_32_V3) || defined(ARDUINO_HELTEC_VISION_MASTER_T190)
    #pragma message("ARDUINO_HELTEC_WIFI_LORA_32_V3 / ARDUINO_HELTEC_VISION_MASTER_T190 defined; using on-board transceiver")
    #define USE_SX1262
    #define PIN_RECEIVER_CS   SS
    #define PIN_RECEIVER_IRQ  DIO0
    #define PIN_RECEIVER_GPIO BUSY_LoRa
    #define PIN_RECEIVER_RST  RST_LoRa

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_ESP32S2 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    // Use pinning for Adafruit Feather ESP32S2 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   6
    #define PIN_RECEIVER_IRQ  5
    #define PIN_RECEIVER_GPIO 11
    #define PIN_RECEIVER_RST  9

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32_V2)
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_ESP32_V2 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")
    // Use pinning for Adafruit Feather ESP32 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   14
    #define PIN_RECEIVER_IRQ  32
    #define PIN_RECEIVER_GPIO 33
    #define PIN_RECEIVER_RST  27

#elif defined(ARDUINO_FEATHER_ESP32) || defined(ARDUINO_THINGPULSE_EPULSE_FEATHER)
    #pragma message("ARDUINO_FEATHER_ESP32/ARDUINO_THINGPULSE_EPULSE_FEATHER defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")
    // Use pinning for Adafruit Feather ESP32 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   14
    #define PIN_RECEIVER_IRQ  32
    #define PIN_RECEIVER_GPIO 33
    #define PIN_RECEIVER_RST  27

#elif defined(ARDUINO_M5STACK_CORE2)
    #pragma message("ARDUINO_M5STACK_CORE2 defined; assuming M5Stack Module LoRa868 will be used")
    #define USE_SX1276
    #define PIN_RECEIVER_CS   33
    #define PIN_RECEIVER_IRQ  36
    #define PIN_RECEIVER_GPIO 35
    #define PIN_RECEIVER_RST  26

#elif defined(ARDUINO_ESP32S3_POWERFEATHER)
    #pragma message("ARDUINO_ESP32S3_POWERFEATHER defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")
    // Use pinning for ESP32-S3 PowerFeather with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   15
    #define PIN_RECEIVER_IRQ  16
    #define PIN_RECEIVER_GPIO 18
    #define PIN_RECEIVER_RST  45

#elif defined(ARDUINO_ESP32S3_DEV)
    #pragma message("ARDUINO_ESP32S3_DEV defined; this is a generic (i.e. non-specific) target")
    #define USE_SX1276
    //#define USE_SX1262
    //#define USE_CC1101
    //#define USE_LR1121
    #pragma message("Cross check if the selected GPIO pins are really available on your board.")
    #pragma message("Connect a radio module with a supported chip.")
    #pragma message("Select the chip by setting the appropriate define.")
    // Use pinning for generic ESP32 S3 board with unspecified radio module
    #define PIN_RECEIVER_CS   10
    #define PIN_RECEIVER_IRQ  21
    #define PIN_RECEIVER_GPIO 8
    #define PIN_RECEIVER_RST  9

#elif defined(ARDUINO_MAKERGO_C3_SUPERMINI)
    // Maker Go ESP32C3 Supermini
    #pragma message("ARDUINO_MAKERGO_C3_SUPERMINI defined; assuming Heltec HT-RA62 (SX1262) will be used")
    #define USE_SX1262
    // Use pinning for Maker Go ESP32C3 Supermini with Heltec HT-RA62
    #define PIN_RECEIVER_CS   7
    #define PIN_RECEIVER_IRQ  1
    #define PIN_RECEIVER_GPIO 2
    #define PIN_RECEIVER_RST  3

#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_RP2040 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")
    // Use pinning for Adafruit Feather RP2040 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   7
    #define PIN_RECEIVER_IRQ  8
    #define PIN_RECEIVER_GPIO 10
    #define PIN_RECEIVER_RST  11

#elif defined(ARDUINO_DFROBOT_FIREBEETLE_ESP32)
    //#define LORAWAN_NODE
    #define DFROBOT_COVER_LORA
    
    #if defined(DFROBOT_COVER_LORA)
        #pragma message("ARDUINO_DFROBOT_FIREBEETLE_ESP32 & DFROBOT_COVER_LORA defined; assuming this is a FireBeetle ESP32 with FireBeetle Cover LoRa")
        #define USE_SX1276
        #pragma message("Required wiring: D2 to RESET, D3 to DIO0, D4 to CS, D5 to DIO1")
        #define PIN_RECEIVER_CS   27 // D4
        #define PIN_RECEIVER_IRQ  26 // D3
        #define PIN_RECEIVER_GPIO 9  // D5
        #define PIN_RECEIVER_RST  25 // D2

    #elif defined(LORAWAN_NODE) 
        #pragma message("ARDUINO_DFROBOT_FIREBEETLE_ESP32 & LORAWAN_NODE defined; assuming this is the LoRaWAN_Node board (DFRobot Firebeetle32 + Adafruit RFM95W LoRa Radio)")
        #define USE_SX1276

        // Use pinning for LoRaWAN_Node (https://github.com/matthias-bs/LoRaWAN_Node)
        #define PIN_RECEIVER_CS   14
        #define PIN_RECEIVER_IRQ  4
        #define PIN_RECEIVER_GPIO 16
        #define PIN_RECEIVER_RST  12

    #else
        #pragma message("ARDUINO_DFROBOT_FIREBEETLE_ESP32 defined; if you use one of those boards, select either LORAWAN_NODE or FIREBEETLE_ESP32_COVER_LORA manually!")

    #endif
#elif defined(ESP32)
    #pragma message("ESP32 defined; this is a generic (i.e. non-specific) target")
    #pragma message("Cross check if the selected GPIO pins are really available on your board.")
    #pragma message("Connect a radio module with a supported chip.")
    #pragma message("Select the chip by setting the appropriate define.")
    #define USE_SX1276
    //#define USE_SX1262
    //#define USE_CC1101
    //#define USE_LR1121
    // Generic pinning for ESP32 development boards
    #define PIN_RECEIVER_CS   27
    #define PIN_RECEIVER_IRQ  21
    #define PIN_RECEIVER_GPIO 33
    #define PIN_RECEIVER_RST  32
#elif defined(ESP8266)
    #pragma message("ESP8266 defined; this is a generic (i.e. non-specific) target")
    #pragma message("Cross check if the selected GPIO pins are really available on your board.")
    #pragma message("Connect a radio module with a supported chip.")
    #pragma message("Select the chip by setting the appropriate define.")
    //#define USE_SX1276
    //#define USE_SX1262
    #define USE_CC1101
    //#define USE_LR1121

    // Generic pinning for ESP8266 development boards (e.g. LOLIN/WEMOS D1 mini)
    #define PIN_RECEIVER_CS   15
    #define PIN_RECEIVER_IRQ  4
    #define PIN_RECEIVER_GPIO 5
    #define PIN_RECEIVER_RST  2
#endif

#if defined(USE_CC1101)
#define RADIO_CHIP CC1101
#elif defined(USE_SX1276)
#define RADIO_CHIP SX1276
#elif defined(USE_SX1262)
#define RADIO_CHIP SX1262
#elif defined(USE_LR1121)
#define RADIO_CHIP LR1121
#else
#pragma message("No radio chip selected!")
#endif


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
// - RP2040:
//   DEBUG_RP2040_PORT is set in Arduino IDE:
//   Tools->Debug port: "<Disabled>|<Serial>|<Serial1>|<Serial2>"
//
//   Replacement for
//   https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h
//   on ESP8266 and RP2040:
#if defined(ESP8266) || defined(ARDUINO_ARCH_RP2040)
    #define ARDUHAL_LOG_LEVEL_NONE      0
    #define ARDUHAL_LOG_LEVEL_ERROR     1
    #define ARDUHAL_LOG_LEVEL_WARN      2
    #define ARDUHAL_LOG_LEVEL_INFO      3
    #define ARDUHAL_LOG_LEVEL_DEBUG     4
    #define ARDUHAL_LOG_LEVEL_VERBOSE   5

    #if defined(ARDUINO_ARCH_RP2040) && defined(DEBUG_RP2040_PORT)
        #define DEBUG_PORT DEBUG_RP2040_PORT
    #elif defined(DEBUG_ESP_PORT)
        #define DEBUG_PORT DEBUG_ESP_PORT
    #endif
    
    // Set desired level here if not defined elsewhere!
    #if !defined(CORE_DEBUG_LEVEL)
        #define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
    #endif

    #if defined(DEBUG_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_NONE
        #define log_e(...) { DEBUG_PORT.printf("%s(), l.%d: ",__func__, __LINE__); DEBUG_PORT.printf(__VA_ARGS__); DEBUG_PORT.println(); }
     #else
        #define log_e(...) {}
     #endif
    #if defined(DEBUG_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_ERROR
        #define log_w(...) { DEBUG_PORT.printf("%s(), l.%d: ", __func__, __LINE__); DEBUG_PORT.printf(__VA_ARGS__); DEBUG_PORT.println(); }
     #else
        #define log_w(...) {}
     #endif
    #if defined(DEBUG_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_WARN
        #define log_i(...) { DEBUG_PORT.printf("%s(), l.%d: ", __func__, __LINE__); DEBUG_PORT.printf(__VA_ARGS__); DEBUG_PORT.println(); }
     #else
        #define log_i(...) {}
     #endif
    #if defined(DEBUG_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_INFO
        #define log_d(...) { DEBUG_PORT.printf("%s(), l.%d: ", __func__, __LINE__); DEBUG_PORT.printf(__VA_ARGS__); DEBUG_PORT.println(); }
     #else
        #define log_d(...) {}
     #endif
    #if defined(DEBUG_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_DEBUG
        #define log_v(...) { DEBUG_PORT.printf("%s(), l.%d: ", __func__, __LINE__); DEBUG_PORT.printf(__VA_ARGS__); DEBUG_PORT.println(); }
     #else
        #define log_v(...) {}
     #endif

#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define RECEIVER_CHIP "[" STR(RADIO_CHIP) "]"
#pragma message("Receiver chip: " RECEIVER_CHIP)
#pragma message("Pin config: RST->" STR(PIN_RECEIVER_RST) ", CS->" STR(PIN_RECEIVER_CS) ", GD0/G0/IRQ->" STR(PIN_RECEIVER_IRQ) ", GDO2/G1/GPIO->" STR(PIN_RECEIVER_GPIO) )

#endif
