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

// Heltec Wireless Stick
// V2 -> SX1276
// V3 -> SX1262
// THERE IS NO WAY TO DISTINGUISH THE VERSION AUTOMATICALLY!
//
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

// Adafruit Feather RP2040 with RFM95W "FeatherWing" ADA3232
// https://github.com/espressif/arduino-esp32/blob/master/variants/feather_esp32/pins_arduino.h
//
// This define is set by selecting "Adafruit Feather RP2040" in the Arduino IDE:
//#define ARDUINO_ADAFRUIT_FEATHER_RP2040

// DFRobot Firebeetle32
// https://github.com/espressif/arduino-esp32/tree/master/variants/firebeetle32/pins_arduino.h
//
// This define (not very specific...) is set by selecting "FireBeetle-ESP32" in the Arduino IDE:
//#define ARDUINO_ESP32_DEV

// M5Stack Core2
// https://github.com/espressif/arduino-esp32/blob/master/variants/m5stack_core2/pins_arduino.h
//
// This define is set by selecting "M5Core2" in the Arduino IDE
//#define ARDUINO_M5STACK_CORE2

#if defined(ARDUINO_TTGO_LoRa32_V1)
    #pragma message("ARDUINO_TTGO_LoRa32_V1 defined; using on-board transceiver")
    #define USE_SX1276

#elif defined(ARDUINO_TTGO_LoRa32_V2)
    #pragma message("ARDUINO_TTGO_LoRa32_V2 defined; using on-board transceiver")
    #pragma message("Wiring required for LMIC: DIO1 to GPIO33")
    #define USE_SX1276

#elif defined(ARDUINO_TTGO_LoRa32_v21new)
    #pragma message("ARDUINO_TTGO_LoRa32_V21new defined; using on-board transceiver")
    #define USE_SX1276

#elif defined(ARDUINO_heltec_wireless_stick)
    #pragma message("ARDUINO_heltec_wireless_stick defined; using on-board transceiver")
    #pragma message("Radio transceiver chip has to be configured manually: V2 -> USE_SX1276 / V3 -> USE_SX1262")
    //#define USE_SX1276 // Heltec Wireless Stick V2
    #define USE_SX1262 // Heltec Wireless Stick V3

#elif defined(ARDUINO_heltec_wireless_stick_v2)
    #pragma message("ARDUINO_heltec_wireless_stick_v2 defined; using on-board transceiver")
    #define USE_SX1276

#elif defined(ARDUINO_heltec_wireless_stick_v3)
    #pragma message("ARDUINO_heltec_wireless_stick_v3 defined; using on-board transceiver")
    #define USE_SX1262

#elif defined(ARDUINO_heltec_wifi_lora_32_V2)
    #pragma message("ARDUINO_heltec_wifi_lora_32_V2 defined; using on-board transceiver")
    #define USE_SX1276

#elif defined(ARDUINO_heltec_wifi_32_lora_V3) || defined(ARDUINO_heltec_wifi_lora_32_V3)
    #pragma message("ARDUINO_heltec_wifi_32_lora_V3 (or similar) defined; using on-board transceiver")
    #define USE_SX1262

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_ESP32S2 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32_V2)
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_ESP32_V2 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")

#elif defined(ARDUINO_FEATHER_ESP32)
    #pragma message("ARDUINO_FEATHER_ESP32 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")

#elif defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_Core2)
    // Note: Depending on board package file date, either variant is used - 
    //       see https://github.com/espressif/arduino-esp32/issues/9423!
    #pragma message("ARDUINO_M5STACK_CORE2 defined; assuming M5Stack Module LoRa868 will be used")
    #define USE_SX1276
    #pragma message("Wiring required for LMIC: DIO1 to GPIO35")

#elif defined(ARDUINO_ESP32S3_POWERFEATHER)
    #pragma message("ARDUINO_ESP32S3_POWERFEATHER defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")

#elif defined(ARDUINO_AVR_FEATHER32U4)
    #pragma message("ARDUINO_AVR_FEATHER32U4 defined; assuming this is the Adafruit Feather 32u4 RFM95 LoRa Radio")
    #define USE_SX1276

#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_RP2040 defined; assuming RFM95W FeatherWing will be used")
    #define USE_SX1276
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")

#elif defined(ARDUINO_ESP32_DEV) || defined(ARDUINO_DFROBOT_FIREBEETLE_ESP32)
    //#define LORAWAN_NODE
    #define FIREBEETLE_ESP32_COVER_LORA

    #if defined(FIREBEETLE_ESP32_COVER_LORA)
        #pragma message("FIREBEETLE_ESP32_COVER_LORA defined; assuming this is a FireBeetle ESP32 with FireBeetle Cover LoRa")
        #define USE_SX1276
        #pragma message("Required wiring: D2 to RESET, D3 to DIO0, D4 to CS, D5 to DIO1")

    #elif defined(LORAWAN_NODE) 
        #pragma message("LORAWAN_NODE defined; assuming this is the LoRaWAN_Node board (DFRobot Firebeetle32 + Adafruit RFM95W LoRa Radio)")
        #define USE_SX1276

    #else
        #pragma message("ARDUINO_ESP32_DEV defined; if you use one of those boards, select either LORAWAN_NODE or FIREBEETLE_ESP32_COVER_LORA manually!")

    #endif
#endif


// ------------------------------------------------------------------------------------------------
// --- Radio Transceiver ---
// ------------------------------------------------------------------------------------------------
// Select type of receiver module (if not yet defined based on the assumptions above)
#if ( !defined(USE_CC1101) && !defined(USE_SX1276) && !defined(USE_SX1262) )
    //#define USE_CC1101
    #define USE_SX1276
    //#define USE_SX1262
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
    
    // Set desired level here!
    #define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO

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

#if ( (defined(USE_CC1101) && defined(USE_SX1276)) || \
      (defined(USE_SX1276) && defined(USE_SX1262)) || \
      (defined(USE_SX1262) && defined(USE_CC1101)) )
    #error "Either USE_CC1101 OR USE_SX1276 OR USE_SX1262 must be defined!"
#endif

#if defined(USE_CC1101)
    #define RECEIVER_CHIP "[CC1101]"
#elif defined(USE_SX1276)
    #define RECEIVER_CHIP "[SX1276]"
#elif defined(USE_SX1262)
    #define RECEIVER_CHIP "[SX1262]"
#else
    #error "Either USE_CC1101, USE_SX1276 or USE_SX1262 must be defined!"
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

#elif defined(ARDUINO_heltec_wifi_32_lora_V3) || defined(ARDUINO_heltec_wifi_lora_32_V3)
    // Use pinning for Heltec WiFi LoRa32 V3
    #define PIN_RECEIVER_CS   SS

    // CC1101: GDO0 / RFM95W/SX127x: G0 / SX1262: DIO0
    #define PIN_RECEIVER_IRQ  DIO0

    // CC1101: GDO2 / RFM95W/SX127x: G1 / SX1262: BUSY
    #define PIN_RECEIVER_GPIO BUSY_LoRa

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

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32_V2)
    // Use pinning for Adafruit Feather ESP32 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   14

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  32

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 33

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  27

#elif defined(ARDUINO_FEATHER_ESP32)
    // Use pinning for Adafruit Feather ESP32 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   14

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  32

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 33

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  27

#elif defined(ARDUINO_M5STACK_CORE2) || defined(ARDUINO_M5STACK_Core2)
    // Note: Depending on board package file date, either variant is used - 
    //       see https://github.com/espressif/arduino-esp32/issues/9423!
    // Use pinning for M5Stack Core2 with M5Stack Module LoRa868
    #define PIN_RECEIVER_CS   33

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  36

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 35

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  26

#elif defined(ARDUINO_ESP32S3_POWERFEATHER)
    // Use pinning for ESP32-S3 PowerFeather with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   15

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  16

    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 18

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  45

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

#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
    // Use pinning for Adafruit Feather RP2040 with RFM95W "FeatherWing" ADA3232
    #define PIN_RECEIVER_CS   7

    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  8

    // CC1101: GDO2 / RFM95W/SX127x: G1 (not used)
    #define PIN_RECEIVER_GPIO 10

    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  11

#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message("Receiver chip: " RECEIVER_CHIP)
#pragma message("Pin config: RST->" STR(PIN_RECEIVER_RST) ", CS->" STR(PIN_RECEIVER_CS) ", GD0/G0/IRQ->" STR(PIN_RECEIVER_IRQ) ", GDO2/G1/GPIO->" STR(PIN_RECEIVER_GPIO) )

#endif
