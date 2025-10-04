///////////////////////////////////////////////////////////////////////////////////////////////////
// config.h
//
// Key configuration for BresserWeatherSenorCanvasGauges.ino
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
// 20251004 Created
//
///////////////////////////////////////////////////////////////////////////////////////////////////


// Raingauge reset button definition - unfortunately a common definition does not exist!
#if defined(ARDUINO_ARCH_ESP32)
#if defined(ARDUINO_LILYGO_T3S3_SX1262) || \
    defined(ARDUINO_LILYGO_T3S3_SX1276) || \
    defined(ARDUINO_LILYGO_T3S3_LR1121)
const uint8_t KEY_RAINGAUGE_RESET = (BUTTON_1);
#elif defined(ARDUINO_DFROBOT_FIREBEETLE_ESP32)
const uint8_t KEY_RAINGAUGE_RESET = 0;
#elif defined(ARDUINO_HELTEC_WIFI_LORA_32_V3) || \
      defined(ARDUINO_HELTEC_VISION_MASTER_T190) || \
      defined(ARDUINO_HELTEC_WIRELESS_STICK_V3)
// Check if this GPIO pin is available/connected to a key on your board
const uint8_t KEY_RAINGAUGE_RESET = 0;
#elif defined(ARDUINO_FEATHER_ESP32) || \
      defined(ARDUINO_THINGPULSE_EPULSE_FEATHER) || \
      defined(ARDUINO_ADAFRUIT_FEATHER_ESP32_V2) || \
      defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
// Check if this GPIO pin is available/connected to a key on your board
const uint8_t KEY_RAINGAUGE_RESET = 4;
#elif defined(ARDUINO_ESP32S3_POWERFEATHER)
const uint8_t KEY_RAINGAUGE_RESET = BTN;
#elif defined(ARDUINO_HELTEC_WIFI_LORA_32_V2) || \
      defined(ARDUINO_HELTEC_WIRELESS_STICK) || \
      defined(ARDUINO_TTGO_LoRa32_V1) || \
      defined(ARDUINO_TTGO_LoRa32_V2) || \
      defined(ARDUINO_TTGO_LoRa32_v21new)
const uint8_t KEY_RAINGAUGE_RESET = KEY_BUILTIN;
#else
// Check if this GPIO pin is available/connected to a key on your board
const uint8_t KEY_RAINGAUGE_RESET = 4;
#endif
#else
// Check if this GPIO pin is available/connected to a key on your board
const uint8_t KEY_RAINGAUGE_RESET = 5;
#endif
