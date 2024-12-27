///////////////////////////////////////////////////////////////////////////////////////////////////
// WeatherSensor.cpp
//
// Bresser 5-in-1/6-in-1/7-in1 868 MHz Weather Sensor Radio Receiver
// based on CC1101 or SX1276/RFM95W, SX1262 or LR1121 and ESP32/ESP8266
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
// Based on:
// ---------
// Bresser5in1-CC1101 by Sean Siford (https://github.com/seaniefs/Bresser5in1-CC1101)
// RadioLib by Jan Gromeš (https://github.com/jgromes/RadioLib)
// rtl433 by Benjamin Larsson (https://github.com/merbanan/rtl_433)
//     - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_5in1.c
//     - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_6in1.c
//     - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_7in1.c
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
// 20220731 Updated decodeBresser5In1Payload()/decodeBresser6In1Payload() from rtl_433 project
// 20220815 Added support of multiple sensors
//          Moved windspeed_ms_to_bft() to WeatherUtils.h/.cpp
// 20220905 Improved code quality and added Doxygen comments
// 20221003 Fixed humidity decoding in decodeBresser5In1Payload()
// 20221024 Modified WeatherSensorCfg.h/WeatherSensor.h handling
// 20221227 Replaced DEBUG_PRINT/DEBUG_PRINTLN by Arduino logging functions
// 20230111 Added additional digit for rain gauge in 5in1-decoder (maximum is now 999.9mm)
// 20230114 Modified decodeBresser6In1Payload() to distinguish msg type based on 'flags' (msg[16])
// 20230228 Added Bresser 7 in 1 decoder by Jorge Navarro-Ortiz (jorgenavarro@ugr.es)
// 20230329 Fixed issue introduced with 7 in 1 decoder
// 20230412 Added workaround for Professional Wind Gauge / Anemometer, P/N 7002531
// 20230412 Fixed 7 in 1 decoder (valid/complete flags were not set)
// 20230624 Added Bresser Lightning Sensor decoder
// 20230613 Fixed rain value in 7 in 1 decoder
// 20230708 Added startup flag in 6-in-1 and 7-in-1 decoder; added sensor type in 7-in-1 decoder
// 20230710 Added verbose log message with de-whitened data (7-in-1 and lightning decoder)
// 20230716 Added decodeMessage() to separate decoding function from receiving function
// 20230804 Added Bresser Water Leakage Sensor (P/N 7009975) decoder
// 20230814 Fixed receiver state handling in getMessage()
// 20231006 Added crc16() from https://github.com/merbanan/rtl_433/blob/master/src/util.c
//          Added CRC check in decodeBresserLeakagePayload()
// 20231024 Added Pool / Spa Thermometer (P/N 7009973) to 6-in-1 decoder
// 20231026 Added Air Quality Sensor (Particulate Matter, P/N 7009970) to 7-in-1 decoder
//          Modified decoding of sensor type (to raw, non de-whitened data)
// 20231028 Fixed startup flag polarity in 7-in-1, lightning and leakage decoder
// 20231030 Refactored sensor data using a union to save memory
// 20231101 Added radio transceiver SX1262
// 20231112 Added setting of rain_ok in genMessage()
// 20231130 Bresser 3-in-1 Professional Wind Gauge / Anemometer, PN 7002531: Replaced workaround
//          for negative temperatures by fix (6-in-1 decoder)
// 20231202 Changed reception to interrupt mode to fix issues with CC1101 and SX1262
// 20231218 Fixed inadvertent end of reception due to transceiver sleep mode
// 20231227 Added sleep()
// 20240116 Fixed counter width and assignment to unknown1 in decodeBresserLightningPayload()
// 20240117 Fixed counter decoding (changed from binary to BCD) in decodeBresserLightningPayload()
// 20240208 Added sensors for CO2, P/N 7009977 and HCHO/VOC, P/N 7009978 to 7-in-1 decoder
//          see https://github.com/merbanan/rtl_433/pull/2815
//            & https://github.com/merbanan/rtl_433/pull/2817
// 20240213 Added PM1.0 to air quality (PM) sensor decoder
// 20240322 Added pin definitions for M5Stack Core2 with M5Stack Module LoRa868
// 20240409 Added radioReset()
// 20240416 Added enabling of 3.3V power supply for FeatherWing on ESP32-S3 PowerFeather
// 20240417 Added sensor configuration at run time
// 20240423 Implemented setting of sensor_ids_inc/sensor_ids_exc to empty if first value in
//          Preferences is 0x00000000
// 20240506 Changed sensor from array to std::vector, added getSensorsCfg() / setSensorsCfg()
// 20240507 Added configuration of enabled decoders at run time
// 20240508 Fixed configuration of enabled decoders at run time
// 20240513 Refactoring: Split into 3 files
//          - Reception/utility part (this file)
//          - Sensor data decoding functions (WeatherSensorDecoders.cpp)
//          - Run-time configuration functions (WeatherSensorConfig.cpp)
// 20240528 Fixed channel comparison in findType()
// 20240608 Modified implementation of maximum number of sensors
// 20240609 Fixed implementation of maximum number of sensors
// 20240714 Added option to skip initialization of include/exclude lists
// 20241205 Added radio LR1121
// 20241227 Added LilyGo T3 S3 LR1121 RF switch and TCXO configuration
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

#if defined(USE_CC1101)
#pragma message("Using CC1101 radio module")
RADIO_CHIP radio = new Module(PIN_RECEIVER_CS, PIN_RECEIVER_IRQ, RADIOLIB_NC, PIN_RECEIVER_GPIO);
#else
RADIO_CHIP radio = new Module(PIN_RECEIVER_CS, PIN_RECEIVER_IRQ, PIN_RECEIVER_RST, PIN_RECEIVER_GPIO);
#endif

#if defined(ARDUINO_LILYGO_T3S3_SX1262) || defined(ARDUINO_LILYGO_T3S3_SX1276) || defined(ARDUINO_LILYGO_T3S3_LR1121)
SPIClass *spi = nullptr;
#endif

#if defined(ARDUINO_LILYGO_T3S3_LR1121)
const uint32_t rfswitch_dio_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
    RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC
};

const Module::RfSwitchMode_t rfswitch_table[] = {
    // mode                  DIO5  DIO6
    { LR11x0::MODE_STBY,   { LOW,  LOW  } },
    { LR11x0::MODE_RX,     { HIGH, LOW  } },
    { LR11x0::MODE_TX,     { LOW,  HIGH } },
    { LR11x0::MODE_TX_HP,  { LOW,  HIGH } },
    { LR11x0::MODE_TX_HF,  { LOW,  LOW  } },
    { LR11x0::MODE_GNSS,   { LOW,  LOW  } },
    { LR11x0::MODE_WIFI,   { LOW,  LOW  } },
    END_OF_MODE_TABLE,
};
#endif

// Flag to indicate that a packet was received
volatile bool receivedFlag = false;

// This function is called when a complete packet is received by the module
// IMPORTANT: This function MUST be 'void' type and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
    IRAM_ATTR
#endif
void setFlag(void)
{
    // We got a packet, set the flag
    receivedFlag = true;
}

int16_t WeatherSensor::begin(uint8_t max_sensors_default, bool init_filters, double frequency_offset)
{
    uint8_t maxSensors = max_sensors_default;
    getSensorsCfg(maxSensors, rxFlags, enDecoders);
    log_d("max_sensors: %u", maxSensors);
    log_d("rx_flags: %u", rxFlags);
    log_d("en_decoders: %u", enDecoders);
    sensor.resize(maxSensors);

    if (init_filters)
    {
        // List of sensor IDs to be excluded - can be empty
        std::vector<uint32_t> sensor_ids_exc_def = SENSOR_IDS_EXC;
        initList(sensor_ids_exc, sensor_ids_exc_def, "exc");

        // List of sensor IDs to be included - if zero, handle all available sensors
        std::vector<uint32_t> sensor_ids_inc_def = SENSOR_IDS_INC;
        initList(sensor_ids_inc, sensor_ids_inc_def, "inc");
    }
    
    #if defined(ARDUINO_LILYGO_T3S3_SX1262) || defined(ARDUINO_LILYGO_T3S3_SX1276) || defined(ARDUINO_LILYGO_T3S3_LR1121)
    spi = new SPIClass(SPI);
    spi->begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    radio = new Module(PIN_RECEIVER_CS, PIN_RECEIVER_IRQ, PIN_RECEIVER_RST, PIN_RECEIVER_GPIO, *spi);
    #endif

    double frequency = 868.3 + frequency_offset;
    log_d("Setting frequency to %f MHz", 868.3 + frequency_offset);
  
    // https://github.com/RFD-FHEM/RFFHEM/issues/607#issuecomment-830818445
    // Freq: 868.300 MHz, Bandwidth: 203 KHz, rAmpl: 33 dB, sens: 8 dB, DataRate: 8207.32 Baud
    log_d("%s Initializing ... ", RECEIVER_CHIP);
  
    // carrier frequency:                   868.3 MHz
    // bit rate:                            8.22 kbps
    // frequency deviation:                 57.136417 kHz
    // Rx bandwidth:                        270.0 kHz (CC1101) / 250 kHz (SX1276) / 234.3 kHz (SX1262)
    // output power:                        10 dBm
    // preamble length:                     40 bits
#if defined(USE_CC1101)
    int state = radio.begin(frequency, 8.21, 57.136417, 270, 10, 32);
#elif defined(USE_SX1276)
    int state = radio.beginFSK(frequency, 8.21, 57.136417, 250, 10, 32);
#elif defined(USE_SX1262)
    int state = radio.beginFSK(frequency, 8.21, 57.136417, 234.3, 10, 32);
#else
    // defined(USE_LR1121)
    int state = radio.beginGFSK(frequency, 8.21, 57.136417, 234.3, 10, 32);
#endif

#if defined(ARDUINO_LILYGO_T3S3_LR1121)
    // set RF switch control configuration
    radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

    // LR1121 TCXO Voltage 2.85~3.15V
    radio.setTCXO(3.0);
#endif

    if (state == RADIOLIB_ERR_NONE)
    {
        log_d("success!");
        state = radio.fixedPacketLengthMode(MSG_BUF_SIZE);
        if (state != RADIOLIB_ERR_NONE)
        {
            log_e("%s Error setting fixed packet length: [%d]", RECEIVER_CHIP, state);
            while (true)
                delay(10);
        }
#if defined(USE_SX1262) || defined(USE_LR1121)
        state = radio.setCRC(0);
#else
        state = radio.setCrcFiltering(false);
#endif
        if (state != RADIOLIB_ERR_NONE)
        {
            log_e("%s Error disabling crc filtering: [%d]", RECEIVER_CHIP, state);
            while (true)
                delay(10);
        }

// Preamble: AA AA AA AA AA
// Sync is: 2D D4
// Preamble 40 bits but the CC1101 doesn't allow us to set that
// so we use a preamble of 32 bits and then use the sync as AA 2D
// which then uses the last byte of the preamble - we recieve the last sync byte
// as the 1st byte of the payload.
#if defined(USE_CC1101)
        state = radio.setSyncWord(0xAA, 0x2D, 0, false);
#else
        uint8_t sync_word[] = {0xAA, 0x2D};
        state = radio.setSyncWord(sync_word, 2);
#endif
        if (state != RADIOLIB_ERR_NONE)
        {
            log_e("%s Error setting sync words: [%d]", RECEIVER_CHIP, state);
            while (true)
                delay(10);
        }
    }
    else
    {
        log_e("%s Error initialising: [%d]", RECEIVER_CHIP, state);
        while (true)
            delay(10);
    }
    log_d("%s Setup complete - awaiting incoming messages...", RECEIVER_CHIP);
    rssi = radio.getRSSI();

    // Set callback function
    radio.setPacketReceivedAction(setFlag);

    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE)
    {
        log_e("%s startReceive() failed, code %d", RECEIVER_CHIP, state);
        while (true)
            delay(10);
    }

    return state;
}

void WeatherSensor::radioReset(void)
{
    radio.reset();
}

void WeatherSensor::sleep(void)
{
    radio.sleep();
}

bool WeatherSensor::getData(uint32_t timeout, uint8_t flags, uint8_t type, void (*func)())
{
    const uint32_t timestamp = millis();

    radio.startReceive();

    while ((millis() - timestamp) < timeout)
    {
        int decode_status = getMessage();

        // Callback function (see https://www.geeksforgeeks.org/callbacks-in-c/)
        if (func)
        {
            (*func)();
        }

        if (decode_status == DECODE_OK)
        {
            bool all_slots_valid = true;
            bool all_slots_complete = true;

            for (size_t i = 0; i < sensor.size(); i++)
            {
                if (!sensor[i].valid)
                {
                    all_slots_valid = false;
                    continue;
                }

                // No special requirements, one valid message is sufficient
                if (flags == 0)
                {
                    radio.standby();
                    return true;
                }

                // Specific sensor type required
                if (((flags & DATA_TYPE) != 0) && (sensor[i].s_type == type))
                {
                    if (sensor[i].complete || !(flags & DATA_COMPLETE))
                    {
                        radio.standby();
                        return true;
                    }
                }
                // All slots required (valid AND complete) - must check all slots
                else if (flags & DATA_ALL_SLOTS)
                {
                    all_slots_valid &= sensor[i].valid;
                    all_slots_complete &= sensor[i].complete;
                }
                // At least one sensor valid and complete
                else if (sensor[i].complete)
                {
                    radio.standby();
                    return true;
                }
            } // for (size_t i=0; i<sensor.size(); i++)

            // All slots required (valid AND complete)
            if ((flags & DATA_ALL_SLOTS) && all_slots_valid && all_slots_complete)
            {
                radio.standby();
                return true;
            }

        } // if (decode_status == DECODE_OK)
    } //  while ((millis() - timestamp) < timeout)

    // Timeout
    radio.standby();
    return false;
}

DecodeStatus WeatherSensor::getMessage(void)
{
    uint8_t recvData[MSG_BUF_SIZE];
    DecodeStatus decode_res = DECODE_INVALID;

    // Receive data
    if (receivedFlag)
    {
        receivedFlag = false;

        int state = radio.readData(recvData, MSG_BUF_SIZE);
        rssi = radio.getRSSI();
        state = radio.startReceive();

        if (state == RADIOLIB_ERR_NONE)
        {
            // Verify last syncword is 1st byte of payload (see setSyncWord() above)
            if (recvData[0] == 0xD4)
            {
#if CORE_DEBUG_LEVEL == ARDUHAL_LOG_LEVEL_VERBOSE
                char buf[128];
                *buf = '\0';
                for (size_t i = 0; i < sizeof(recvData); i++)
                {
                    sprintf(&buf[strlen(buf)], "%02X ", recvData[i]);
                }
                log_v("%s Data: %s", RECEIVER_CHIP, buf);
#endif
                log_d("%s R [%02X] RSSI: %0.1f", RECEIVER_CHIP, recvData[0], rssi);

                decode_res = decodeMessage(&recvData[1], sizeof(recvData) - 1);
            } // if (recvData[0] == 0xD4)
        } // if (state == RADIOLIB_ERR_NONE)
        else if (state == RADIOLIB_ERR_RX_TIMEOUT)
        {
            log_v("T");
        }
        else
        {
            // some other error occurred
            log_d("%s Receive failed: [%d]", RECEIVER_CHIP, state);
        }
    }

    return decode_res;
}

//
// Generate sample data for testing
//
bool WeatherSensor::genMessage(int i, uint32_t id, uint8_t s_type, uint8_t channel, uint8_t startup)
{
    sensor[i].sensor_id = id;
    sensor[i].s_type = s_type;
    sensor[i].startup = startup;
    sensor[i].chan = channel;
    sensor[i].battery_ok = true;
    sensor[i].rssi = 88.8;
    sensor[i].valid = true;
    sensor[i].complete = true;

    if ((s_type == SENSOR_TYPE_WEATHER0) || (s_type == SENSOR_TYPE_WEATHER1))
    {
        sensor[i].w.temp_ok = true;
        sensor[i].w.temp_c = 22.2f;
        sensor[i].w.humidity_ok = true;
        sensor[i].w.humidity = 55;
#ifdef WIND_DATA_FLOATINGPOINT
        sensor[i].w.wind_direction_deg = 111.1;
        sensor[i].w.wind_gust_meter_sec = 4.4f;
        sensor[i].w.wind_avg_meter_sec = 3.3f;
#endif
#ifdef WIND_DATA_FIXEDPOINT
        sensor[i].w.wind_direction_deg_fp1 = 1111;
        sensor[i].w.wind_gust_meter_sec_fp1 = 44;
        sensor[i].w.wind_avg_meter_sec_fp1 = 33;
#endif
        sensor[i].w.wind_ok = true;
        sensor[i].w.rain_ok = true;
        sensor[i].w.rain_mm = 9.9f;
    }
    else if (s_type == SENSOR_TYPE_LIGHTNING)
    {
        sensor[i].lgt.strike_count = 42;
        sensor[i].lgt.distance_km = 22;
    }
    else if (s_type == SENSOR_TYPE_LEAKAGE)
    {
        sensor[i].leak.alarm = 0;
    }
    else if (s_type == SENSOR_TYPE_SOIL)
    {
        sensor[i].soil.temp_c = 7.7f;
        sensor[i].soil.moisture = 50;
    }
    else if (s_type == SENSOR_TYPE_AIR_PM)
    {
        sensor[i].pm.pm_2_5 = 1234;
        sensor[i].pm.pm_10 = 1567;
    }

    return true;
}

//
// Find required sensor data by ID
//
int WeatherSensor::findId(uint32_t id)
{
    for (size_t i = 0; i < sensor.size(); i++)
    {
        if (sensor[i].valid && (sensor[i].sensor_id == id))
            return i;
    }
    return -1;
}

//
// Find required sensor data by type and (optionally) channel
//
int WeatherSensor::findType(uint8_t type, uint8_t ch)
{
    for (size_t i = 0; i < sensor.size(); i++)
    {
        if (sensor[i].valid && (sensor[i].s_type == type) &&
            ((ch == 0xFF) || (sensor[i].chan == ch)))
            return i;
    }
    return -1;
}

//
// From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/util.c
//
uint16_t WeatherSensor::lfsr_digest16(uint8_t const message[], unsigned bytes, uint16_t gen, uint16_t key)
{
    uint16_t sum = 0;
    for (unsigned k = 0; k < bytes; ++k)
    {
        uint8_t data = message[k];
        for (int i = 7; i >= 0; --i)
        {
            // fprintf(stderr, "key at bit %d : %04x\n", i, key);
            // if data bit is set then xor with key
            if ((data >> i) & 1)
                sum ^= key;

            // roll the key right (actually the lsb is dropped here)
            // and apply the gen (needs to include the dropped lsb as msb)
            if (key & 1)
                key = (key >> 1) ^ gen;
            else
                key = (key >> 1);
        }
    }
    return sum;
}

//
// From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/util.c
//
int WeatherSensor::add_bytes(uint8_t const message[], unsigned num_bytes)
{
    int result = 0;
    for (unsigned i = 0; i < num_bytes; ++i)
    {
        result += message[i];
    }
    return result;
}

//
// From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/util.c
//
uint16_t WeatherSensor::crc16(uint8_t const message[], unsigned nBytes, uint16_t polynomial, uint16_t init)
{
    uint16_t remainder = init;
    unsigned byte, bit;

    for (byte = 0; byte < nBytes; ++byte)
    {
        remainder ^= message[byte] << 8;
        for (bit = 0; bit < 8; ++bit)
        {
            if (remainder & 0x8000)
            {
                remainder = (remainder << 1) ^ polynomial;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder;
}
