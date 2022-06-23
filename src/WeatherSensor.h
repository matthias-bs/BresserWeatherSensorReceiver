///////////////////////////////////////////////////////////////////////////////////////////////////
// WeatherSensor.h
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
// 20220526 Implemented getData(), changed debug output to macros,
//          changed radio transceiver instance to member variable of WeatherSensor
//          (with initialization of 'Module' from WeatherSensor's constructor parameters)
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WeatherSensor_h
#define WeatherSensor_h

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include <RadioLib.h>

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

#define DEBUG_PORT Serial
#if defined(_DEBUG_MODE_)
    #define DEBUG_PRINT(...) { DEBUG_PORT.print(__VA_ARGS__); }
    #define DEBUG_PRINTLN(...) { DEBUG_PORT.println(__VA_ARGS__); }
#else
  #define DEBUG_PRINT(...) {}
  #define DEBUG_PRINTLN(...) {}
#endif


typedef enum DecodeStatus {
    DECODE_OK, DECODE_PAR_ERR, DECODE_CHK_ERR, DECODE_DIG_ERR
} DecodeStatus;


/*!
  \class WeatherSensor

  \brief Receive, decode and store Bresser Weather Sensor Data 
  Uses CC1101 or SX1276 radio module for receiving FSK modulated signal at 868 MHz.
*/
class WeatherSensor {
    public:
        /*!
        \brief Constructor.

        */
        WeatherSensor()
        {
        };
        
        
        /*!
        \brief Presence check and initialization of radio module.

        \returns RADIOLIB_ERR_NONE on success (otherwise does never return).
        */
        int16_t begin(void);
        
        
        /*!
        \brief Wait for reception of data or occurrance of timeout.
        With BRESSER_6_IN_1, data is distributed across two different messages. Reception of entire
        data is tried if 'complete' is set.

        \param timeout timeout in ms.

        \param complete false: Returns after successful reception of first message receiving. (default)
                        true:  Returns after reception of complete data set.

        \param func     Callback function for each loop iteration. (default: NULL)
                        
        \returns false: Timeout occurred.
                 true:  Reception (according to parammeter 'complete') succesful. 
        */
        bool    getData(uint32_t timeout, bool complete = false, void (*func)() = NULL);
        
        
        /*!
        \brief Tries to receive radio message (non-blocking) and to decode it.
        Timeout occurs after a multitude of expected time-on-air.

        \returns false: Decoding failed or receive timeout occurred.
                 true:  Reception and decoding succesful. 
        */        
        bool    getMessage(void);
        
        
        /*!
        \brief Tries to receive radio message (non-blocking) and to decode it.
        Timeout occurs after a multitude of expected time-on-air.

        \returns Always true (for compatibility with getMessage())
        */        
        bool    genMessage(void);
        
        
        /*!
        \brief Converts wind speed from Meters per Second to Beaufort.

        \param ms Wind speed in m/s.
        
        \returns Wind speed in bft.
        */        
        uint8_t windspeed_ms_to_bft(float ms);
        
        uint8_t  s_type;               // !> sensor type (only 6-in1)
        uint32_t sensor_id;            // !> sensor ID (5-in-1: 1 byte / 6-in-1: 4 bytes)
        uint8_t  chan;                 // !> channel (only 6-in-1)
        bool     temp_ok = false;      // !> temperature o.k. (only 6-in-1)
        float    temp_c;               // !> temperature in degC
        int      humidity;             // !> humidity in %
        bool     uv_ok = false;        // !> uv radiation o.k. (only 6-in-1)
        float    uv;                   // !> uv radiation (only 6-in-1)
        bool     wind_ok = false;      // !> wind speed/direction o.k. (only 6-in-1)
        float    wind_direction_deg;   // !> wind direction in deg
        float    wind_gust_meter_sec;  // !> wind speed (gusts) in m/s
        float    wind_avg_meter_sec;   // !> wind speed (avg)   in m/s
        // For LoRa_Serialization:
        //   fixed point integer with 1 decimal -
        //   saves two bytes compared to "RawFloat"
        uint16_t wind_direction_deg_fp1;  // !> wind direction in deg (fixed point int w. 1 decimal)
        uint16_t wind_gust_meter_sec_fp1; // !> wind speed (gusts) in m/s (fixed point int w. 1 decimal)
        uint16_t wind_avg_meter_sec_fp1;  // !> wind speed (avg)   in m/s (fixed point int w. 1 decimal)
        bool     rain_ok = false;      // !> rain gauge level o.k.
        float    rain_mm;              // !> rain gauge level in mm
        bool     battery_ok = false;   // !> battery o.k.
        bool     moisture_ok = false;  // !> moisture o.k. (only 6-in-1)
        int      moisture;             // !> moisture in % (only 6-in-1)
        bool     message_ok;           // !> status of last getMessage() call (set to 'true' by genMessage() call)
        bool     data_ok;              // !> status of last getData() call    (set to 'true' by genMessage() call)
        float    rssi;                 // !> received signal strength indicator in dBm

        
    private:

        #ifdef BRESSER_5_IN_1
            /*!
            \brief Decode BRESSER_5_IN_1 message.

            \param msg     Message buffer.

            \param msgSize Message size in bytes.

            \returns Decode status. 
            */
            DecodeStatus decodeBresser5In1Payload(uint8_t *msg, uint8_t msgSize);
        #endif
        #ifdef BRESSER_6_IN_1
            /*!
            \brief Decode BRESSER_6_IN_1 message.
            With BRESSER_6_IN_1, data is distributed across two different messages. Additionally,
            the message format supports different kinds of sensors.

            \param msg     Message buffer.

            \param msgSize Message size in bytes.

            \returns Decode status. 
            */
            DecodeStatus decodeBresser6In1Payload(uint8_t *msg, uint8_t msgSize);
        #endif
            
    protected: 
        /*!
        \brief Linear Feedback Shift Register - Digest16 (Data integrity check).
        */
        uint16_t lfsr_digest16(uint8_t const message[], unsigned bytes, uint16_t gen, uint16_t key);
        
        /*!
        \brief Calculate sum of all message bytes.
        
        \param message   Message buffer.
        \param num_bytes Number of bytes.
        
        \returns Sum of all message bytes.
        */
        int add_bytes(uint8_t const message[], unsigned num_bytes);
        
        #ifdef _DEBUG_MODE_
            /*!
            \brief Print raw message payload as hex byte values.
            */   
            void printRawdata(uint8_t *msg, uint8_t msgSize) {
                DEBUG_PRINT(F("Raw Data:"));
                for (uint8_t p = 0 ; p < msgSize ; p++) {
                    if (msg[p] < 16) {
                        DEBUG_PRINT("0");
                    }
                    DEBUG_PRINT(msg[p], HEX);
                    DEBUG_PRINT(" ");
                }
                DEBUG_PRINTLN();
            };
        #endif

};

#endif
