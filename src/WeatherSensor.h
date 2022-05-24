///////////////////////////////////////////////////////////////////////////////////////////////////
// Bresser_Weather_Sensor_Receiver.ino
//
// Bresser 5-in-1/6-in-1 868 MHz Weather Sensor Radio Receiver 
// based on CC1101 or SX1276/RFM95W and ESP32/ESP8266
//
// https://github.com/matthias-bs/Bresser_Weather_Sensor_Receiver
//
// Based on:
// ---------
// Bresser5in1-CC1101 by Sean Siford (https://github.com/seaniefs/Bresser5in1-CC1101)
// RadioLib by Jan Gromeš (https://github.com/jgromes/RadioLib)
// rtl433 by Benjamin Larsson (https://github.com/merbanan/rtl_433) 
//     - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_5in1.c
//     - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_6in1.c
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
// 20220523 Created from https://github.com/matthias-bs/Bresser5in1-CC1101
// 20220524 Moved code to class WeatherSensor
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////


// Select appropriate sensor message format
//#define BRESSER_5_IN_1
#define BRESSER_6_IN_1

// Select type of receiver module
//#define USE_CC1101
#define USE_SX1276

//#define _DEBUG_MODE_
//#define RADIOLIB_DEBUG
#include <Arduino.h>
#include <RadioLib.h>
//#define RADIOLIB_BUILD_ARDUINO


// Board   SCK   MOSI  MISO
// ESP8266 D5    D7    D6
// ESP32   D18   D23   D19

#define PIN_RECEIVER_CS     27
#define PIN_RECEIVER_GDO0   21 // only CC1101
#define PIN_RECEIVER_GDO2   33 // only CC1101
#define PIN_RECEIVER_DIO0   21 // only RFM95W/SX127x
#define PIN_RECEIVER_DIO1   33 // only RFM95W/SX127x
#define PIN_RECEIVER_RESET  32 // only RFM95W/SX127x

#if (defined(BRESSER_5_IN_1) and defined(BRESSER_6_IN_1))
    #error "Either BRESSER_5_IN_1 OR BRESSER_6_IN_1 must be defined!"
#endif
#if (not(defined(BRESSER_5_IN_1)) and not(defined(BRESSER_6_IN_1)))
    #error "Either BRESSER_5_IN_1 or BRESSER_6_IN_1 must be defined!"
#endif

#if (defined(USE_CC1101) and defined(USE_SX1276))
    #error "Either USE_CC1101 OR USE_SX1276 must be defined!"
#endif

#if defined(USE_CC1101)
    #define RECEIVER_CHIP F("[CC1101]")
#elif defined(USE_SX1276)
    #define RECEIVER_CHIP F("[SX1276]")
#else
    #error "Either USE_CC1101 or USE_SX1276 must be defined!"
#endif


typedef enum DecodeStatus {
    DECODE_OK, DECODE_PAR_ERR, DECODE_CHK_ERR, DECODE_DIG_ERR
} DecodeStatus;


class WeatherSensor {
    public:
        WeatherSensor();
        
        int16_t begin(void);
        uint8_t getData(unsigned timeout, bool complete = false); 
        bool    getMessage(void);
        
        uint8_t  s_type;               // only 6-in1
        uint32_t sensor_id;            // 5-in-1: 1 byte / 6-in-1: 4 bytes
        uint8_t  chan;                 // only 6-in-1
        bool     temp_ok = false;      // only 6-in-1
        float    temp_c;
        int      humidity;
        bool     uv_ok = false;        // only 6-in-1
        float    uv;                   // only 6-in-1
        bool     wind_ok = false;      // only 6-in-1
        float    wind_direction_deg;
        float    wind_gust_meter_sec;
        float    wind_avg_meter_sec;
        bool     rain_ok = false;      // only 6-in-1
        float    rain_mm;
        bool     battery_ok = false;
        bool     moisture_ok = false;  // only 6-in-1
        int      moisture;             // only 6-in-1

        
    private:
        #ifdef BRESSER_5_IN_1
            DecodeStatus decodeBresser5In1Payload(uint8_t *msg, uint8_t msgSize);
        #endif
        #ifdef BRESSER_6_IN_1
            DecodeStatus decodeBresser6In1Payload(uint8_t *msg, uint8_t msgSize);
        #endif
            
    protected:
        uint16_t lfsr_digest16(uint8_t const message[], unsigned bytes, uint16_t gen, uint16_t key);
        int add_bytes(uint8_t const message[], unsigned num_bytes);
        
        #ifdef _DEBUG_MODE_
            void printRawdata(uint8_t *msg, uint8_t msgSize) {
                Serial.println("Raw Data:");
                for (uint8_t p = 0 ; p < msgSize ; p++) {
                    Serial.printf("%02X ", msg[p]);
                }
                Serial.printf("\n");
            };
        #endif

};
