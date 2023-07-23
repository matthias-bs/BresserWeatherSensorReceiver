///////////////////////////////////////////////////////////////////////////////////////////////////
// Lightning.cpp
//
// Post-processing of lightning sensor data
//
// Input:   
//     * Timestamp (time and date)
//     * Sensor startup flag
//     * Accumulated lightning event counter
//     * Estimated distance of last strike
//
// Output:
//     * Number of events during last update cycle
//     * Timestamp of last event
//     * Number of strikes during past 60 minutes  
//
// Non-volatile data is stored in the ESP32's RTC RAM to allow retention during deep sleep mode.
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 07/2023
//
//
// MIT License
//
// Copyright (c) 2023 Matthias Prinke
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
// 20220721 Created
//
// ToDo: 
// - Store non-volatile data in NVS Flash instead of RTC RAM
//   (to support ESP8266 and to keep data during power-off)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "Lightning.h"

#ifndef RTC_DATA_ATTR
   #define RTC_DATA_ATTR static
#endif
#if !defined(ESP32)
   #pragma message("Lightning with SLEEP_EN only supported on ESP32!")
#endif

/**
 * \typedef nvData_t
 *
 * \brief Data structure for lightning sensor to be stored in non-volatile memory
 *
 * On ESP32, this data is stored in the RTC RAM. 
 */
typedef struct {
    /* Data of last lightning event */
    int       prevCount;
    int       events;
    uint8_t   distance;
    time_t    timestamp;

    /* Data of past 60 minutes */
    uint16_t  hist[LIGHTNING_HIST_SIZE];

} nvLightning_t;

RTC_DATA_ATTR nvLightning_t nvLightning = {
   .prevCount = -1,
   .events = 0,
   .distance = 0,
   .timestamp = 0,
   .hist = {0}
};

void
Lightning::reset(void)
{
    nvLightning.prevCount      = -1;
    nvLightning.events         = -1;
    nvLightning.distance       = 0;
    nvLightning.timestamp      = 0;
}

void
Lightning::init(uint16_t count)
{
    for (int i=0; i<LIGHTNING_HIST_SIZE; i++) {
        nvLightning.hist[i] = count;
    }
}

void
Lightning::update(time_t timestamp, int count, uint8_t distance, bool startup)
{
    // currently unused
    (void)startup;

    if (nvLightning.prevCount == -1) {
        nvLightning.prevCount = count;
        return;
    }

    if (nvLightning.prevCount < count) {
        nvLightning.events = count - nvLightning.prevCount;
        nvLightning.prevCount = count;
        nvLightning.distance = distance;
        nvLightning.timestamp = timestamp;
    }


    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);

    int min = timeinfo.tm_min;
    int idx = min / LIGHTNING_UPD_RATE;

    // Search for skipped entries, i.e. entries which are smaller than their predecessor
    int start = inc(idx);
    int end   = dec(idx);
    for (int i=start; i==end; i=inc(i)) {
        printf("i=%d ", i);
        if (nvLightning.hist[inc(i)] < nvLightning.hist[i]) {
            printf("Marking %d as invalid\n", inc(i));
            nvLightning.hist[inc(i)] = -1;
        }
    }
    printf("\n");
    nvLightning.hist[idx] = count;
}

bool
Lightning::lastEvent(time_t &timestamp, int &events, uint8_t &distance)
{
    if (nvLightning.events == -1) {
        return false;
    }

    events = nvLightning.events;
    distance = nvLightning.distance;
    timestamp = nvLightning.timestamp;

    return true;
}

int
Lightning::pastHour(time_t timestamp)
{
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    
    int min = timeinfo.tm_min;
    int idx = min / LIGHTNING_UPD_RATE;
    
    if (nvLightning.hist[inc(idx)] == -1) {
        printf("Invalid\n");
    }
    return nvLightning.hist[idx] - nvLightning.hist[inc(idx)]; 
}
