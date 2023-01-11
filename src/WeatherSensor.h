///////////////////////////////////////////////////////////////////////////////////////////////////
// WeatherSensor.h
//
// Bresser 5-in-1/6-in-1 868 MHz Weather Sensor Radio Receiver 
// based on CC1101 or SX1276/RFM95W and ESP32/ESP8266
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
// NOTE: Application/hardware specific configurations should be made in WeatherSensorCfg.h!
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
// 20220526 Implemented getData(), changed debug output to macros
// 20220815 Added support of multiple sensors
//          Moved windspeed_ms_to_bft() to WeatherUtils.h/.cpp
// 20221207 Added SENSOR_TYPE_THERMO_HYGRO
// 20220110 Added WEATHER0_RAIN_OV/WEATHER1_RAIN_OV
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WeatherSensor_h
#define WeatherSensor_h

#include <Arduino.h>
#include <string>
#include <RadioLib.h>


// Sensor Types
// 0 - Weather Station          5-in-1; PN 7002510..12/7902510..12
// 1 - Weather Station          6-in-1; PN 7002585
// 2 - Thermo-/Hygro-Sensor     6-in-1; PN 7009999
// 4 - Soil Moisture Sensor     6-in-1; PN 7009972
// 9 - Professional Rain Gauge  (5-in-1 decoder)
// ? - Air Quality Sensor
// ? - Water Leakage Sensor
// ? - Pool Thermometer
// ? - Lightning Sensor
#define SENSOR_TYPE_WEATHER0        0 // Weather Station 
#define SENSOR_TYPE_WEATHER1        1 // Weather Station
#define SENSOR_TYPE_THERMO_HYGRO    2 // Thermo-/Hygro-Sensor
#define SENSOR_TYPE_SOIL            4 // Soil Temperature and Moisture (from 6-in-1 decoder)
#define SENSOR_TYPE_RAIN            9 // Professional Rain Gauge (from 5-in-1 decoder)


// Sensor specific rain gauge overflow threshold (mm)
#define WEATHER0_RAIN_OV          1000
#define WEATHER1_RAIN_OV        100000


// Flags for controlling completion of reception in getData()
#define DATA_COMPLETE           0x1     // only completed slots (as opposed to partially filled) 
#define DATA_TYPE               0x2     // at least one slot with specific sensor type
#define DATA_ALL_SLOTS          0x8     // all slots completed


// Radio message decoding status
typedef enum DecodeStatus {
    DECODE_INVALID, DECODE_OK, DECODE_PAR_ERR, DECODE_CHK_ERR, DECODE_DIG_ERR, DECODE_SKIP, DECODE_FULL
} DecodeStatus;


/*!
 * \struct SensorMap
 * 
 * \brief Mapping of sensor IDs to names
 */
typedef struct SensorMap {
    uint32_t        id;    //!< ID if sensor (as transmitted in radio message)
    std::string     name;  //!< Name of sensor (e.g. for MQTT topic)
} SensorMap;


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

        \param flags    DATA_COMPLETE / DATA_TYPE / DATA_ALL_SLOTS
        
        \param type     sensor type (combined with FLAGS==DATA_TYPE)

        \param func     Callback function for each loop iteration. (default: NULL)
                        
        \returns false: Timeout occurred.
                 true:  Reception (according to parammeter 'complete') succesful. 
        */
        bool    getData(uint32_t timeout, uint8_t flags = 0, uint8_t type = 0, void (*func)() = NULL);
        
        
        /*!
        \brief Tries to receive radio message (non-blocking) and to decode it.
        Timeout occurs after a multitude of expected time-on-air.

        \returns DecodeStatus 
        */        
        DecodeStatus    getMessage(void);
        
        /**
         * \struct Sensor 
         * 
         * \brief sensor data and status flags
         */
        struct Sensor {
            uint32_t sensor_id;            //!< sensor ID (5-in-1: 1 byte / 6-in-1: 4 bytes)
            uint8_t  s_type;               //!< sensor type (only 6-in1)
            uint8_t  chan;                 //!< channel (only 6-in-1)
            bool     valid;                //!< data valid (but not necessarily complete)
            bool     complete;             //!< data is split into two separate messages is complete (only 6-in-1 WS)
            bool     temp_ok = false;      //!< temperature o.k. (only 6-in-1)
            bool     humidity_ok = false;  //!< humidity o.k.
            bool     uv_ok = false;        //!< uv radiation o.k. (only 6-in-1)
            bool     wind_ok = false;      //!< wind speed/direction o.k. (only 6-in-1)
            bool     rain_ok = false;      //!< rain gauge level o.k.
            bool     battery_ok = false;   //!< battery o.k.
            bool     moisture_ok = false;  //!< moisture o.k. (only 6-in-1)
            float    temp_c;               //!< temperature in degC
            float    uv;                   //!< uv radiation (only 6-in-1)
            float    rain_mm;              //!< rain gauge level in mm
            #ifdef WIND_DATA_FLOATINGPOINT
            float    wind_direction_deg;   //!< wind direction in deg
            float    wind_gust_meter_sec;  //!< wind speed (gusts) in m/s
            float    wind_avg_meter_sec;   //!< wind speed (avg)   in m/s
            #endif
            #ifdef WIND_DATA_FIXEDPOINT
            // For LoRa_Serialization:
            //   fixed point integer with 1 decimal -
            //   saves two bytes compared to "RawFloat"
            uint16_t wind_direction_deg_fp1;  //!< wind direction in deg (fixed point int w. 1 decimal)
            uint16_t wind_gust_meter_sec_fp1; //!< wind speed (gusts) in m/s (fixed point int w. 1 decimal)
            uint16_t wind_avg_meter_sec_fp1;  //!< wind speed (avg)   in m/s (fixed point int w. 1 decimal)
            #endif
            uint8_t  humidity;             //!< humidity in %
            uint8_t  moisture;             //!< moisture in % (only 6-in-1)
            float    rssi;                 //!< received signal strength indicator in dBm
        };
        
        typedef struct Sensor sensor_t;    //!< Shortcut for struct Sensor
        sensor_t sensor[NUM_SENSORS];      //!< sensor data array
        float    rssi;                     //!< received signal strength indicator in dBm
        
        
        /*!
        \brief Generates data otherwise received and decoded from a radio message.

        \returns Always true (for compatibility with getMessage())
        */        
        bool    genMessage(int i, uint32_t id = 0xff, uint8_t type = 1, uint8_t channel = 0);

        
         /*!
        \brief Clear sensor data
        
        If 'type' is not specified, all slots are cleared. If 'type' is specified,
        only slots containing data of the given sensor type are cleared.
        
        \param type Sensor type
        */
        void clearSlots(uint8_t type = 0xFF)
        {
            for (int i=0; i< NUM_SENSORS; i++) {
                if ((type == 0xFF) || (sensor[i].s_type == type)) {
                    sensor[i].valid       = false;
                    sensor[i].complete    = false;
                    sensor[i].temp_ok     = false;
                    sensor[i].humidity_ok = false;
                    sensor[i].uv_ok       = false;
                    sensor[i].wind_ok     = false;
                    sensor[i].rain_ok     = false;
                    sensor[i].moisture_ok = false;
                }
            }
        };

        /*!
         * Find slot of required data set by ID
         * 
         * \param id    sensor ID
         * 
         * \returns     slot (or -1 if not found)
         */
        int findId(uint32_t id);
        
        
        /*!
         * Find slot of required data set by type and (optionally) channel
         * 
         * \param type      sensor type
         * \param channel   sensor channel (0xFF: don't care)
         * 
         * \returns         slot (or -1 if not found)
         */        
        int findType(uint8_t type, uint8_t channel = 0xFF);
        
    private:
        struct Sensor *pData; //!< pointer to slot in sensor data array
        
        /*!
         * \brief Find slot in sensor data array
         * 
         * 1. The current sensor ID is checked against the exclude-list (sensor_ids_exc).
         *    If there is a match, the current message is skipped.
         * 
         * 2. If the include-list (sensor_ids_inc) is not empty, the current ID is checked
         *    against it. If there is NO match, the current message is skipped.
         * 
         * 3. Either an existing slot with the same ID as the current message is updated
         *    or a free slot (if any) is selected.
         * 
         * \param id Sensor ID from current message
         * 
         * \returns Pointer to slot in sensor data array or NULL if ID is not wanted or 
         *          no free slot is available.
         */
        int findSlot(uint32_t id, DecodeStatus * status);
        
        
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
                DEBUG_PRINT(F("Raw Data: "));
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
