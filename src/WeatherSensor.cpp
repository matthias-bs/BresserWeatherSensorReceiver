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


// List of sensor IDs to be excluded - can be empty
uint32_t const sensor_ids_exc[] = SENSOR_IDS_EXC;

// List of sensor IDs to be included - if empty, handle all available sensors
uint32_t const sensor_ids_inc[] = SENSOR_IDS_INC;

int16_t WeatherSensor::begin(void) {
    // https://github.com/RFD-FHEM/RFFHEM/issues/607#issuecomment-830818445
    // Freq: 868.300 MHz, Bandwidth: 203 KHz, rAmpl: 33 dB, sens: 8 dB, DataRate: 8207.32 Baud
    log_d("%s Initializing ... ", RECEIVER_CHIP);
    // carrier frequency:                   868.3 MHz
    // bit rate:                            8.22 kbps
    // frequency deviation:                 57.136417 kHz
    // Rx bandwidth:                        270.0 kHz (CC1101) / 250 kHz (SX1276)
    // output power:                        10 dBm
    // preamble length:                     40 bits
    #ifdef USE_CC1101
        int state = radio.begin(868.3, 8.21, 57.136417, 270, 10, 32);
    #else
        int state = radio.beginFSK(868.3, 8.21, 57.136417, 250, 10, 32);
    #endif
    if (state == RADIOLIB_ERR_NONE) {
        log_d("success!");
        state = radio.setCrcFiltering(false);
        if (state != RADIOLIB_ERR_NONE) {
            log_e("%s Error disabling crc filtering: [%d]", RECEIVER_CHIP, state);
            while (true)
                ;
        }
        state = radio.fixedPacketLengthMode(MSG_BUF_SIZE);
        if (state != RADIOLIB_ERR_NONE) {
            log_e("%s Error setting fixed packet length: [%d]", RECEIVER_CHIP, state);
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
        if (state != RADIOLIB_ERR_NONE) {
            log_e("%s Error setting sync words: [%d]", RECEIVER_CHIP, state);
            while (true)
                ;
        }
    } else {
        log_e("%s Error initialising: [%d]", RECEIVER_CHIP, state);
        while (true)
            ;
    }
    log_d("%s Setup complete - awaiting incoming messages...", RECEIVER_CHIP);
    rssi = radio.getRSSI();

    return state;
}


bool WeatherSensor::getData(uint32_t timeout, uint8_t flags, uint8_t type, void (*func)())
{
    const uint32_t timestamp = millis();


    while ((millis() - timestamp) < timeout) {
        int decode_status = getMessage();

        // Callback function (see https://www.geeksforgeeks.org/callbacks-in-c/)
        if (func) {
            (*func)();
        }

        if (decode_status == DECODE_OK) {
            bool all_slots_valid    = true;
            bool all_slots_complete = true;
            for (int i=0; i<NUM_SENSORS; i++) {
                if (!sensor[i].valid) {
                    all_slots_valid = false;
                    continue;
                }

                // No special requirements, one valid message is sufficient
                if (flags == 0)
                    return true;

                // Specific sensor type required
                if (((flags & DATA_TYPE) != 0) && (sensor[i].s_type == type)) {
                    if (sensor[i].complete || !(flags & DATA_COMPLETE)) {
                        return true;
                    }
                }
                // All slots required (valid AND complete) - must check all slots
                else if (flags & DATA_ALL_SLOTS) {
                    all_slots_valid    &= sensor[i].valid;
                    all_slots_complete &= sensor[i].complete;
                }
                // At least one sensor valid and complete
                else if (sensor[i].complete) {
                    return true;
                }
            } // for (int i=0; i<NUM_SENSORS; i++)

            // All slots required (valid AND complete)
            if ((flags & DATA_ALL_SLOTS) && all_slots_valid && all_slots_complete) {
                return true;
            }

        } // if (decode_status == DECODE_OK)
    } //  while ((millis() - timestamp) < timeout)


    // Timeout
    return false;
}


DecodeStatus WeatherSensor::getMessage(void)
{
    uint8_t         recvData[MSG_BUF_SIZE];
    DecodeStatus    decode_res = DECODE_INVALID;

    // Receive data
    //     1. flush RX buffer
    //     2. switch to RX mode
    //     3. wait for expected RX packet or timeout [~500us in this configuration]
    //     4. flush RX buffer
    //     5. switch to standby
    int state = radio.receive(recvData, MSG_BUF_SIZE);
    rssi = radio.getRSSI();

    if (state == RADIOLIB_ERR_NONE) {
        // Verify last syncword is 1st byte of payload (see setSyncWord() above)
        if (recvData[0] == 0xD4) {
            #if CORE_DEBUG_LEVEL == ARDUHAL_LOG_LEVEL_VERBOSE
                char buf[128];
                *buf = '\0';
                for(size_t i = 0 ; i < sizeof(recvData) ; i++) {
                    sprintf(&buf[strlen(buf)], "%02X ", recvData[i]);
                }
                log_v("%s Data: %s", RECEIVER_CHIP, buf);
            #endif
            log_d("%s R [%02X] RSSI: %0.1f", RECEIVER_CHIP, recvData[0], rssi);

            #ifdef BRESSER_7_IN_1
                decode_res = decodeBresser7In1Payload(&recvData[1], sizeof(recvData) - 1);
            #endif
            #ifdef BRESSER_6_IN_1
                if (decode_res == DECODE_INVALID ||
                    decode_res == DECODE_PAR_ERR ||
                    decode_res == DECODE_CHK_ERR ||
                    decode_res == DECODE_DIG_ERR) {
                    decode_res = decodeBresser6In1Payload(&recvData[1], sizeof(recvData) - 1);
                }
            #endif
            #ifdef BRESSER_5_IN_1
                if (decode_res == DECODE_INVALID ||
                    decode_res == DECODE_PAR_ERR ||
                    decode_res == DECODE_CHK_ERR ||
                    decode_res == DECODE_DIG_ERR) {
                    decode_res = decodeBresser5In1Payload(&recvData[1], sizeof(recvData) - 1);
                }
            #endif
        } // if (recvData[0] == 0xD4)
        else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
            log_v("T");
        } // if (state == RADIOLIB_ERR_RX_TIMEOUT)
        else {
            // some other error occurred
            log_d("%s Receive failed: [%d]", RECEIVER_CHIP, state);
        }
    } // if (state == RADIOLIB_ERR_NONE)

    return decode_res;
}

//
// Generate sample data for testing
//
bool WeatherSensor::genMessage(int i, uint32_t id, uint8_t type, uint8_t channel)
{
    sensor[i].sensor_id               = id;
    sensor[i].s_type                  = type;
    sensor[i].chan                    = channel;
    sensor[i].temp_ok                 = true;
    sensor[i].temp_c                  = 22.2f;
    sensor[i].humidity_ok             = true;
    sensor[i].humidity                = 55;
#ifdef WIND_DATA_FLOATINGPOINT
    sensor[i].wind_direction_deg      = 111.1;
    sensor[i].wind_gust_meter_sec     = 4.4f;
    sensor[i].wind_avg_meter_sec      = 3.3f;
#endif
#ifdef WIND_DATA_FIXEDPOINT
    sensor[i].wind_direction_deg_fp1  = 1111;
    sensor[i].wind_gust_meter_sec_fp1 = 44;
    sensor[i].wind_avg_meter_sec_fp1  = 33;
#endif
    sensor[i].wind_ok                 = true;
    sensor[i].rain_mm                 = 9.9f;
    sensor[i].battery_ok              = true;
    sensor[i].rssi                    = 88.8;

    sensor[i].valid                   = true;
    sensor[i].complete                = true;
    return true;
}


//
// Find slot in sensor data array
//
int WeatherSensor::findSlot(uint32_t id, DecodeStatus * status)
{

    log_v("find_slot(): ID=%08X", id);

    // Skip sensors from exclude-list (if any)
    uint8_t n_exc = sizeof(sensor_ids_exc)/4;
    if (n_exc != 0) {
       for (int i=0; i<n_exc; i++) {
           if (id == sensor_ids_exc[i]) {
               log_v("In Exclude-List, skipping!");
               *status = DECODE_SKIP;
               return -1;
           }
       }
    }

    // Handle sensors from include-list (if not empty)
    uint8_t n_inc = sizeof(sensor_ids_inc)/4;
    if (n_inc != 0) {
        bool found = false;
        for (int i=0; i<n_inc; i++) {
            if (id == sensor_ids_inc[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            log_v("Not in Include-List, skipping!");
            *status = DECODE_SKIP;
            return -1;
        }
    }

    // Search all slots
    int free_slot   = -1;
    int update_slot = -1;
    for (int i=0; i<NUM_SENSORS; i++) {
        // Save first free slot
        if (!sensor[i].valid && (free_slot < 0)) {
            free_slot = i;
        }

        // Check if sensor has already been stored
        else if (sensor[i].valid && (sensor[i].sensor_id == id)) {
            update_slot = i;
        }
    }

    if (update_slot > -1) {
        // Update slot
        log_v("find_slot(): Updating slot #%d", update_slot);
        *status = DECODE_OK;
        return update_slot;
    }
    else if (free_slot > -1) {
        // Store to free slot
        log_v("find_slot(): Storing into slot #%d", free_slot);
        *status = DECODE_OK;
        return free_slot;
    }
    else {
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
    for (int i=0; i<NUM_SENSORS; i++) {
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
    for (int i=0; i<NUM_SENSORS; i++) {
        if (sensor[i].valid && (sensor[i].s_type == type) &&
            ((ch == 0xFF) || (sensor[i].chan = ch)))
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
    for (unsigned k = 0; k < bytes; ++k) {
        uint8_t data = message[k];
        for (int i = 7; i >= 0; --i) {
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
    for (unsigned i = 0; i < num_bytes; ++i) {
        result += message[i];
    }
    return result;
}


//
// From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_5in1.c (20220212)
//
// Example input data:
//   EA EC 7F EB 5F EE EF FA FE 76 BB FA FF 15 13 80 14 A0 11 10 05 01 89 44 05 00
//   CC CC CC CC CC CC CC CC CC CC CC CC CC uu II SS GG DG WW  W TT  T HH RR RR Bt
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
DecodeStatus WeatherSensor::decodeBresser5In1Payload(uint8_t *msg, uint8_t msgSize) {
    // First 13 bytes need to match inverse of last 13 bytes
    for (unsigned col = 0; col < msgSize / 2; ++col) {
        if ((msg[col] ^ msg[col + 13]) != 0xff) {
            log_d("Parity wrong at column %d", col);
            return DECODE_PAR_ERR;
        }
    }

    // Verify checksum (number number bits set in bytes 14-25)
    uint8_t bitsSet = 0;
    uint8_t expectedBitsSet = msg[13];

    for(uint8_t p = 14 ; p < msgSize ; p++) {
      uint8_t currentByte = msg[p];
      while(currentByte) {
        bitsSet += (currentByte & 1);
        currentByte >>= 1;
      }
    }

    if (bitsSet != expectedBitsSet) {
        log_d("Checksum wrong - actual [%02X] != [%02X]", bitsSet, expectedBitsSet);
        return DECODE_CHK_ERR;
    }

    uint8_t id_tmp   = msg[14];
    uint8_t type_tmp = msg[15] & 0xF;
    DecodeStatus status;

    // Find appropriate slot in sensor data array and update <status>
    int slot = findSlot(id_tmp, &status);

    if (status != DECODE_OK)
        return status;

    sensor[slot].sensor_id   = id_tmp;
    sensor[slot].s_type      = type_tmp;

    int temp_raw = (msg[20] & 0x0f) + ((msg[20] & 0xf0) >> 4) * 10 + (msg[21] &0x0f) * 100;
    if (msg[25] & 0x0f) {
        temp_raw = -temp_raw;
    }
    sensor[slot].temp_c = temp_raw * 0.1f;

    sensor[slot].humidity = (msg[22] & 0x0f) + ((msg[22] & 0xf0) >> 4) * 10;

    int wind_direction_raw = ((msg[17] & 0xf0) >> 4) * 225;
    int gust_raw = ((msg[17] & 0x0f) << 8) + msg[16];
    int wind_raw = (msg[18] & 0x0f) + ((msg[18] & 0xf0) >> 4) * 10 + (msg[19] & 0x0f) * 100;


#ifdef WIND_DATA_FLOATINGPOINT
    sensor[slot].wind_direction_deg      = wind_direction_raw * 0.1f;
    sensor[slot].wind_gust_meter_sec     = gust_raw * 0.1f;
    sensor[slot].wind_avg_meter_sec      = wind_raw * 0.1f;
#endif
#ifdef WIND_DATA_FIXEDPOINT
    sensor[slot].wind_direction_deg_fp1  = wind_direction_raw;
    sensor[slot].wind_gust_meter_sec_fp1 = gust_raw;
    sensor[slot].wind_avg_meter_sec_fp1  = wind_raw;
#endif

    int rain_raw = (msg[23] & 0x0f) + ((msg[23] & 0xf0) >> 4) * 10 + (msg[24] & 0x0f) * 100 + ((msg[24] & 0xf0) >> 4) * 1000;
    sensor[slot].rain_mm = rain_raw * 0.1f;

    sensor[slot].battery_ok = (msg[25] & 0x80) ? false : true;

    /* check if the message is from a Bresser Professional Rain Gauge */
    if ((msg[15] & 0xF) == 0x9) {
        // rescale the rain sensor readings
        sensor[slot].rain_mm *= 2.5;

        // Rain Gauge has no humidity (according to description) and no wind sensor (obviously)
        sensor[slot].humidity_ok = false;
        sensor[slot].wind_ok     = false;

    }
    else {
        sensor[slot].humidity_ok = true;
        sensor[slot].wind_ok     = true;
    }

    sensor[slot].temp_ok      = true;
    sensor[slot].light_ok     = false;
    sensor[slot].uv_ok        = false;
    sensor[slot].rain_ok      = true;
    sensor[slot].moisture_ok  = false;
    sensor[slot].valid        = true;
    sensor[slot].complete     = true;

    // Save rssi to sensor specific data set
    sensor[slot].rssi = rssi;

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
//     DIGEST:8h8h ID?8h8h8h8h STYPE:4h STARTUP:1b CH:3d 8h 8h8h 8h8h TEMP:12h ?2b BATT:1b ?1b MOIST:8h UV?~12h ?4h CHKSUM:8h
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
//     DIGEST:8h8h ID:8h8h8h8h STYPE:4h STARTUP:1b CH:3d WSPEED:~8h~4h ~4h~8h WDIR:12h ?4h TEMP:8h.4h ?2b BATT:1b ?1b HUM:8h UV?~12h ?4h CHKSUM:8h
//     DIGEST:8h8h ID:8h8h8h8h STYPE:4h STARTUP:1b CH:3d WSPEED:~8h~4h ~4h~8h WDIR:12h ?4h RAINFLAG:8h RAIN:8h8h UV:8h8h CHKSUM:8h
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
DecodeStatus WeatherSensor::decodeBresser6In1Payload(uint8_t *msg, uint8_t msgSize) {
    (void)msgSize; // unused parameter - kept for consistency with other decoders; avoid warning
    int const moisture_map[] = {0, 7, 13, 20, 27, 33, 40, 47, 53, 60, 67, 73, 80, 87, 93, 99}; // scale is 20/3

    // Per-message status flags
    bool temp_ok     = false;
    bool humidity_ok = false;
    bool uv_ok       = false;
    bool wind_ok     = false;
    bool rain_ok     = false;
    bool moisture_ok = false;

    // LFSR-16 digest, generator 0x8810 init 0x5412
    int chkdgst = (msg[0] << 8) | msg[1];
    int digest  = lfsr_digest16(&msg[2], 15, 0x8810, 0x5412);
    if (chkdgst != digest) {
        log_d("Digest check failed - [%02X] != [%02X]", chkdgst, digest);
        return DECODE_DIG_ERR;
    }
    // Checksum, add with carry
    int sum    = add_bytes(&msg[2], 16); // msg[2] to msg[17]
    if ((sum & 0xff) != 0xff) {
        log_d("Checksum failed");
        return DECODE_CHK_ERR;
    }

    uint32_t id_tmp   = ((uint32_t)msg[2] << 24) | (msg[3] << 16) | (msg[4] << 8) | (msg[5]);
    uint8_t  type_tmp = (msg[6] >> 4); // 1: weather station, 2: indoor?, 4: soil probe
    uint8_t  chan_tmp = (msg[6] & 0x7);
    uint8_t  flags    = (msg[16] & 0x0f);
    DecodeStatus status;

    // Find appropriate slot in sensor data array and update <status>
    int slot = findSlot(id_tmp, &status);

    if (status != DECODE_OK)
        return status;

    // unused...
    //int startup = (msg[6] >> 3) & 1; // s.a. #1214

    sensor[slot].sensor_id   = id_tmp;
    sensor[slot].s_type      = type_tmp;
    sensor[slot].chan        = chan_tmp;


    // temperature, humidity(, uv) - shared with rain counter
    temp_ok = humidity_ok = (flags == 0);
    if (temp_ok) {
        bool  sign     = (msg[13] >> 3) & 1;
        int   temp_raw = (msg[12] >> 4) * 100 + (msg[12] & 0x0f) * 10 + (msg[13] >> 4);
        float temp     = ((sign) ? (temp_raw - 1000) : temp_raw) * 0.1f;

        sensor[slot].temp_c      = temp;
        sensor[slot].battery_ok  = (msg[13] >> 1) & 1; // b[13] & 0x02 is battery_good, s.a. #1993
        sensor[slot].humidity    = (msg[14] >> 4) * 10 + (msg[14] & 0x0f);

        // apparently ff01 or 0000 if not available, ???0 if valid, inverted BCD
        uv_ok  = (~msg[15] & 0xff) <= 0x99 && (~msg[16] & 0xf0) <= 0x90;
        if (uv_ok) {
            int uv_raw    = ((~msg[15] & 0xf0) >> 4) * 100 + (~msg[15] & 0x0f) * 10 + ((~msg[16] & 0xf0) >> 4);
                sensor[slot].uv = uv_raw * 0.1f;
        }
    }

    //int unk_ok  = (msg[16] & 0xf0) == 0xf0;
    //int unk_raw = ((msg[15] & 0xf0) >> 4) * 10 + (msg[15] & 0x0f);

    // invert 3 bytes wind speeds
    msg[7] ^= 0xff;
    msg[8] ^= 0xff;
    msg[9] ^= 0xff;
    wind_ok = (msg[7] <= 0x99) && (msg[8] <= 0x99) && (msg[9] <= 0x99);
    if (wind_ok) {
        int gust_raw     = (msg[7] >> 4) * 100 + (msg[7] & 0x0f) * 10 + (msg[8] >> 4);
        int wavg_raw     = (msg[9] >> 4) * 100 + (msg[9] & 0x0f) * 10 + (msg[8] & 0x0f);
        int wind_dir_raw = ((msg[10] & 0xf0) >> 4) * 100 + (msg[10] & 0x0f) * 10 + ((msg[11] & 0xf0) >> 4);

#ifdef WIND_DATA_FLOATINGPOINT
        sensor[slot].wind_gust_meter_sec     = gust_raw * 0.1f;
        sensor[slot].wind_avg_meter_sec      = wavg_raw * 0.1f;
        sensor[slot].wind_direction_deg      = wind_dir_raw * 1.0f;
#endif
#ifdef WIND_DATA_FIXEDPOINT
        sensor[slot].wind_gust_meter_sec_fp1 = gust_raw;
        sensor[slot].wind_avg_meter_sec_fp1  = wavg_raw;
        sensor[slot].wind_direction_deg_fp1  = wind_dir_raw * 10;
#endif
    }

    // rain counter, inverted 3 bytes BCD - shared with temp/hum
    msg[12] ^= 0xff;
    msg[13] ^= 0xff;
    msg[14] ^= 0xff;

    rain_ok = (flags == 1) && (type_tmp == 1);
    if (rain_ok) {
        int rain_raw     = (msg[12] >> 4) * 100000 + (msg[12] & 0x0f) * 10000
                         + (msg[13] >> 4) * 1000 + (msg[13] & 0x0f) * 100
                         + (msg[14] >> 4) * 10 + (msg[14] & 0x0f);
        sensor[slot].rain_mm   = rain_raw * 0.1f;
    }

    moisture_ok = false;

    // the moisture sensor might present valid readings but does not have the hardware
    if (sensor[slot].s_type == 4) {
        wind_ok = 0;
        uv_ok   = 0;
    }

    if (sensor[slot].s_type == 4 && temp_ok && sensor[slot].humidity >= 1 && sensor[slot].humidity <= 16) {
        moisture_ok = true;
        humidity_ok = false;
        sensor[slot].moisture = moisture_map[sensor[slot].humidity - 1];
    }

    // Update per-slot status flags
    sensor[slot].temp_ok     |= temp_ok;
    sensor[slot].humidity_ok |= humidity_ok;
    sensor[slot].uv_ok       |= uv_ok;
    sensor[slot].wind_ok     |= wind_ok;
    sensor[slot].rain_ok     |= rain_ok;
    sensor[slot].moisture_ok |= moisture_ok;
    log_d("Temp: %d  Hum: %d  UV: %d  Wind: %d  Rain: %d  Moist: %d", temp_ok, humidity_ok, uv_ok, wind_ok, rain_ok, moisture_ok);

    sensor[slot].valid    = true;

    // Weather station data is split into two separate messages
    sensor[slot].complete = ((sensor[slot].s_type == SENSOR_TYPE_WEATHER1) && sensor[slot].temp_ok && sensor[slot].rain_ok) || (sensor[slot].s_type != SENSOR_TYPE_WEATHER1);

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
The data has a whitening of 0xaa.
Data layout:
    {271}631d05c09e9a18abaabaaaaaaaaa8adacbacff9cafcaaaaaaa000000000000000000
    {262}10b8b4a5a3ca10aaaaaaaaaaaaaa8bcacbaaaa2aaaaaaaaaaa0000000000000000 [0.08 klx]
    {220}543bb4a5a3ca10aaaaaaaaaaaaaa8bcacbaaaa28aaaaaaaaaa00000 [0.08 klx]
    {273}2492b4a5a3ca10aaaaaaaaaaaaaa8bdacbaaaa2daaaaaaaaaa0000000000000000000 [0.08klx]
    {269}9a59b4a5a3da10aaaaaaaaaaaaaa8bdac8afea28a8caaaaaaa000000000000000000 [54.0 klx UV=2.6]
    {230}fe15b4a5a3da10aaaaaaaaaaaaaa8bdacbba382aacdaaaaaaa00000000 [109.2klx   UV=6.7]
    {254}2544b4a5a32a10aaaaaaaaaaaaaa8bdac88aaaaabeaaaaaaaa00000000000000 [200.000 klx UV=14
    DIGEST:8h8h ID?8h8h WDIR:8h4h 4h 8h WGUST:8h.4h WAVG:8h.4h RAIN:8h8h4h.4h RAIN?:8h TEMP:8h.4hC FLAGS?:4h HUM:8h% LIGHT:8h4h,8h4hKL UV:8h.4h TRAILER:8h8h8h4h
Unit of light is kLux (not W/m²).
First two bytes are an LFSR-16 digest, generator 0x8810 key 0xba95 with a final xor 0x6df1, which likely means we got that wrong.
*/
#ifdef BRESSER_7_IN_1
DecodeStatus WeatherSensor::decodeBresser7In1Payload(uint8_t *msg, uint8_t msgSize) {

  if (msg[21] == 0x00) {
      log_e("DECODE_FAIL_SANITY !!!");
  }
  
  // data whitening
  uint8_t msgw[MSG_BUF_SIZE];
  for (unsigned i = 0; i < msgSize; ++i) {
      msgw[i] = msg[i] ^ 0xaa;
  }

  // LFSR-16 digest, generator 0x8810 key 0xba95 final xor 0x6df1
  int chkdgst = (msgw[0] << 8) | msgw[1];
  int digest  = lfsr_digest16(&msgw[2], 23, 0x8810, 0xba95); // bresser_7in1
  if ((chkdgst ^ digest) != 0x6df1) { // bresser_7in1
      log_d("Digest check failed - [%02X] Vs [%02X] (%02X)", chkdgst, digest, chkdgst ^ digest); // bresser_7in1
      return DECODE_DIG_ERR;
  }

  int id_tmp   = (msgw[2] << 8) | (msgw[3]);
  DecodeStatus status;

  // Find appropriate slot in sensor data array and update <status>
  int slot = findSlot(id_tmp, &status);

  if (status != DECODE_OK)
      return status;

  int wdir     = (msgw[4] >> 4) * 100 + (msgw[4] & 0x0f) * 10 + (msgw[5] >> 4);
  int wgst_raw = (msgw[7] >> 4) * 100 + (msgw[7] & 0x0f) * 10 + (msgw[8] >> 4);
  int wavg_raw = (msgw[8] & 0x0f) * 100 + (msgw[9] >> 4) * 10 + (msgw[9] & 0x0f);
  //int rain_raw = (msgw[10] >> 4) * 100000 + (msgw[10] & 0x0f) * 10000 + (msgw[11] >> 4) * 1000 + (msgw[11] & 0x0f) * 100 + (msgw[12] >> 4) * 10 + (msgw[12] & 0x0f) * 1; // 6 digits
  int rain_raw = (msgw[10] >> 4) * 1000 + (msgw[10] & 0x0f) * 100 + (msgw[11] >> 4) * 10 + (msgw[11] & 0x0f) * 1; // 4 digits
  float rain_mm = rain_raw * 0.1f;
  int temp_raw = (msgw[14] >> 4) * 100 + (msgw[14] & 0x0f) * 10 + (msgw[15] >> 4);
  float temp_c = temp_raw * 0.1f;
  int flags    = (msgw[15] & 0x0f);
  int battery_low = (flags & 0x06) == 0x06;
  if (temp_raw > 600)
      temp_c = (temp_raw - 1000) * 0.1f;
  int humidity = (msgw[16] >> 4) * 10 + (msgw[16] & 0x0f);
  int lght_raw = (msgw[17] >> 4) * 100000 + (msgw[17] & 0x0f) * 10000 + (msgw[18] >> 4) * 1000 + (msgw[18] & 0x0f) * 100 + (msgw[19] >> 4) * 10 + (msgw[19] & 0x0f);
  int uv_raw =   (msgw[20] >> 4) * 100 + (msgw[20] & 0x0f) * 10 + (msgw[21] >> 4);

  float light_klx = lght_raw * 0.001f; // TODO: remove this
  float light_lux = lght_raw;
  float uv_index = uv_raw * 0.1f;

  // The RTL_433 decoder does not include any field to verify that these data
  // are ok, so we are assuming that they are ok if the decode status is ok.
  sensor[slot].temp_ok      = true;
  sensor[slot].humidity_ok  = true;
  sensor[slot].wind_ok      = true;
  sensor[slot].rain_ok      = true;
  sensor[slot].light_ok     = true;
  sensor[slot].uv_ok        = true;

  sensor[slot].sensor_id   = id_tmp;
  sensor[slot].temp_c      = temp_c;
  sensor[slot].humidity    = humidity;
#ifdef WIND_DATA_FLOATINGPOINT
  sensor[slot].wind_gust_meter_sec     = wgst_raw * 0.1f;
  sensor[slot].wind_avg_meter_sec      = wavg_raw * 0.1f;
  sensor[slot].wind_direction_deg      = wdir * 1.0f;
#endif
#ifdef WIND_DATA_FIXEDPOINT
  sensor[slot].wind_gust_meter_sec_fp1 = wgst_raw;
  sensor[slot].wind_avg_meter_sec_fp1  = wavg_raw;
  sensor[slot].wind_direction_deg_fp1  = wdir * 10;
#endif
  sensor[slot].rain_mm     = rain_mm;
  sensor[slot].light_klx   = light_klx;
  sensor[slot].light_lux   = light_lux;
  sensor[slot].uv          = uv_index;
  sensor[slot].battery_ok  = !battery_low;
  sensor[slot].rssi        = rssi;

  /* clang-format off */
  /*  data = data_make(
          "model",            "",             DATA_STRING, "Bresser-7in1",
          "id",               "",             DATA_INT,    id,
          "temperature_C",    "Temperature",  DATA_FORMAT, "%.1f C", DATA_DOUBLE, temp_c,
          "humidity",         "Humidity",     DATA_INT,    humidity,
          "wind_max_m_s",     "Wind Gust",    DATA_FORMAT, "%.1f m/s", DATA_DOUBLE, wgst_raw * 0.1f,
          "wind_avg_m_s",     "Wind Speed",   DATA_FORMAT, "%.1f m/s", DATA_DOUBLE, wavg_raw * 0.1f,
          "wind_dir_deg",     "Direction",    DATA_INT,    wdir,
          "rain_mm",          "Rain",         DATA_FORMAT, "%.1f mm", DATA_DOUBLE, rain_mm,
          "light_klx",        "Light",        DATA_FORMAT, "%.3f klx", DATA_DOUBLE, light_klx, // TODO: remove this
          "light_lux",        "Light",        DATA_FORMAT, "%.3f lux", DATA_DOUBLE, light_lux,
          "uv",               "UV Index",     DATA_FORMAT, "%.1f", DATA_DOUBLE, uv_index,
          "battery_ok",       "Battery",      DATA_INT,    !battery_low,
          "mic",              "Integrity",    DATA_STRING, "CRC",
          NULL);
   */
  /* clang-format on */

//  decoder_output_data(decoder, data);

  return DECODE_OK;

}
#endif
