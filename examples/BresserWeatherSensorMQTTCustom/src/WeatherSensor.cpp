///////////////////////////////////////////////////////////////////////////////////////////////////
// WeatherSensor.cpp
//
// Bresser 5-in-1/6-in-1/7-in1 868 MHz Weather Sensor Radio Receiver
// based on CC1101 or SX1276/RFM95W and ESP32/ESP8266
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
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

#if defined(USE_CC1101)
static CC1101 radio = new Module(PIN_RECEIVER_CS, PIN_RECEIVER_IRQ, RADIOLIB_NC, PIN_RECEIVER_GPIO);
#endif
#if defined(USE_SX1276)
static SX1276 radio = new Module(PIN_RECEIVER_CS, PIN_RECEIVER_IRQ, PIN_RECEIVER_RST, PIN_RECEIVER_GPIO);
#endif
#if defined(USE_SX1262)
static SX1262 radio = new Module(PIN_RECEIVER_CS, PIN_RECEIVER_IRQ, PIN_RECEIVER_RST, PIN_RECEIVER_GPIO);
#endif

// Flag to indicate that a packet was received
volatile bool receivedFlag = false;

// This function is called when a complete packet is received by the module
// IMPORTANT: This function MUST be 'void' type and MUST NOT have any arguments!
void
#if defined(ESP8266) || defined(ESP32)
    IRAM_ATTR
#endif
    setFlag(void)
{
    // We got a packet, set the flag
    receivedFlag = true;
}

int16_t WeatherSensor::begin(void)
{
    uint8_t maxSensors;
    getSensorsCfg(maxSensors, rxFlags);
    log_d("max_sensors: %u", maxSensors);
    log_d("rx_flags: %u", rxFlags);
    sensor.resize(maxSensors);

    // List of sensor IDs to be excluded - can be empty
    std::vector<uint32_t> sensor_ids_exc_def = SENSOR_IDS_EXC;
    initList(sensor_ids_exc, sensor_ids_exc_def, "exc");

    // List of sensor IDs to be included - if zero, handle all available sensors
    std::vector<uint32_t> sensor_ids_inc_def = SENSOR_IDS_INC;
    initList(sensor_ids_inc, sensor_ids_inc_def, "inc");

    // https://github.com/RFD-FHEM/RFFHEM/issues/607#issuecomment-830818445
    // Freq: 868.300 MHz, Bandwidth: 203 KHz, rAmpl: 33 dB, sens: 8 dB, DataRate: 8207.32 Baud
    log_d("%s Initializing ... ", RECEIVER_CHIP);
// carrier frequency:                   868.3 MHz
// bit rate:                            8.22 kbps
// frequency deviation:                 57.136417 kHz
// Rx bandwidth:                        270.0 kHz (CC1101) / 250 kHz (SX1276) / 234.3 kHz (SX1262)
// output power:                        10 dBm
// preamble length:                     40 bits
#ifdef USE_CC1101
    int state = radio.begin(868.3, 8.21, 57.136417, 270, 10, 32);
#elif defined(USE_SX1276)
    int state = radio.beginFSK(868.3, 8.21, 57.136417, 250, 10, 32);
#else
    int state = radio.beginFSK(868.3, 8.21, 57.136417, 234.3, 10, 32);
#endif
    if (state == RADIOLIB_ERR_NONE)
    {
        log_d("success!");
        state = radio.fixedPacketLengthMode(MSG_BUF_SIZE);
        if (state != RADIOLIB_ERR_NONE)
        {
            log_e("%s Error setting fixed packet length: [%d]", RECEIVER_CHIP, state);
            while (true)
                ;
        }
#ifdef USE_SX1262
        state = radio.setCRC(0);
#else
        state = radio.setCrcFiltering(false);
#endif
        if (state != RADIOLIB_ERR_NONE)
        {
            log_e("%s Error disabling crc filtering: [%d]", RECEIVER_CHIP, state);
            while (true)
                ;
        }

// Preamble: AA AA AA AA AA
// Sync is: 2D D4
// Preamble 40 bits but the CC1101 doesn't allow us to set that
// so we use a preamble of 32 bits and then use the sync as AA 2D
// which then uses the last byte of the preamble - we recieve the last sync byte
// as the 1st byte of the payload.
#ifdef USE_CC1101
        state = radio.setSyncWord(0xAA, 0x2D, 0, false);
#else
        uint8_t sync_word[] = {0xAA, 0x2D};
        state = radio.setSyncWord(sync_word, 2);
#endif
        if (state != RADIOLIB_ERR_NONE)
        {
            log_e("%s Error setting sync words: [%d]", RECEIVER_CHIP, state);
            while (true)
                ;
        }
    }
    else
    {
        log_e("%s Error initialising: [%d]", RECEIVER_CHIP, state);
        while (true)
            ;
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
            ;
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

            for (int i = 0; i < sensor.size(); i++)
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
            } // for (int i=0; i<sensor.size(); i++)

            // All slots required (valid AND complete)
            if ((flags & DATA_ALL_SLOTS) && all_slots_valid && all_slots_complete)
            {
                radio.standby();
                return true;
            }

        } // if (decode_status == DECODE_OK)
    }     //  while ((millis() - timestamp) < timeout)

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
        }     // if (state == RADIOLIB_ERR_NONE)
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

DecodeStatus WeatherSensor::decodeMessage(const uint8_t *msg, uint8_t msgSize)
{
    DecodeStatus decode_res = DECODE_INVALID;

#ifdef BRESSER_7_IN_1
    decode_res = decodeBresser7In1Payload(msg, msgSize);
    if (decode_res == DECODE_OK ||
        decode_res == DECODE_FULL ||
        decode_res == DECODE_SKIP)
    {
        return decode_res;
    }
#endif
#ifdef BRESSER_6_IN_1
    decode_res = decodeBresser6In1Payload(msg, msgSize);
    if (decode_res == DECODE_OK ||
        decode_res == DECODE_FULL ||
        decode_res == DECODE_SKIP)
    {
        return decode_res;
    }
#endif
#ifdef BRESSER_5_IN_1
    decode_res = decodeBresser5In1Payload(msg, msgSize);
    if (decode_res == DECODE_OK ||
        decode_res == DECODE_FULL ||
        decode_res == DECODE_SKIP)
    {
        return decode_res;
    }
#endif
#ifdef BRESSER_LIGHTNING
    decode_res = decodeBresserLightningPayload(msg, msgSize);
    if (decode_res == DECODE_OK ||
        decode_res == DECODE_FULL ||
        decode_res == DECODE_SKIP)
    {
        return decode_res;
    }
#endif
#ifdef BRESSER_LEAKAGE
    decode_res = decodeBresserLeakagePayload(msg, msgSize);
#endif
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
// Find slot in sensor data array
//
int WeatherSensor::findSlot(uint32_t id, DecodeStatus *status)
{
    log_v("find_slot(): ID=%08X", id);

    // Skip sensors from exclude-list (if any)
    for (const uint32_t &exc : sensor_ids_exc)
    {
        if (id == exc)
        {
            log_v("In Exclude-List, skipping!");
            *status = DECODE_SKIP;
            return -1;
        }
    }

    // Handle sensors from include-list (if not empty)
    if (sensor_ids_inc.size() > 0)
    {
        bool found = false;
        for (const uint32_t &inc : sensor_ids_inc)
        {
            if (id == inc)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            log_v("Not in Include-List, skipping!");
            *status = DECODE_SKIP;
            return -1;
        }
    }

    // Search all slots
    int free_slot = -1;
    int update_slot = -1;
    for (int i = 0; i < sensor.size(); i++)
    {
        log_d("sensor[%d]: v=%d id=0x%08X t=%d c=%d", i, sensor[i].valid, (unsigned int)sensor[i].sensor_id, sensor[i].s_type, sensor[i].complete);

        // Save first free slot
        if (!sensor[i].valid && (free_slot < 0))
        {
            free_slot = i;
        }

        // Check if sensor has already been stored
        else if (sensor[i].valid && (sensor[i].sensor_id == id))
        {
            update_slot = i;
        }
    }

    if (update_slot > -1)
    {
        // Update slot
        log_v("find_slot(): Updating slot #%d", update_slot);
        *status = DECODE_OK;
        return update_slot;
    }
    else if (free_slot > -1)
    {
        // Store to free slot
        log_v("find_slot(): Storing into slot #%d", free_slot);
        *status = DECODE_OK;
        return free_slot;
    }
    else
    {
        log_v("find_slot(): No slot left");
        // No slot left
        *status = DECODE_FULL;
        return -1;
    }
}

//
// Find required sensor data by ID
//
int WeatherSensor::findId(uint32_t id)
{
    for (int i = 0; i < sensor.size(); i++)
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
    for (int i = 0; i < sensor.size(); i++)
    {
        if (sensor[i].valid && (sensor[i].s_type == type) &&
            ((ch == 0xFF) || (sensor[i].chan = ch)))
            return i;
    }
    return -1;
}

// Initialize list of sensor IDs
void WeatherSensor::initList(std::vector<uint32_t> &list, const std::vector<uint32_t> list_def, const char *key)
{
    list.clear();
    cfgPrefs.begin("BWS-CFG", false);
    log_d("Key %s in preferences? %d", key, cfgPrefs.isKey(key));
    if (cfgPrefs.isKey(key))
    {
        size_t size = cfgPrefs.getBytesLength(key);
        log_d("Using sensor_ids_%s list from Preferences (%d bytes)", key, size);
        uint8_t buf[48];
        cfgPrefs.getBytes(key, buf, size);
        if ((buf[0] | buf[1] | buf[2] | buf[3]) == 0)
        {
            size = 0;
        }
        for (size_t i = 0; i < size; i += 4)
        {
            list.push_back(
                (buf[i] << 24) |
                (buf[i + 1] << 16) |
                (buf[i + 2] << 8) |
                buf[i + 3]);
        }
    }
    else
    {
        log_d("Using sensor_ids_%s list from WeatherSensorCfg.h", key);
        list = list_def;
    }
    cfgPrefs.end();

    if (list.size() == 0)
    {
        log_d("(empty)");
    }
    for (size_t i = 0; i < list.size(); i++)
    {
        log_d("%08X", list[i]);
    }
}

// Set sensors include list in Preferences
void WeatherSensor::setSensorsInc(uint8_t *buf, uint8_t size)
{
    log_d("size: %d", size);
    cfgPrefs.begin("BWS-CFG", false);
    cfgPrefs.putBytes("inc", buf, size);
    cfgPrefs.end();

    sensor_ids_inc.clear();
    if ((buf[0] | buf[1] | buf[2] | buf[3]) == 0)
    {
        size = 0;
    }
    for (size_t i = 0; i < size; i += 4)
    {
        sensor_ids_inc.push_back(
            (buf[i] << 24) |
            (buf[i + 1] << 16) |
            (buf[i + 2] << 8) |
            buf[i + 3]);
    }
}

// Get sensors include list from Preferences
uint8_t WeatherSensor::getSensorsInc(uint8_t *payload)
{
    cfgPrefs.begin("BWS-CFG", false);
    uint8_t size = cfgPrefs.getBytesLength("inc");
    cfgPrefs.getBytes("inc", payload, size);
    cfgPrefs.end();
    log_d("size: %d", size);

    return size;
}

// Set sensors exclude list in Preferences
void WeatherSensor::setSensorsExc(uint8_t *buf, uint8_t size)
{
    log_d("size: %d", size);
    cfgPrefs.begin("BWS-CFG", false);
    cfgPrefs.putBytes("exc", buf, size);
    cfgPrefs.end();

    sensor_ids_exc.clear();
    if ((buf[0] | buf[1] | buf[2] | buf[3]) == 0)
    {
        size = 0;
    }
    for (size_t i = 0; i < size; i += 4)
    {
        sensor_ids_exc.push_back(
            (buf[i] << 24) |
            (buf[i + 1] << 16) |
            (buf[i + 2] << 8) |
            buf[i + 3]);
    }
}

// Get sensors exclude list from Preferences
uint8_t WeatherSensor::getSensorsExc(uint8_t *payload)
{
    cfgPrefs.begin("BWS-CFG", false);
    uint8_t size = cfgPrefs.getBytesLength("exc");
    cfgPrefs.getBytes("exc", payload, size);
    cfgPrefs.end();
    log_d("size: %d", size);

    return size;
}

// Set sensor configuration and store in in Preferences
void WeatherSensor::setSensorsCfg(uint8_t maxSensors, uint8_t rxFlags)
{
    cfgPrefs.begin("BWS-CFG", false);
    cfgPrefs.putUChar("maxsensors", maxSensors);
    cfgPrefs.getUChar("rxflags", rxFlags);
    cfgPrefs.end();
    log_d("max_sensors: %u", maxSensors);
    log_d("rx_flags: %u", rxFlags);
    sensor.resize(maxSensors);
}

// Get sensor configuration from Preferences
void WeatherSensor::getSensorsCfg(uint8_t &maxSensors, uint8_t &rxFlags)
{
    cfgPrefs.begin("BWS-CFG", false);
    maxSensors = cfgPrefs.getUChar("maxsensors", MAX_SENSORS_DEFAULT);
    rxFlags = cfgPrefs.getUChar("rxflags", DATA_COMPLETE);
    cfgPrefs.end();
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

//
// From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_5in1.c (20220212)
//
// Example input data:
//   EA EC 7F EB 5F EE EF FA FE 76 BB FA FF 15 13 80 14 A0 11 10 05 01 89 44 05 00
//   CC CC CC CC CC CC CC CC CC CC CC CC CC uu II sS GG DG WW  W TT  T HH RR RR Bt
// - C = Check, inverted data of 13 byte further
// - uu = checksum (number/count of set bits within bytes 14-25)
// - I = station ID (maybe)
// - G = wind gust in 1/10 m/s, normal binary coded, GGxG = 0x76D1 => 0x0176 = 256 + 118 = 374 => 37.4 m/s.  MSB is out of sequence.
// - D = wind direction 0..F = N..NNE..E..S..W..NNW
// - W = wind speed in 1/10 m/s, BCD coded, WWxW = 0x7512 => 0x0275 = 275 => 27.5 m/s. MSB is out of sequence.
// - T = temperature in 1/10 °C, BCD coded, TTxT = 1203 => 31.2 °C
// - t = temperature sign, minus if unequal 0
// - H = humidity in percent, BCD coded, HH = 23 => 23 %
// - R = rain in mm, BCD coded, RRRR = 1203 => 031.2 mm
// - B = Battery. 0=Ok, 8=Low.
// - s = startup, 0 after power-on/reset / 8 after 1 hour
// - S = sensor type, only low nibble used, 0x9 for Bresser Professional Rain Gauge
//
// Parameters:
//
// msg     - Pointer to message
// msgSize - Size of message
// pOut    - Pointer to WeatherData
//
// Returns:
//
// DECODE_OK      - OK - WeatherData will contain the updated information
// DECODE_PAR_ERR - Parity Error
// DECODE_CHK_ERR - Checksum Error
//
#ifdef BRESSER_5_IN_1
DecodeStatus WeatherSensor::decodeBresser5In1Payload(const uint8_t *msg, uint8_t msgSize)
{
    // First 13 bytes need to match inverse of last 13 bytes
    for (unsigned col = 0; col < msgSize / 2; ++col)
    {
        if ((msg[col] ^ msg[col + 13]) != 0xff)
        {
            log_d("Parity wrong at column %d", col);
            return DECODE_PAR_ERR;
        }
    }

    // Verify checksum (number number bits set in bytes 14-25)
    uint8_t bitsSet = 0;
    uint8_t expectedBitsSet = msg[13];

    for (uint8_t p = 14; p < msgSize; p++)
    {
        uint8_t currentByte = msg[p];
        while (currentByte)
        {
            bitsSet += (currentByte & 1);
            currentByte >>= 1;
        }
    }

    if (bitsSet != expectedBitsSet)
    {
        log_d("Checksum wrong - actual [%02X] != [%02X]", bitsSet, expectedBitsSet);
        return DECODE_CHK_ERR;
    }

    uint8_t id_tmp = msg[14];
    uint8_t type_tmp = msg[15] & 0xF;
    DecodeStatus status;

    // Find appropriate slot in sensor data array and update <status>
    int slot = findSlot(id_tmp, &status);

    if (status != DECODE_OK)
        return status;

    sensor[slot].sensor_id = id_tmp;
    sensor[slot].chan = 0; // for compatibility with other decoders
    sensor[slot].startup = ((msg[15] & 0x80) == 0) ? true : false;
    sensor[slot].battery_ok = (msg[25] & 0x80) ? false : true;
    sensor[slot].valid = true;
    sensor[slot].rssi = rssi;
    sensor[slot].complete = true;

    int temp_raw = (msg[20] & 0x0f) + ((msg[20] & 0xf0) >> 4) * 10 + (msg[21] & 0x0f) * 100;
    if (msg[25] & 0x0f)
    {
        temp_raw = -temp_raw;
    }
    sensor[slot].w.temp_c = temp_raw * 0.1f;

    sensor[slot].w.humidity = (msg[22] & 0x0f) + ((msg[22] & 0xf0) >> 4) * 10;

    int wind_direction_raw = ((msg[17] & 0xf0) >> 4) * 225;
    int gust_raw = ((msg[17] & 0x0f) << 8) + msg[16];
    int wind_raw = (msg[18] & 0x0f) + ((msg[18] & 0xf0) >> 4) * 10 + (msg[19] & 0x0f) * 100;

#ifdef WIND_DATA_FLOATINGPOINT
    sensor[slot].w.wind_direction_deg = wind_direction_raw * 0.1f;
    sensor[slot].w.wind_gust_meter_sec = gust_raw * 0.1f;
    sensor[slot].w.wind_avg_meter_sec = wind_raw * 0.1f;
#endif
#ifdef WIND_DATA_FIXEDPOINT
    sensor[slot].w.wind_direction_deg_fp1 = wind_direction_raw;
    sensor[slot].w.wind_gust_meter_sec_fp1 = gust_raw;
    sensor[slot].w.wind_avg_meter_sec_fp1 = wind_raw;
#endif

    int rain_raw = (msg[23] & 0x0f) + ((msg[23] & 0xf0) >> 4) * 10 + (msg[24] & 0x0f) * 100 + ((msg[24] & 0xf0) >> 4) * 1000;
    sensor[slot].w.rain_mm = rain_raw * 0.1f;

    // Check if the message is from a Bresser Professional Rain Gauge
    // This sensor has the same type as the Bresser Lightning Sensor -
    // we change this to SENSOR_TYPE_WEATHER0 to simplify processing by the application.
    if (type_tmp == 0x9)
    {
        // rescale the rain sensor readings
        sensor[slot].w.rain_mm *= 2.5;
        type_tmp = SENSOR_TYPE_WEATHER0;

        // Rain Gauge has no humidity (according to description) and no wind sensor (obviously)
        sensor[slot].w.humidity_ok = false;
        sensor[slot].w.wind_ok = false;
    }
    else
    {
        sensor[slot].w.humidity_ok = true;
        sensor[slot].w.wind_ok = true;
    }

    sensor[slot].s_type = type_tmp;
    sensor[slot].w.temp_ok = true;
    sensor[slot].w.light_ok = false;
    sensor[slot].w.uv_ok = false;
    sensor[slot].w.rain_ok = true;

    return DECODE_OK;
}
#endif

//
// From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_6in1.c (20220608)
//
// - also Bresser Weather Center 7-in-1 indoor sensor.
// - also Bresser new 5-in-1 sensors.
// - also Froggit WH6000 sensors.
// - also rebranded as Ventus C8488A (W835)
// - also Bresser 3-in-1 Professional Wind Gauge / Anemometer PN 7002531
// - also Bresser Pool / Spa Thermometer PN 7009973 (s_type = 3)
//
// There are at least two different message types:
// - 24 seconds interval for temperature, hum, uv and rain (alternating messages)
// - 12 seconds interval for wind data (every message)
//
// Also Bresser Explore Scientific SM60020 Soil moisture Sensor.
// https://www.bresser.de/en/Weather-Time/Accessories/EXPLORE-SCIENTIFIC-Soil-Moisture-and-Soil-Temperature-Sensor.html
//
// Moisture:
//
//     f16e 187000e34 7 ffffff0000 252 2 16 fff 004 000 [25,2, 99%, CH 7]
//     DIGEST:8h8h ID?8h8h8h8h TYPE:4h STARTUP:1b CH:3d 8h 8h8h 8h8h TEMP:12h ?2b BATT:1b ?1b MOIST:8h UV?~12h ?4h CHKSUM:8h
//
// Moisture is transmitted in the humidity field as index 1-16: 0, 7, 13, 20, 27, 33, 40, 47, 53, 60, 67, 73, 80, 87, 93, 99.
// The Wind speed and direction fields decode to valid zero but we exclude them from the output.
//
//     aaaa2dd4e3ae1870079341ffffff0000221201fff279 [Batt ok]
//     aaaa2dd43d2c1870079341ffffff0000219001fff2fc [Batt low]
//
//     {206}55555555545ba83e803100058631ff11fe6611ffffffff01cc00 [Hum 96% Temp 3.8 C Wind 0.7 m/s]
//     {205}55555555545ba999263100058631fffffe66d006092bffe0cff8 [Hum 95% Temp 3.0 C Wind 0.0 m/s]
//     {199}55555555545ba840523100058631ff77fe668000495fff0bbe [Hum 95% Temp 3.0 C Wind 0.4 m/s]
//     {205}55555555545ba94d063100058631fffffe665006092bffe14ff8
//     {206}55555555545ba860703100058631fffffe6651ffffffff0135fc [Hum 95% Temp 3.0 C Wind 0.0 m/s]
//     {205}55555555545ba924d23100058631ff99fe68b004e92dffe073f8 [Hum 96% Temp 2.7 C Wind 0.4 m/s]
//     {202}55555555545ba813403100058631ff77fe6810050929ffe1180 [Hum 94% Temp 2.8 C Wind 0.4 m/s]
//     {205}55555555545ba98be83100058631fffffe6130050929ffe17800 [Hum 95% Temp 2.8 C Wind 0.8 m/s]
//
//     2dd4  1f 40 18 80 02 c3 18 ff 88 ff 33 08 ff ff ff ff 80 e6 00 [Hum 96% Temp 3.8 C Wind 0.7 m/s]
//     2dd4  cc 93 18 80 02 c3 18 ff ff ff 33 68 03 04 95 ff f0 67 3f [Hum 95% Temp 3.0 C Wind 0.0 m/s]
//     2dd4  20 29 18 80 02 c3 18 ff bb ff 33 40 00 24 af ff 85 df    [Hum 95% Temp 3.0 C Wind 0.4 m/s]
//     2dd4  a6 83 18 80 02 c3 18 ff ff ff 33 28 03 04 95 ff f0 a7 3f
//     2dd4  30 38 18 80 02 c3 18 ff ff ff 33 28 ff ff ff ff 80 9a 7f [Hum 95% Temp 3.0 C Wind 0.0 m/s]
//     2dd4  92 69 18 80 02 c3 18 ff cc ff 34 58 02 74 96 ff f0 39 3f [Hum 96% Temp 2.7 C Wind 0.4 m/s]
//     2dd4  09 a0 18 80 02 c3 18 ff bb ff 34 08 02 84 94 ff f0 8c 0  [Hum 94% Temp 2.8 C Wind 0.4 m/s]
//     2dd4  c5 f4 18 80 02 c3 18 ff ff ff 30 98 02 84 94 ff f0 bc 00 [Hum 95% Temp 2.8 C Wind 0.8 m/s]
//
//     {147} 5e aa 18 80 02 c3 18 fa 8f fb 27 68 11 84 81 ff f0 72 00 [Temp 11.8 C  Hum 81%]
//     {149} ae d1 18 80 02 c3 18 fa 8d fb 26 78 ff ff ff fe 02 db f0
//     {150} f8 2e 18 80 02 c3 18 fc c6 fd 26 38 11 84 81 ff f0 68 00 [Temp 11.8 C  Hum 81%]
//     {149} c4 7d 18 80 02 c3 18 fc 78 fd 29 28 ff ff ff fe 03 97 f0
//     {149} 28 1e 18 80 02 c3 18 fb b7 fc 26 58 ff ff ff fe 02 c3 f0
//     {150} 21 e8 18 80 02 c3 18 fb 9c fc 33 08 11 84 81 ff f0 b7 f8 [Temp 11.8 C  Hum 81%]
//     {149} 83 ae 18 80 02 c3 18 fc 78 fc 29 28 ff ff ff fe 03 98 00
//     {150} 5c e4 18 80 02 c3 18 fb ba fc 26 98 11 84 81 ff f0 16 00 [Temp 11.8 C  Hum 81%]
//     {148} d0 bd 18 80 02 c3 18 f9 ad fa 26 48 ff ff ff fe 02 ff f0
//
// Wind and Temperature/Humidity or Rain:
//
//     DIGEST:8h8h ID:8h8h8h8h TYPE:4h STARTUP:1b CH:3d WSPEED:~8h~4h ~4h~8h WDIR:12h ?4h TEMP:8h.4h ?2b BATT:1b ?1b HUM:8h UV?~12h ?4h CHKSUM:8h
//     DIGEST:8h8h ID:8h8h8h8h TYPE:4h STARTUP:1b CH:3d WSPEED:~8h~4h ~4h~8h WDIR:12h ?4h RAINFLAG:8h RAIN:8h8h UV:8h8h CHKSUM:8h
//
// Digest is LFSR-16 gen 0x8810 key 0x5412, excluding the add-checksum and trailer.
// Checksum is 8-bit add (with carry) to 0xff.
//
// Notes on different sensors:
//
// - 1910 084d 18 : RebeckaJohansson, VENTUS W835
// - 2030 088d 10 : mvdgrift, Wi-Fi Colour Weather Station with 5in1 Sensor, Art.No.: 7002580, ff 01 in the UV field is (obviously) invalid.
// - 1970 0d57 18 : danrhjones, bresser 5-in-1 model 7002580, no UV
// - 18b0 0301 18 : konserninjohtaja 6-in-1 outdoor sensor
// - 18c0 0f10 18 : rege245 BRESSER-PC-Weather-station-with-6-in-1-outdoor-sensor
// - 1880 02c3 18 : f4gqk 6-in-1
// - 18b0 0887 18 : npkap
//
// Parameters:
//
//  msg     - Pointer to message
//  msgSize - Size of message
//  pOut    - Pointer to WeatherData
//
//  Returns:
//
//  DECODE_OK      - OK - WeatherData will contain the updated information
//  DECODE_DIG_ERR - Digest Check Error
//  DECODE_CHK_ERR - Checksum Error
#ifdef BRESSER_6_IN_1
DecodeStatus WeatherSensor::decodeBresser6In1Payload(const uint8_t *msg, uint8_t msgSize)
{
    (void)msgSize;                                                                             // unused parameter - kept for consistency with other decoders; avoid warning
    int const moisture_map[] = {0, 7, 13, 20, 27, 33, 40, 47, 53, 60, 67, 73, 80, 87, 93, 99}; // scale is 20/3

    // Per-message status flags
    bool temp_ok = false;
    bool humidity_ok = false;
    bool uv_ok = false;
    bool wind_ok = false;
    bool rain_ok = false;
    bool f_3in1 = false;

    // LFSR-16 digest, generator 0x8810 init 0x5412
    int chkdgst = (msg[0] << 8) | msg[1];
    int digest = lfsr_digest16(&msg[2], 15, 0x8810, 0x5412);
    if (chkdgst != digest)
    {
        log_d("Digest check failed - [%02X] != [%02X]", chkdgst, digest);
        return DECODE_DIG_ERR;
    }
    // Checksum, add with carry
    int sum = add_bytes(&msg[2], 16); // msg[2] to msg[17]
    if ((sum & 0xff) != 0xff)
    {
        log_d("Checksum failed");
        return DECODE_CHK_ERR;
    }

    uint32_t id_tmp = ((uint32_t)msg[2] << 24) | (msg[3] << 16) | (msg[4] << 8) | (msg[5]);
    uint8_t type_tmp = (msg[6] >> 4); // 1: weather station, 2: indoor?, 4: soil probe
    uint8_t chan_tmp = (msg[6] & 0x7);
    uint8_t flags = (msg[16] & 0x0f);
    DecodeStatus status;

    // Find appropriate slot in sensor data array and update <status>
    int slot = findSlot(id_tmp, &status);

    if (status != DECODE_OK)
        return status;

    if (!sensor[slot].valid)
    {
        // Reset value after if slot is empty
        sensor[slot].w.temp_ok = false;
        sensor[slot].w.humidity_ok = false;
        sensor[slot].w.uv_ok = false;
        sensor[slot].w.wind_ok = false;
        sensor[slot].w.rain_ok = false;
    }
    sensor[slot].sensor_id = id_tmp;
    sensor[slot].s_type = type_tmp;
    sensor[slot].chan = chan_tmp;
    sensor[slot].startup = ((msg[6] & 0x8) == 0) ? true : false; // s.a. #1214
    sensor[slot].battery_ok = (msg[13] >> 1) & 1;                // b[13] & 0x02 is battery_good, s.a. #1993

    // temperature, humidity(, uv) - shared with rain counter
    temp_ok = humidity_ok = (flags == 0);
    float temp = 0;
    if (temp_ok)
    {
        bool sign = (msg[13] >> 3) & 1;
        int temp_raw = (msg[12] >> 4) * 100 + (msg[12] & 0x0f) * 10 + (msg[13] >> 4);

        temp = ((sign) ? (temp_raw - 1000) : temp_raw) * 0.1f;

        // Correction for Bresser 3-in-1 Professional Wind Gauge / Anemometer, PN 7002531
        // The temperature range (as far as provided in other Bresser manuals) is -40...+60°C
        if (temp < -50.0)
        {
            temp = -temp_raw * 0.1f;
        }

        sensor[slot].w.temp_c = temp;
        sensor[slot].w.humidity = (msg[14] >> 4) * 10 + (msg[14] & 0x0f);

        // apparently ff01 or 0000 if not available, ???0 if valid, inverted BCD
        uv_ok = ((~msg[15] & 0xff) <= 0x99) && ((~msg[16] & 0xf0) <= 0x90) && !f_3in1;
        if (uv_ok)
        {
            int uv_raw = ((~msg[15] & 0xf0) >> 4) * 100 + (~msg[15] & 0x0f) * 10 + ((~msg[16] & 0xf0) >> 4);
            sensor[slot].w.uv = uv_raw * 0.1f;
        }
    }

    // int unk_ok  = (msg[16] & 0xf0) == 0xf0;
    // int unk_raw = ((msg[15] & 0xf0) >> 4) * 10 + (msg[15] & 0x0f);

    // invert 3 bytes wind speeds
    uint8_t _imsg7 = msg[7] ^ 0xff;
    uint8_t _imsg8 = msg[8] ^ 0xff;
    uint8_t _imsg9 = msg[9] ^ 0xff;

    wind_ok = (_imsg7 <= 0x99) && (_imsg8 <= 0x99) && (_imsg9 <= 0x99);
    if (wind_ok)
    {
        int gust_raw = (_imsg7 >> 4) * 100 + (_imsg7 & 0x0f) * 10 + (_imsg8 >> 4);
        int wavg_raw = (_imsg9 >> 4) * 100 + (_imsg9 & 0x0f) * 10 + (_imsg8 & 0x0f);
        int wind_dir_raw = ((msg[10] & 0xf0) >> 4) * 100 + (msg[10] & 0x0f) * 10 + ((msg[11] & 0xf0) >> 4);

#ifdef WIND_DATA_FLOATINGPOINT
        sensor[slot].w.wind_gust_meter_sec = gust_raw * 0.1f;
        sensor[slot].w.wind_avg_meter_sec = wavg_raw * 0.1f;
        sensor[slot].w.wind_direction_deg = wind_dir_raw * 1.0f;
#endif
#ifdef WIND_DATA_FIXEDPOINT
        sensor[slot].w.wind_gust_meter_sec_fp1 = gust_raw;
        sensor[slot].w.wind_avg_meter_sec_fp1 = wavg_raw;
        sensor[slot].w.wind_direction_deg_fp1 = wind_dir_raw * 10;
#endif
    }

    // rain counter, inverted 3 bytes BCD - shared with temp/hum
    uint8_t _imsg12 = msg[12] ^ 0xff;
    uint8_t _imsg13 = msg[13] ^ 0xff;
    uint8_t _imsg14 = msg[14] ^ 0xff;

    rain_ok = (flags == 1) && (type_tmp == 1);
    if (rain_ok)
    {
        int rain_raw = (_imsg12 >> 4) * 100000 + (_imsg12 & 0x0f) * 10000 + (_imsg13 >> 4) * 1000 + (_imsg13 & 0x0f) * 100 + (_imsg14 >> 4) * 10 + (_imsg14 & 0x0f);
        sensor[slot].w.rain_mm = rain_raw * 0.1f;
    }

    // Pool / Spa thermometer
    if (sensor[slot].s_type == SENSOR_TYPE_POOL_THERMO)
    {
        humidity_ok = false;
    }

    // the moisture sensor might present valid readings but does not have the hardware
    if (sensor[slot].s_type == SENSOR_TYPE_SOIL)
    {
        wind_ok = 0;
        uv_ok = 0;
    }

    if (sensor[slot].s_type == SENSOR_TYPE_SOIL && temp_ok && sensor[slot].w.humidity >= 1 && sensor[slot].w.humidity <= 16)
    {
        humidity_ok = false;
        sensor[slot].soil.moisture = moisture_map[sensor[slot].w.humidity - 1];
        sensor[slot].soil.temp_c = temp;
    }

    // Update per-slot status flags
    sensor[slot].w.temp_ok |= temp_ok;
    sensor[slot].w.humidity_ok |= humidity_ok;
    sensor[slot].w.uv_ok |= uv_ok;
    sensor[slot].w.wind_ok |= wind_ok;
    sensor[slot].w.rain_ok |= rain_ok;
    log_d("Flags: Temp=%d  Hum=%d  Wind=%d  Rain=%d  UV=%d", temp_ok, humidity_ok, wind_ok, rain_ok, uv_ok);

    sensor[slot].valid = true;

    // Weather station data is split into two separate messages (except for Professional Wind Gauge)
    if (sensor[slot].s_type == SENSOR_TYPE_WEATHER1)
    {
        if (f_3in1 || (sensor[slot].w.temp_ok && sensor[slot].w.rain_ok))
        {
            sensor[slot].complete = true;
        }
    }
    else
    {
        sensor[slot].complete = true;
    }

    // Save rssi to sensor specific data set
    sensor[slot].rssi = rssi;

    return DECODE_OK;
}
#endif

//
// From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_7in1.c (20230215)
//
/**
Decoder for Bresser Weather Center 7-in-1, outdoor sensor.
See https://github.com/merbanan/rtl_433/issues/1492
Preamble:
    aa aa aa aa aa 2d d4
Observed length depends on reset_limit.
The data (not including STYPE, STARTUP, CH and maybe ID) has a whitening of 0xaa.

Weather Center
Data layout:
    {271}631d05c09e9a18abaabaaaaaaaaa8adacbacff9cafcaaaaaaa000000000000000000
    {262}10b8b4a5a3ca10aaaaaaaaaaaaaa8bcacbaaaa2aaaaaaaaaaa0000000000000000 [0.08 klx]
    {220}543bb4a5a3ca10aaaaaaaaaaaaaa8bcacbaaaa28aaaaaaaaaa00000 [0.08 klx]
    {273}2492b4a5a3ca10aaaaaaaaaaaaaa8bdacbaaaa2daaaaaaaaaa0000000000000000000 [0.08klx]
    {269}9a59b4a5a3da10aaaaaaaaaaaaaa8bdac8afea28a8caaaaaaa000000000000000000 [54.0 klx UV=2.6]
    {230}fe15b4a5a3da10aaaaaaaaaaaaaa8bdacbba382aacdaaaaaaa00000000 [109.2klx   UV=6.7]
    {254}2544b4a5a32a10aaaaaaaaaaaaaa8bdac88aaaaabeaaaaaaaa00000000000000 [200.000 klx UV=14
    DIGEST:8h8h ID?8h8h WDIR:8h4h 4h STYPE:4h STARTUP:1b CH:3d WGUST:8h.4h WAVG:8h.4h RAIN:8h8h4h.4h RAIN?:8h TEMP:8h.4hC FLAGS?:4h HUM:8h% LIGHT:8h4h,8h4hKL UV:8h.4h TRAILER:8h8h8h4h
Unit of light is kLux (not W/m²).

Air Quality Sensor PM2.5 / PM10 Sensor (PN 7009970)
Data layout:
DIGEST:8h8h ID?8h8h ?8h8h STYPE:4h STARTUP:1b CH:3b ?8h 4h ?4h8h4h PM_2_5:4h8h4h PM10:4h8h4h ?4h ?8h4h BATT:1b ?3b ?8h8h8h8h8h8h TRAILER:8h8h8h

Air Quality Sensor CO2 (PN 7009977) : issue #2813

From user manual , co2 ppm is from 400 to 5000 ppm, so it's 16 bits coded.

Samples :
Raw :
                  SType Startup & Channel

                      | |
    {207}dab6d782acd9 a 1 ad9aad9aad9aaaaaaaaaaaaaaaaae99aaaaa00 Type = 0xa = 10, Startup = 0, ch = 1
    {207}04a9d782acd8 a 1 ad9aad9aad9aaaaaaaaaaaaaaaaae99aaaaa00 Type = 0xa = 10, Startup = 0, ch = 1
    {207}04a9d782acd8 a 1 ad9aad9aad9aaaaaaaaaaaaaaaaae99aaaaa00 Type = 0xa = 10, Startup = 0, ch = 1
    {207}0dd1d782b8ee a 1 ad9aad9aad9aaaaaaaaaaaaaaaaae99aaaaa00 Type = 0xa = 10, Startup = 0, ch = 1

Data layout raw :
    DIGEST:16h ID:16h 8x8x STYPE:4h STARTUP:1b CH:3d 8x8x8x8x8x8x8x8x8x8x8x8x8x8x8x8x8x8x TRAILER:8x

XOR / de-whitened :

          0 1  2 3  4 5  6 7 8 9101112131415161718192021222324
       DIGEST   ID  ppm                  bat
            |    |    |                    |
    {200}701c 7d28 0673 0b073007300730000000000000000043300000 [ XOR from g001_868.34M_1000k.cu8 co2 ppm  673]
    {200}ae03 7d28 0672 0b073007300730000000000000000043300000 [ XOR from g001_868.34M_250k.cu8  co2 ppm  672]
    {200}ae03 7d28 0672 0b073007300730000000000000000043300000 [ XOR from g002_868.34M_1000k.cu8 co2 ppm  672]
    {200}a77b 7d28 1244 0b073007300730000000000000000043300000 [ XOR from g002_868.34M_250k.cu8  co2 ppm 1244]

Data layout de-whitened :
    DIGEST:16h ID:16h PPM:16h 8x8x8x8x8x8x8x8x8x8x4x BATT:1b 3x8x8x8x8x8x8x TRAILER:16x

Air Quality Sensor HCHO/VOC (PN 7009978) : issue #2814

From user manual , hcho ppb is from 0 to 1000 ppm, so it's 16 bits coded.
              and voc level is from 1 (bad air quality) to 5 (good air quality), so it's 4 bits coded.

Samples:
Raw :
                  SType Startup & Channel
                      | |
    {207}3f2dc4a5aaaf b 1 aaa8aaa8aaa8aaaaaaaaaaaaaaaae9feaaaa00 Type = 0xb = 11, Startup = 0, ch = 1
    {207}0c1cc4a5aaaf b 1 aaa8aaa8aaa8aaaaaaaaaaaaaaaae9ffaaaa00 Type = 0xb = 11, Startup = 0, ch = 1
    {207}3f2dc4a5aaaf b 1 aaa8aaa8aaa8aaaaaaaaaaaaaaaae9feaaaa00 Type = 0xb = 11, Startup = 0, ch = 1
    {207}0c1cc4a5aaaf b 1 aaa8aaa8aaa8aaaaaaaaaaaaaaaae9ffaaaa00 Type = 0xb = 11, Startup = 0, ch = 1
    {207}61afc4a5aaa2 b 9 aaa8aaa8aaa9aaaaaaaaaaaaaaaae9f8aaaa00 Type = 0xb = 11, Startup = 1, ch = 1
    {207}ecddc4a5aaae b 9 aaa8aaa8aaa9aaaaaaaaaaaaaaaae9fbaaaa00 Type = 0xb = 11, Startup = 1, ch = 1

Data layout raw :
    DIGEST:16h ID:16h 8x8x STYPE:4h STARTUP:1b CH:3d 8x8x8x8x8x8x8x8x8x8x8x8x8x8x8x8x8x8x TRAILER:8x

XOR / de-whitened :

          0 1  2 3  4 5  6 7 8 9101112131415161718192021 22 2324
       DIGEST   ID  ppb                  bat            voc
            |    |    |                    |              |
    {200}9587 6e0f 0005 1b0002000200020000000000000000435 4 0000 [XOR from g001_868.34M_1000k.cu8 hcho_ppb 5 voc_level 4]
    {200}a6b6 6e0f 0005 1b0002000200020000000000000000435 5 0000 [XOR from g001_868.34M_250k.cu8  hcho_ppb 5 voc_level 5]
    {200}9587 6e0f 0005 1b0002000200020000000000000000435 4 0000 [XOR from g002_868.34M_1000k.cu8 hcho_ppb 5 voc_level 4]
    {200}a6b6 6e0f 0005 1b0002000200020000000000000000435 5 0000 [XOR from g001_868.34M_250k.cu8  hcho_ppb 5 voc_level 5]
    {200}cb05 6e0f 0008 130002000200030000000000000000435 2 0000 [XOR from g003_868.34M_1000k.cu8 hcho_ppb 8 voc_level 2]
    {200}4677 6e0f 0004 130002000200030000000000000000435 1 0000 [XOR from g004_868.34M_1000k.cu8 hcho_ppb 4 voc_level 1]

Data layout de-whitened :
    DIGEST:16h ID:16h PPB:16h 8x8x8x8x8x8x8x8x8x8x4x BATT:1b 3x8x8x8x8x8x4x VOC:4h TRAILER:16x

#2816 Bresser Air Quality sensors, ignore first packet:
    The first signal is not sending the good BCD values , all at 0xF and need to be excluded from result (BCD value can't be > 9) .

First two bytes are an LFSR-16 digest, generator 0x8810 key 0xba95 with a final xor 0x6df1, which likely means we got that wrong.
*/
#ifdef BRESSER_7_IN_1
DecodeStatus WeatherSensor::decodeBresser7In1Payload(const uint8_t *msg, uint8_t msgSize)
{

    if (msg[21] == 0x00)
    {
        log_d("Data sanity check failed");
    }

    // data de-whitening
    uint8_t msgw[MSG_BUF_SIZE];
    for (unsigned i = 0; i < msgSize; ++i)
    {
        msgw[i] = msg[i] ^ 0xaa;
    }

    // LFSR-16 digest, generator 0x8810 key 0xba95 final xor 0x6df1
    int chkdgst = (msgw[0] << 8) | msgw[1];
    int digest = lfsr_digest16(&msgw[2], 23, 0x8810, 0xba95); // bresser_7in1
    if ((chkdgst ^ digest) != 0x6df1)
    { // bresser_7in1
        log_d("Digest check failed - [%04X] vs [%04X] (%04X)", chkdgst, digest, chkdgst ^ digest);
        return DECODE_DIG_ERR;
    }

#if CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
    log_message("De-whitened Data", msgw, msgSize);
#endif

    int id_tmp = (msgw[2] << 8) | (msgw[3]);
    int s_type = msg[6] >> 4; // raw data, no de-whitening

    DecodeStatus status;

    // Find appropriate slot in sensor data array and update <status>
    int slot = findSlot(id_tmp, &status);

    if (status != DECODE_OK)
        return status;

    int flags = (msgw[15] & 0x0f);
    int battery_low = (flags & 0x06) == 0x06;

    sensor[slot].sensor_id = id_tmp;
    sensor[slot].s_type = s_type;
    sensor[slot].startup = (msg[6] & 0x08) == 0x00; // raw data, no de-whitening
    sensor[slot].chan = msg[6] & 0x07;              // raw data, no de-whitening
    sensor[slot].battery_ok = !battery_low;
    sensor[slot].valid = true;
    sensor[slot].complete = true;
    sensor[slot].rssi = rssi;

    if (s_type == SENSOR_TYPE_WEATHER1)
    {
        int wdir = (msgw[4] >> 4) * 100 + (msgw[4] & 0x0f) * 10 + (msgw[5] >> 4);
        int wgst_raw = (msgw[7] >> 4) * 100 + (msgw[7] & 0x0f) * 10 + (msgw[8] >> 4);
        int wavg_raw = (msgw[8] & 0x0f) * 100 + (msgw[9] >> 4) * 10 + (msgw[9] & 0x0f);
        int rain_raw = (msgw[10] >> 4) * 100000 + (msgw[10] & 0x0f) * 10000 + (msgw[11] >> 4) * 1000 + (msgw[11] & 0x0f) * 100 + (msgw[12] >> 4) * 10 + (msgw[12] & 0x0f) * 1; // 6 digits
        float rain_mm = rain_raw * 0.1f;
        int temp_raw = (msgw[14] >> 4) * 100 + (msgw[14] & 0x0f) * 10 + (msgw[15] >> 4);
        float temp_c = temp_raw * 0.1f;
        if (temp_raw > 600)
            temp_c = (temp_raw - 1000) * 0.1f;
        int humidity = (msgw[16] >> 4) * 10 + (msgw[16] & 0x0f);
        int lght_raw = (msgw[17] >> 4) * 100000 + (msgw[17] & 0x0f) * 10000 + (msgw[18] >> 4) * 1000 + (msgw[18] & 0x0f) * 100 + (msgw[19] >> 4) * 10 + (msgw[19] & 0x0f);
        int uv_raw = (msgw[20] >> 4) * 100 + (msgw[20] & 0x0f) * 10 + (msgw[21] >> 4);

        float light_klx = lght_raw * 0.001f; // TODO: remove this
        float light_lux = lght_raw;
        float uv_index = uv_raw * 0.1f;

        // The RTL_433 decoder does not include any field to verify that these data
        // are ok, so we are assuming that they are ok if the decode status is ok.
        sensor[slot].w.temp_ok = true;
        sensor[slot].w.humidity_ok = true;
        sensor[slot].w.wind_ok = true;
        sensor[slot].w.rain_ok = true;
        sensor[slot].w.light_ok = true;
        sensor[slot].w.uv_ok = true;
        sensor[slot].w.temp_c = temp_c;
        sensor[slot].w.humidity = humidity;
#ifdef WIND_DATA_FLOATINGPOINT
        sensor[slot].w.wind_gust_meter_sec = wgst_raw * 0.1f;
        sensor[slot].w.wind_avg_meter_sec = wavg_raw * 0.1f;
        sensor[slot].w.wind_direction_deg = wdir * 1.0f;
#endif
#ifdef WIND_DATA_FIXEDPOINT
        sensor[slot].w.wind_gust_meter_sec_fp1 = wgst_raw;
        sensor[slot].w.wind_avg_meter_sec_fp1 = wavg_raw;
        sensor[slot].w.wind_direction_deg_fp1 = wdir * 10;
#endif
        sensor[slot].w.rain_mm = rain_mm;
        sensor[slot].w.light_klx = light_klx;
        sensor[slot].w.light_lux = light_lux;
        sensor[slot].w.uv = uv_index;
    }
    else if (s_type == SENSOR_TYPE_AIR_PM)
    {
#if CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
        uint16_t pn1 = (msgw[14] & 0x0f) * 1000 + (msgw[15] >> 4) * 100 + (msgw[15] & 0x0f) * 10 + (msgw[16] >> 4);
        uint16_t pn2 = (msgw[17] >> 4) * 100 + (msgw[17] & 0x0f) * 10 + (msgw[18] >> 4);
        uint16_t pn3 = (msgw[19] >> 4) * 100 + (msgw[19] & 0x0f) * 10 + (msgw[20] >> 4);
#endif
        log_d("PN1: %04d PN2: %04d PN3: %04d", pn1, pn2, pn3);
        sensor[slot].pm.pm_1_0 = (msgw[8] & 0x0f) * 1000 + (msgw[9] >> 4) * 100 + (msgw[9] & 0x0f) * 10 + (msgw[10] >> 4);
        sensor[slot].pm.pm_2_5 = (msgw[10] & 0x0f) * 1000 + (msgw[11] >> 4) * 100 + (msgw[11] & 0x0f) * 10 + (msgw[12] >> 4);
        sensor[slot].pm.pm_10 = (msgw[12] & 0x0f) * 1000 + (msgw[13] >> 4) * 100 + (msgw[13] & 0x0f) * 10 + (msgw[14] >> 4);
        sensor[slot].pm.pm_1_0_init = ((msgw[10] >> 4) & 0x0f) == 0x0f;
        sensor[slot].pm.pm_2_5_init = ((msgw[12] >> 4) & 0x0f) == 0x0f;
        sensor[slot].pm.pm_10_init = ((msgw[14] >> 4) & 0x0f) == 0x0f;
    }
    else if (s_type == SENSOR_TYPE_CO2)
    {
        sensor[slot].co2.co2_ppm = ((msgw[4] & 0xf0) >> 4) * 1000 + (msgw[4] & 0x0f) * 100 + ((msgw[5] & 0xf0) >> 4) * 10 + (msgw[5] & 0x0f);
        sensor[slot].co2.co2_init = (msgw[5] & 0x0f) == 0x0f;
    }
    else if (s_type == SENSOR_TYPE_HCHO_VOC)
    {
        sensor[slot].voc.hcho_ppb = ((msgw[4] & 0xf0) >> 4) * 1000 + (msgw[4] & 0x0f) * 100 + ((msgw[5] & 0xf0) >> 4) * 10 + (msgw[5] & 0x0f);
        sensor[slot].voc.voc_level = (msgw[22] & 0x0f);
        sensor[slot].voc.hcho_init = (msgw[5] & 0x0f) == 0x0f;
        sensor[slot].voc.voc_init = msgw[22] == 0x0f;
    }

    return DECODE_OK;
}
#endif

/**
Decoder for Bresser Lightning, outdoor sensor.

https://github.com/merbanan/rtl_433/issues/2140

DIGEST:8h8h ID:8h8h CTR:12h   ?4h8h KM:8d ?8h8h
       0 1     2 3      4 5h   5l 6    7   8 9

Preamble:

  aa 2d d4

Observed length depends on reset_limit.
The data has a whitening of 0xaa.


First two bytes are an LFSR-16 digest, generator 0x8810 key 0xabf9 with a final xor 0x899e
*/

#ifdef BRESSER_LIGHTNING
DecodeStatus WeatherSensor::decodeBresserLightningPayload(const uint8_t *msg, uint8_t msgSize)
{
    (void)msgSize;
#if CORE_DEBUG_LEVEL == ARDUHAL_LOG_LEVEL_VERBOSE
    // see AS3935 Datasheet, Table 17 - Distance Estimation
    uint8_t const distance_map[] = {1, 5, 6, 8, 10, 12, 14, 17, 20, 24, 27, 31, 34, 37, 40, 63};
#endif

#if defined(LIGHTNING_TEST_DATA)
    uint8_t test_data[] = {0x73, 0x69, 0xB5, 0x08, 0xAA, 0xA2, 0x90, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x15};
#endif

    // data de-whitening
    uint8_t msgw[MSG_BUF_SIZE];
    for (unsigned i = 0; i < msgSize; ++i)
    {
#if defined(LIGHTNING_TEST_DATA)
        msgw[i] = test_data[i] ^ 0xaa;
#else
        msgw[i] = msg[i] ^ 0xaa;
#endif
    }

    // LFSR-16 digest, generator 0x8810 key 0xabf9 with a final xor 0x899e
    int chk = (msgw[0] << 8) | msgw[1];
    int digest = lfsr_digest16(&msgw[2], 8, 0x8810, 0xabf9);
    if (((chk ^ digest) != 0x899e))
    {
        log_d("Digest check failed - [%04X] vs [%04X] (%04X)", chk, digest, chk ^ digest);
        return DECODE_DIG_ERR;
    }

#if CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
    log_message("            Data", msg, msgSize);
    log_message("De-whitened Data", msgw, msgSize);
#endif

    int id_tmp = (msgw[2] << 8) | (msgw[3]);
    int s_type = msg[6] >> 4;
    int startup = (msg[6] & 0x8) == 0x00;

    DecodeStatus status;

    // Find appropriate slot in sensor data array and update <status>
    int slot = findSlot(id_tmp, &status);

    if (status != DECODE_OK)
        return status;

    // Counter encoded as BCD with most significant digit counting up to 15!
    // -> Maximum value: 1599
    uint16_t ctr = (msgw[4] >> 4) * 100 + (msgw[4] & 0xf) * 10 + (msgw[5] >> 4);
    uint8_t battery_low = (msgw[5] & 0x08) == 0x00;
    uint16_t unknown1 = ((msgw[5] & 0x0f) << 8) | msgw[6];
    uint8_t distance_km = msgw[7];
    log_v("--> DST RAW: %d  BCD: %d  TAB: %d", msgw[7], ((((msgw[7] & 0xf0) >> 4) * 10) + (msgw[7] & 0x0f)), distance_map[msgw[7]]);
    uint16_t unknown2 = (msgw[8] << 8) | msgw[9];

    sensor[slot].sensor_id = id_tmp;
    sensor[slot].s_type = s_type;
    sensor[slot].startup = startup;
    sensor[slot].chan = 0;
    sensor[slot].battery_ok = !battery_low;
    sensor[slot].rssi = rssi;
    sensor[slot].valid = true;
    sensor[slot].complete = true;

    sensor[slot].lgt.strike_count = ctr;
    sensor[slot].lgt.distance_km = distance_km;
    sensor[slot].lgt.unknown1 = unknown1;
    sensor[slot].lgt.unknown2 = unknown2;

    log_d("ID: 0x%04X  TYPE: %d  CTR: %u  batt_low: %d  distance_km: %d  unknown1: 0x%x  unknown2: 0x%04x", id_tmp, s_type, ctr, battery_low, distance_km, unknown1, unknown2);

    return DECODE_OK;
}
#endif

/**
 * Decoder for Bresser Water Leakage outdoor sensor
 *
 * https://github.com/matthias-bs/BresserWeatherSensorReceiver/issues/77
 *
 * Preamble: aa aa 2d d4
 *
 * hhhh ID:hhhhhhhh TYPE:4d NSTARTUP:b CH:3d ALARM:b NALARM:b BATT:bb FLAGS:bbbb hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh
 *
 * Examples:
 * ---------
 * [Bresser Water Leakage Sensor, PN 7009975]
 *
 *[00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25]
 *
 * C7 70 35 97 04 08 57 70 00 00 00 00 00 00 00 00 03 FF FF FF FF FF FF FF FF FF [CH7]
 * DF 7D 36 49 27 09 56 70 00 00 00 00 00 00 00 00 03 FF FF FF FF FF FF FF FF FF [CH6]
 * 9E 30 79 84 33 06 55 70 00 00 00 00 00 00 00 00 03 FF FD DF FF BF FF DF FF FF [CH5]
 * 37 D8 57 19 73 02 51 70 00 00 00 00 00 00 00 00 03 FF FF FF FF FF BF FF EF FB [set CH4, received CH1 -> switch not positioned correctly]
 * E2 C8 68 27 91 24 54 70 00 00 00 00 00 00 00 00 03 FF FF FF FF FF FF FF FF FF [CH4]
 * B3 DA 55 57 17 40 53 70 00 00 00 00 00 00 00 00 03 FF FF FF FF FF FF FF FF FB [CH3]
 * 37 FA 84 73 03 02 52 70 00 00 00 00 00 00 00 00 03 FF FF FF DF FF FF FF FF FF [CH2]
 * 27 F3 80 02 52 88 51 70 00 00 00 00 00 00 00 00 03 FF FF FF FF FF DF FF FF FF [CH1]
 * A6 FB 80 02 52 88 59 70 00 00 00 00 00 00 00 00 03 FD F7 FF FF BF FF FF FF FF [CH1+NSTARTUP]
 * A6 FB 80 02 52 88 59 B0 00 00 00 00 00 00 00 00 03 FF FF FF FD FF F7 FF FF FF [CH1+NSTARTUP+ALARM]
 * A6 FB 80 02 52 88 59 70 00 00 00 00 00 00 00 00 03 FF FF BF F7 F7 FD 7F FF FF [CH1+NSTARTUP]
 * [Reset]
 * C0 10 36 79 37 09 51 70 00 00 00 00 00 00 00 00 01 1E FD FD FF FF FF DF FF FF [CH1]
 * C0 10 36 79 37 09 51 B0 00 00 00 00 00 00 00 00 03 FE FD FF AF FF FF FF FF FD [CH1+ALARM]
 * [Reset]
 * 71 9C 54 81 72 09 51 40 00 00 00 00 00 00 00 00 0F FF FF FF FF FF FF DF FF FE [CH1+BATT_LO]
 * 71 9C 54 81 72 09 51 40 00 00 00 00 00 00 00 00 0F FE FF FF FF FF FB FF FF FF
 * 71 9C 54 81 72 09 51 40 00 00 00 00 00 00 00 00 07 FD F7 FF DF FF FF DF FF FF
 * 71 9C 54 81 72 09 51 80 00 00 00 00 00 00 00 00 1F FF FF F7 FF FF FF FF FF FF [CH1+BATT_LO+ALARM]
 * F0 94 54 81 72 09 59 40 00 00 00 00 00 00 00 00 0F FF DF FF FF FF FF BF FD F7 [CH1+BATT_LO+NSTARTUP]
 * F0 94 54 81 72 09 59 80 00 00 00 00 00 00 00 00 03 FF B7 FF ED FF FF FF DF FF [CH1+BATT_LO+NSTARTUP+ALARM]
 *
 * - The actual message length is not known (probably 16 or 17 bytes)
 * - The first two bytes are presumably a checksum/crc/digest; algorithm still to be found
 * - The ID changes on power-up/reset
 * - NSTARTUP changes from 0 to 1 approx. one hour after power-on/reset
 */
#ifdef BRESSER_LEAKAGE
DecodeStatus WeatherSensor::decodeBresserLeakagePayload(const uint8_t *msg, uint8_t msgSize)
{
#if CORE_DEBUG_LEVEL == ARDUHAL_LOG_LEVEL_VERBOSE
    log_message("Data", msg, msgSize);
#else
    (void)msgSize;
#endif

    // Verify CRC (CRC16/XMODEM)
    uint16_t crc_act = crc16(&msg[2], 5, 0x1021, 0x0000);
    uint16_t crc_exp = (msg[0] << 8) | msg[1];
    if (crc_act != crc_exp)
    {
        log_d("CRC16 check failed - [%04X] vs [%04X]", crc_act, crc_exp);
        return DECODE_CHK_ERR;
    }

    uint32_t id_tmp = ((uint32_t)msg[2] << 24) | (msg[3] << 16) | (msg[4] << 8) | (msg[5]);
    uint8_t type_tmp = msg[6] >> 4;
    uint8_t chan_tmp = (msg[6] & 0x7);
    bool alarm = (msg[7] & 0x80) == 0x80;
    bool no_alarm = (msg[7] & 0x40) == 0x40;

    // Sanity checks
    bool decode_ok = (type_tmp == SENSOR_TYPE_LEAKAGE) &&
                     (alarm != no_alarm) &&
                     (chan_tmp != 0);

    if (!decode_ok)
        return DECODE_INVALID;

    DecodeStatus status = DECODE_OK;

    // Find appropriate slot in sensor data array and update <status>
    int slot = findSlot(id_tmp, &status);

    if (status != DECODE_OK)
        return status;

    sensor[slot].sensor_id = id_tmp;
    sensor[slot].s_type = type_tmp;
    sensor[slot].chan = chan_tmp;
    sensor[slot].startup = (msg[6] & 0x8) == 0x00;
    sensor[slot].battery_ok = (msg[7] & 0x30) != 0x00;
    sensor[slot].rssi = rssi;
    sensor[slot].valid = true;
    sensor[slot].complete = true;
    sensor[slot].leak.alarm = (alarm && !no_alarm);

    log_d("ID: 0x%08X  CH: %d  TYPE: %d  batt_ok: %d  startup: %d, alarm: %d no_alarm: %d",
          (unsigned int)id_tmp, chan_tmp, type_tmp, sensor[slot].battery_ok, sensor[slot].startup ? 1 : 0, alarm ? 1 : 0, no_alarm ? 1 : 0);

    return DECODE_OK;
}
#endif
