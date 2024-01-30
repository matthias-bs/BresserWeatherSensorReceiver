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
// 20231227 Added prerequisites for storing rain data in preferences
// 20231218 Implemented storing of rain data in preferences, 
//          new algotrithm for past 60 minutes rainfall
// 20240118 Changed raingaugeMax to class member set by constructor
//          Modified startup/overflow handling
// 20240119 Changed preferences to class member
//          Modified update at the same index as before
//          Modified pastHour() algorithm and added features
// 20240120 Removed old implementation
// 20240122 Changed scope of nvData -
//          Using RTC RAM: global
//          Using Preferences, Unit Tests: class member
//          Improvements
// 20240130 Update pastHour() documentation
//
// ToDo: 
// -
//
// Notes:
// - Extreme values of rainfall: https://en.wikipedia.org/wiki/List_of_weather_records#Rain
//   (for variable widths and evaluation of rain gauge overflows)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "RainGauge.h"


#if !defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
RTC_DATA_ATTR nvData_t nvData = {
   .lastUpdate = 0,
   .hist = {-1},
   .startupPrev = false,
   .rainPreStartup = 0,
   .tsDayBegin = 0xFF,
   .rainDayBegin = 0,
   .tsWeekBegin = 0xFF,
   .rainWeekBegin = 0,
   .wdayPrev = 0xFF,
   .tsMonthBegin = 0xFF,
   .rainMonthBegin = 0,
   .rainPrev = 0,
   .rainAcc = 0
};
#endif


void
RainGauge::reset(uint8_t flags)
{
#if defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
    preferences.begin("BWS-RAIN", false);
    if (flags & RESET_RAIN_H) {
        hist_init();
        for (int i=0; i<RAIN_HIST_SIZE; i++) {
            char buf[7];
            sprintf(buf, "hist%02d", i);
            preferences.putShort(buf, nvData.hist[i]);
        }
    }
    if (flags & RESET_RAIN_D) {
        nvData.tsDayBegin     = 0xFF;
        nvData.rainDayBegin   = 0;
        preferences.putUChar("tsDayBegin", nvData.tsDayBegin);
        preferences.putFloat("rainDayBegin", nvData.rainDayBegin);
    }
    if (flags & RESET_RAIN_W) {
        nvData.tsWeekBegin    = 0xFF;
        nvData.rainWeekBegin  = 0;
        preferences.putUChar("tsWeekBegin", nvData.tsWeekBegin);
        preferences.putFloat("rainWeekBegin", nvData.rainWeekBegin);
    }
    if (flags & RESET_RAIN_M) {
        nvData.tsMonthBegin   = 0xFF;
        nvData.rainMonthBegin = 0;
        preferences.putUChar("tsMonthBegin", nvData.tsMonthBegin);
        preferences.putFloat("rainMonthBegin", nvData.rainMonthBegin);
    }

    if (flags == (RESET_RAIN_H | RESET_RAIN_D | RESET_RAIN_W | RESET_RAIN_M)) {
        nvData.startupPrev       = false;
        nvData.rainPreStartup    = 0;
        nvData.rainPrev          = -1;
        nvData.rainAcc           = 0;
        rainCurr                 = 0;
        preferences.putBool("startupPrev", nvData.startupPrev);
        preferences.putFloat("rainPreStartup", nvData.rainPreStartup);
        preferences.putFloat("rainPrev", nvData.rainPrev);
        preferences.putFloat("rainAcc", nvData.rainAcc);
    }    
    preferences.end();
#else
    if (flags & RESET_RAIN_H) {
        hist_init();
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
        nvData.startupPrev       = false;
        nvData.rainPreStartup    = 0;
        nvData.rainPrev          = -1;
        nvData.rainAcc           = 0;
        rainCurr                 = 0;
    }
#endif
}

void
RainGauge::hist_init(int16_t rain)
{
    for (int i=0; i<RAIN_HIST_SIZE; i++) {
        nvData.hist[i] = rain;
    }
}

#if defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
void
RainGauge::prefs_load(void)
{
    preferences.begin("BWS-RAIN", false);
    nvData.lastUpdate     = preferences.getULong64("lastUpdate", 0);
    // Optimization: Reduces number of Flash writes
    // preferences.getBytes("hist", nvData.hist, sizeof(nvData.hist));
    for (int i=0; i<RAIN_HIST_SIZE; i++) {
        char buf[7];
        sprintf(buf, "hist%02d", i);
        nvData.hist[i] = preferences.getShort(buf, -1);
    }
    nvData.startupPrev       = preferences.getBool("startupPrev", false);
    nvData.rainPreStartup    = preferences.getFloat("rainPreStartup", 0);
    nvData.tsDayBegin        = preferences.getUChar("tsDayBegin", 0xFF);
    nvData.rainDayBegin      = preferences.getFloat("rainDayBegin", 0);
    nvData.tsWeekBegin       = preferences.getUChar("tsWeekBegin", 0xFF);
    nvData.rainWeekBegin     = preferences.getFloat("rainWeekBegin", 0);
    nvData.wdayPrev          = preferences.getUChar("wdayPrev", 0xFF);
    nvData.tsMonthBegin      = preferences.getUChar("tsMonthBegin", 0xFF);
    nvData.rainMonthBegin    = preferences.getFloat("rainMonthBegin", 0);
    nvData.rainPrev          = preferences.getFloat("rainPrev", -1);
    nvData.rainAcc           = preferences.getFloat("rainAcc", 0);

    log_d("lastUpdate        =%s", String(nvData.lastUpdate).c_str());
    log_d("startupPrev       =%d", nvData.startupPrev);
    log_d("rainPreStartup    =%f", nvData.rainPreStartup);
    log_d("tsDayBegin        =%d", nvData.tsDayBegin);
    log_d("rainDayBegin      =%f", nvData.rainDayBegin);
    log_d("tsWeekBegin       =%d", nvData.tsWeekBegin);
    log_d("rainWeekBegin     =%f", nvData.rainWeekBegin);
    log_d("wdayPrev          =%d", nvData.wdayPrev);
    log_d("tsMonthBegin      =%d", nvData.tsMonthBegin);
    log_d("rainMonthBegin    =%f", nvData.rainMonthBegin);
    log_d("rainPrev          =%f", nvData.rainPrev);
    log_d("rainAcc           =%f", nvData.rainAcc);
    preferences.end();
}

void
RainGauge::prefs_save(void)
{
    preferences.begin("BWS-RAIN", false);
    preferences.putULong64("lastUpdate", nvData.lastUpdate);
    // Optimization: Reduces number of Flash writes
    // preferences.putBytes("hist", nvData.hist, sizeof(nvData.hist));
    for (int i=0; i<RAIN_HIST_SIZE; i++) {
        char buf[7];
        sprintf(buf, "hist%02d", i);
        preferences.putShort(buf, nvData.hist[i]);
    }
    preferences.putBool("startupPrev", nvData.startupPrev);
    preferences.putFloat("rainPreStartup", nvData.rainPreStartup);
    preferences.putUChar("tsDayBegin", nvData.tsDayBegin);
    preferences.putFloat("rainDayBegin", nvData.rainDayBegin);
    preferences.putUChar("tsWeekBegin", nvData.tsWeekBegin);
    preferences.putFloat("rainWeekBegin", nvData.rainWeekBegin);
    preferences.putUChar("wdayPrev", nvData.wdayPrev);
    preferences.putUChar("tsMonthBegin", nvData.tsMonthBegin);
    preferences.putFloat("rainMonthBegin", nvData.rainMonthBegin);
    preferences.putFloat("rainPrev", nvData.rainPrev);
    preferences.putFloat("rainAcc", nvData.rainAcc);
    preferences.end();
}
#endif


void
RainGauge::update(time_t timestamp, float rain, bool startup)
{
    #if defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
        prefs_load();
    #endif

    struct tm t;
    localtime_r(&timestamp, &t);

    if (nvData.lastUpdate == 0) {
        // Initialize history
        hist_init();
    }

    if (nvData.rainPrev == -1) {
        // No previous count or counter reset
        nvData.rainPrev = rain;
        nvData.lastUpdate = timestamp;
        
        #if defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
            prefs_save();
        #endif
    }

    rainCurr = nvData.rainAcc + rain;
    
    if (rainCurr < nvData.rainPrev) {
       // Startup change 0->1 detected
       if (!nvData.startupPrev && startup) {
           // Add last rain gauge reading before startup
           nvData.rainAcc += nvData.rainPreStartup;
       } else {
           // Add counter overflow
           nvData.rainAcc += raingaugeMax;
       }
    }
    
    rainCurr = nvData.rainAcc + rain;
    nvData.startupPrev = startup;
    nvData.rainPreStartup = rain;

    float rainDelta = rainCurr - nvData.rainPrev;
    log_d("rainDelta: %.1f", rainDelta);

    // Check if no saved data is available yet
    if (nvData.wdayPrev == 0xFF) {
        // Save day of week to allow detection of new week
        nvData.wdayPrev = t.tm_wday;
    }

    /**
     * \verbatim
     * Total rainfall during past 60 minutes
     * --------------------------------------
     *
     * In each update():
     * - timestamp (time_t) ->                  t (localtime, struct tm)
     * - calculate index into hist[]:           idx = t.tm_min / RAINGAUGE_UPD_RATE
     * - expired time since last update:        t_delta = timestamp - nvData.lastUpdate
     * - amount of rain since last update:      rainDelta = rainCurr - nvData.rainPrev
     * - t_delta
     *      < 0:                                something is wrong, e.g. RTC was not set correctly -> ignore, return
     *      t_delta < expected update rate:
     *          idx same as in previous cycle:  hist[idx] += rainDelta
     *          idx changed by 1:               hist[idx] = rainDelta
     *      t_delta >= history size:            mark all history entries as invalid
     *      else (index changed > 1):           mark all history entries in interval [expected_index, current_index) as invalid
     *                                          hist[idx] = rainDelta
     *
     *   ---------------     -----------
     *  |   |   |   |   |...|   |   |   |   hist[RAIN_HIST_SIZE]
     *   ---------------     -----------
     *        ^
     *        |
     *       idx = t.tm_min / RAINGAUGE_UPD_RATE
     *
     * - Calculate hourly rate:
     *   pastHour = sum of all valid hist[] entries
     *
     * Notes:
     * - rainDelta values (floating point with resolution of 0.1) are stored as integers to reduce memory consumption.
     *   To avoid rounding errors, the rainDelta values are multiplied by 100 for conversion to integer.
     * \endverbatim
     */

    // Delta time between last update and current time
    time_t t_delta = timestamp - nvData.lastUpdate;
    log_d("t_delta: %ld", t_delta);

    // t_delta < 0: something is wrong, e.g. RTC was not set correctly
    if (t_delta < 0) {
        log_w("Negative time span since last update!?");
        return; 
    }


    int idx = t.tm_min / RAINGAUGE_UPD_RATE;

    if (t_delta / 60 < RAINGAUGE_UPD_RATE) {
        // t_delta shorter than expected update rate
        if (nvData.hist[idx] < 0)
            nvData.hist[idx] = 0;
        struct tm t_prev;
        localtime_r(&nvData.lastUpdate, &t_prev);
        if (t_prev.tm_min / RAINGAUGE_UPD_RATE == idx) {
            // same index as in previous cycle - add value
            nvData.hist[idx] += static_cast<int16_t>(rainDelta * 100);
            log_d("hist[%d]=%d (upd)", idx, nvData.hist[idx]);
        } else {
            // different index - new value
            nvData.hist[idx] = static_cast<int16_t>(rainDelta * 100);
            log_d("hist[%d]=%d (new)", idx, nvData.hist[idx]);
        }
    }
    else if (t_delta >= RAIN_HIST_SIZE * RAINGAUGE_UPD_RATE * 60) {
        // t_delta >= RAINGAUGE_HIST_SIZE * RAINGAUGE_UPDATE_RATE -> reset history
        log_w("History time frame expired, resetting!");
        hist_init();
    }
    else {
        // Some other index

        // Mark all history entries in interval [expected_index, current_index) as invalid
        // N.B.: excluding current index!
        for (time_t ts = nvData.lastUpdate + (RAINGAUGE_UPD_RATE * 60); ts < timestamp; ts += RAINGAUGE_UPD_RATE * 60) {
            struct tm timeinfo;
            localtime_r(&ts, &timeinfo);
            int idx = timeinfo.tm_min / RAINGAUGE_UPD_RATE;
            nvData.hist[idx] = -1;
            log_d("hist[%d]=-1", idx);
        }

        // Write delta
        nvData.hist[idx] = static_cast<int16_t>(rainDelta * 100);
        log_d("hist[%d]=%d (new)", idx, nvData.hist[idx]);
    }


    #if CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
        String buf;
        buf = String("hist[]={");
        for (size_t i=0; i<RAIN_HIST_SIZE; i++) {
            buf += String(nvData.hist[i]) + String(", ");
        }
        buf += String("}");
        log_d("%s", buf.c_str());
    #endif
    
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

    nvData.lastUpdate = timestamp;
    nvData.rainPrev = rainCurr;

    #if defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
        prefs_save();
    #endif
}

float
RainGauge::pastHour(bool *valid, int *quality)
{
    int _quality = 0;
    float res = 0;

    // Sum of all valid entries
    for (size_t i=0; i<RAIN_HIST_SIZE; i++){
        if (nvData.hist[i] >= 0) {
            res += nvData.hist[i] * 0.01;
            _quality++;
        }
    }

    // Optional: return quality indication
    if (quality)
        *quality = _quality;
    
    // Optional: return valid flag
    if (valid) {
        if (_quality >= qualityThreshold) {
            *valid = true;
        } else {
            *valid = false;
        }
    }

    return res;
}

float
RainGauge::currentDay(void)
{
    if (nvData.tsMonthBegin == 0xFF)
        return -1;
    
    return rainCurr - nvData.rainDayBegin;
}

float
RainGauge::currentWeek(void)
{
    if (nvData.tsWeekBegin == 0xFF)
        return -1;
    
    return rainCurr - nvData.rainWeekBegin;
}

float
RainGauge::currentMonth(void)
{
    if (nvData.tsMonthBegin == 0xFF)
        return -1;
    
    return rainCurr - nvData.rainMonthBegin;
}
