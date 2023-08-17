///////////////////////////////////////////////////////////////////////////////////////////////////
// RainGauge.cpp
//
// Calculation of hourly (past 60 minutes), daily, weekly and monthly rainfall
// from raw rain gauge data.
// 
// Non-volatile data is stored in the ESP32's RTC RAM to allow retention during deep sleep mode.
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 08/2022
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
// 20220830 Created
// 20230716 Implemented sensor startup handling
// 20230817 Implemented partial reset
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "RainGauge.h"

const int SECONDS_PER_HOUR = 3600;
const int SECONDS_PER_DAY  = 86400;

#ifndef RTC_DATA_ATTR
   #define RTC_DATA_ATTR static
#endif
#if !defined(ESP32)
   #pragma message("RainGauge with SLEEP_EN only supported on ESP32!")
#endif


/**
 * \typedef nvData_t
 *
 * \brief Data structure for rain statistics to be stored in non-volatile memory
 *
 * On ESP32, this data is stored in the RTC RAM. 
 */
typedef struct {
    /* rainfall during past hour - circular buffer */
    uint32_t  tsBuf[RAINGAUGE_BUF_SIZE];
    uint16_t  rainBuf[RAINGAUGE_BUF_SIZE];
    uint8_t   head;
    uint8_t   tail;

    /* Sensor startup handling */
    bool      startupPrev; // previous state of startup
    float     rainStartup; // rain gauge before startup 

    /* Rainfall of current day (can start anytime, but will reset on begin of new day) */
    uint8_t   tsDayBegin; // day of week
    float     rainDayBegin; // rain gauge @ begin of day

    /* Rainfall of current week (can start anytime, but will reset on Monday */
    uint8_t   tsWeekBegin; // day of week 
    float     rainWeekBegin; // rain gauge @ begin of week
    uint8_t   wdayPrev; // day of week at previous run - to detect new week

    /* Rainfall of current calendar month (can start anytime, but will reset at begin of month */
    uint8_t   tsMonthBegin; // month
    float     rainMonthBegin; // rain gauge @ begin of month

    float     rainPrev;  // rain gauge at previous run - to detect overflow
    uint16_t  rainOvf; // number of rain gauge overflows
} nvData_t;


RTC_DATA_ATTR nvData_t nvData = {
   .tsBuf = {0},
   .rainBuf = {0}, 
   .head = 0,
   .tail = 0,
   .startupPrev = false,
   .rainStartup = 0,
   .tsDayBegin = 0xFF,
   .rainDayBegin = 0,
   .tsWeekBegin = 0xFF,
   .rainWeekBegin = 0,
   .wdayPrev = 0xFF,
   .tsMonthBegin = 0xFF,
   .rainMonthBegin = 0,
   .rainPrev = 0,
   .rainOvf = 0
};


/**
 * \verbatim
 * Total rainfall in the past hour
 * -------------------------------
 * To determine the rainfall in the past hour, timestamps (ts) and rain gauge values (rain) 
 * are stored in a circular buffer:
 *
 *      ---------------     -----------
 * .-> |   |   |   |   |...|   |   |   |--. 
 * |    ---------------     -----------   |
 * |     ^                   ^            |
 * |    tail                head          |
 * `--------------------------------------'
 *
 * 
 * - Add new value: 
 *   increment(head); 
 *   rainBuf[head] = rainNow; 
 *   tsBuf[head]   = tsNow;
 * 
 * - Remove stale entries: 
 *   if ((tsBuf[head]-tsBuf[tail]) > 1hour) {
 *     increment(tail);
 *   }
 * 
 * - Calculate hourly rate:
 *   rainHour = rainBuf[head] - rainBuf[tail];
 * 
 * Notes:
 * - increment(i) := "i = (i+1) mod RAINGAUGE_BUF_SIZE"
 * - If a new value is added when the buffer is already filled
 *   (which would result in increment(head) == tail), head is
 *   NOT incremented and the previous value is overwritten instead.
 * - Rain values are stored as uint16_t encoded as fixed-point data with one decimal
 *   to reduce memory consumption.
 * - Timestamps are stored as seconds since midnight; the discontinuity between days
 *   is handled when stale entries are removed. 
 * \endverbatim
 */

#ifdef _DEBUG_CIRCULAR_BUFFER_
void
RainGauge::printCircularBuffer(void)
{
    for (int i=0; i<RAINGAUGE_BUF_SIZE; i++)
        printf("[%3d ]\t", i);
    printf("\n");
    for (int i=0; i<RAINGAUGE_BUF_SIZE; i++)
        printf("%6d\t", nvData.tsBuf[i] & 0xFFFF);
    printf("\n");
    for (int i=0; i<RAINGAUGE_BUF_SIZE; i++)
        printf("%6.1f\t", 0.1 * nvData.rainBuf[i]);
    printf("\n");
    for (int i=0; i<RAINGAUGE_BUF_SIZE; i++)
        printf("%3s\t", (i == nvData.tail) ? "T" : "");
    printf("\n");
    for (int i=0; i<RAINGAUGE_BUF_SIZE; i++)
        printf("%3s\t", (i == nvData.head) ? "H" : "");
    printf("\n\n");
}
#endif


void
RainGauge::reset(uint8_t flags)
{
    if (flags & RESET_RAIN_H) {
        nvData.head           = 0;
        nvData.tail           = 0;

    }
    if (flags & RESET_RAIN_D) {
        nvData.tsDayBegin     = 0xFF;
        nvData.rainDayBegin   = 0;
    }
    if (flags & RESET_RAIN_W) {
        nvData.tsWeekBegin    = 0xFF;
        nvData.rainWeekBegin  = 0;

    }
    if (flags & RESET_RAIN_M) {
        nvData.tsMonthBegin   = 0xFF;
        nvData.rainMonthBegin = 0;
    }

    if (flags == (RESET_RAIN_H | RESET_RAIN_D | RESET_RAIN_W | RESET_RAIN_M)) {
        nvData.startupPrev    = false;
        nvData.rainStartup    = 0;
        nvData.rainPrev       = 0;
        nvData.rainOvf        = 0;
        rainCurr              = 0;
    }
}

void
RainGauge::init(tm t, float rain)
{
    // Seconds since midnight
    uint32_t ts = timeStamp(t);
    
    // Init circular buffer with current timestamp (seconds since midnight) and rain gauge data
    for (int i=0; i<RAINGAUGE_BUF_SIZE; i++) {
        nvData.tsBuf[i]   = (uint32_t)ts;
        nvData.rainBuf[i] = (uint16_t)(rain * 10);
    }
    nvData.head = 0;
    nvData.tail = 0;
    nvData.tsDayBegin = t.tm_wday;
}

uint32_t
RainGauge::timeStamp(tm t)
{
    time_t  ts;          // seconds since epoch
    tm      t_midnight;  // timestamp at midnight
    time_t  ts_midnight; // seconds since epoch at midnight
   
    // Calculate seconds since midnight
    t.tm_sec           = 0;
    t_midnight = t;
    t_midnight.tm_hour = 0;
    t_midnight.tm_min  = 0;
    ts          = mktime(&t);
    ts_midnight = mktime(&t_midnight);
    ts = ts - ts_midnight;
    
    return (uint32_t)ts;
}

void
RainGauge::update(tm t, float rain, bool startup, float raingaugeMax)
{
    uint8_t  head_tmp; // circular buffer; temporary head index

    // Seconds since Midnight
    uint32_t ts = timeStamp(t);

    if (rain < nvData.rainPrev) {
       // Startup change 0->1 detected
       if (!nvData.startupPrev && startup) {
           // Save last rain value before startup
           nvData.rainStartup = nvData.rainPrev;
       } else {
           nvData.rainOvf++;
       }
    }
   
    nvData.startupPrev = startup;
    nvData.rainPrev = rain;
    
    rainCurr = (nvData.rainOvf * raingaugeMax) + nvData.rainStartup + rain;

    // Check if no saved data is available yet
    if (nvData.wdayPrev == 0xFF) {
        // Save day of week to allow detection of new week
        nvData.wdayPrev = t.tm_wday;

        // Init tail of circular buffer
        nvData.tsBuf[nvData.tail]   = ts;
        nvData.rainBuf[nvData.tail] = (uint16_t)(rainCurr * 10);
    }

    // Remove stale entries
    uint32_t ts_cmp;
    while (!(nvData.tail == nvData.head)) {
        ts_cmp = ts;
        // if current timestamp smaller than saved timestamp, add one day
        if (ts_cmp < nvData.tsBuf[nvData.tail]) {
            ts_cmp = ts_cmp + SECONDS_PER_DAY;
        }
        if ((ts_cmp - nvData.tsBuf[nvData.tail]) <= SECONDS_PER_HOUR)
            break;
        nvData.tail = (nvData.tail == RAINGAUGE_BUF_SIZE-1) ? 0 : nvData.tail+1;
    }

    //printCircularBuffer();
    // Add new value
    head_tmp = (nvData.head == RAINGAUGE_BUF_SIZE-1) ? 0 : nvData.head+1;
    // Prevent head from reaching tail if update rate is too fast
    nvData.head = (head_tmp == nvData.tail) ? nvData.head : head_tmp;
    nvData.tsBuf[nvData.head]   = ts;
    nvData.rainBuf[nvData.head] = (uint16_t)(rainCurr * 10);
            
    // Check if day of the week has changed
    // or no saved data is available yet
    if ((t.tm_wday != nvData.tsDayBegin) || 
        (nvData.tsDayBegin == 0xFF)) {
        // save timestamp
        nvData.tsDayBegin = t.tm_wday;
        
        // save rain gauge value
        nvData.rainDayBegin = rainCurr;
    }
    
    // Check if the week has changed
    // (transition from 0 - Sunday to 1 - Monday
    // or no saved data is available yet
    if (((t.tm_wday == 1) && (nvData.wdayPrev == 0)) ||
        (nvData.tsWeekBegin == 0xFF)) {
        // save timestamp
        nvData.tsWeekBegin = t.tm_wday;
        
        // save rain gauge value
        nvData.rainWeekBegin = rainCurr;
    }
    
    // Update day of week
    nvData.wdayPrev = t.tm_wday;
        
    // Check if month has changed
    // or no saved data is available yet
    if ((t.tm_mon != nvData.tsMonthBegin) ||
        (nvData.tsMonthBegin == 0xFF)) {
        // save timestamp
        nvData.tsMonthBegin = t.tm_mon;
        
        // save rain gauge value
        nvData.rainMonthBegin = rainCurr;
    }

}

float
RainGauge::pastHour(void)
{
    return (float)(0.1 * (nvData.rainBuf[nvData.head] - nvData.rainBuf[nvData.tail]));
}

float
RainGauge::currentDay(void)
{
    return rainCurr - nvData.rainDayBegin;
}

float
RainGauge::currentWeek(void)
{
    return rainCurr - nvData.rainWeekBegin;
}

float
RainGauge::currentMonth(void)
{
    return rainCurr - nvData.rainMonthBegin;
}
