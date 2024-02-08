///////////////////////////////////////////////////////////////////////////////////////////////////
// Lightning.cpp
//
// Post-processing of lightning sensor data
//
// Input:
//     * Timestamp
//     * Sensor startup flag
//     * Accumulated lightning event counter
//     * Estimated distance of last strike
//
// Output:
//     * Number of events during last update cycle
//     * Timestamp, number of strikes and estimated distance of last event
//     * Number of strikes during past 60 minutes
//
// Non-volatile data is stored in the ESP32's RTC RAM or in Preferences (Flash FS)
// to allow retention during deep sleep mode.
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
// 20230721 Created
// 20231105 Added data storage via Preferences, modified history implementation
// 20240113 Fixed timestamp format string and hourly history calculation
// 20240114 Implemented counter overflow and startup handling
// 20240119 Changed preferences to class member
// 20240123 Changed scope of nvLightning -
//          Using RTC RAM: global
//          Using Preferences, Unit Tests: class member
//          Modified for unit testing
//          Modified pastHour()
//          Added qualityThreshold
// 20240124 Fixed handling of overflow, startup and missing update cycles
// 20240125 Added lastCycle()
// 20240130 Update pastHour() documentation
//
// ToDo:
// -
//
// Notes:
// Maximum number of lighning strikes on earth:
// https://en.wikipedia.org/wiki/Catatumbo_lightning
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "Lightning.h"


#if !defined(LIGHTNING_USE_PREFS) && !defined(INSIDE_UNITTEST)
RTC_DATA_ATTR nvLightning_t nvLightning = {
    .lastUpdate = 0,
    .startupPrev = false,
    .preStCount = 0,
    .accCount = 0,
    .prevCount = -1,
    .events = 0,
    .distance = 0,
    .timestamp = 0,
    .hist = {0}
};
#endif

void
Lightning::reset(void)
{
    nvLightning.lastUpdate = 0;
    nvLightning.startupPrev = false;
    nvLightning.preStCount = 0;
    nvLightning.prevCount = -1;
    nvLightning.accCount = 0;
    nvLightning.events = -1;
    nvLightning.distance = 0;
    nvLightning.timestamp = 0;
    deltaEvents = -1;
}

void
Lightning::hist_init(int16_t count)
{
    for (int i=0; i<LIGHTNING_HIST_SIZE; i++) {
        nvLightning.hist[i] = count;
    }
}

#if defined(LIGHTNING_USE_PREFS)  && !defined(INSIDE_UNITTEST)
void Lightning::prefs_load(void)
{
    preferences.begin("BWS-LGT", false);
    nvLightning.lastUpdate   = preferences.getULong64("lastUpdate", 0);
    nvLightning.startupPrev  = preferences.getBool("startupPrev", false);
    nvLightning.preStCount   = preferences.getShort("preStCount", 0);
    nvLightning.accCount     = preferences.getUInt("accCount", 0);
    nvLightning.prevCount    = preferences.getUShort("prevCount", -1);
    nvLightning.events       = preferences.getUShort("events", -1);
    nvLightning.distance     = preferences.getUChar("distance", 0);
    nvLightning.timestamp    = preferences.getULong64("timestamp", 0);
    //preferences.getBytes("hist", nvLightning.hist, sizeof(nvLightning.hist));
    // Optimization: Reduces number of Flash writes
    for (int i=0; i<LIGHTNING_HIST_SIZE; i++) {
        char buf[7];
        sprintf(buf, "hist%02d", i);
        nvLightning.hist[i] = preferences.getShort(buf, -1);
    }
    log_d("lastUpdate   =%s", String(nvLightning.lastUpdate).c_str());
    log_d("startupPrev  =%d", nvLightning.startupPrev);
    log_d("preStCount   =%d", nvLightning.preStCount);
    log_d("accCount     =%u", nvLightning.accCount);
    log_d("prevCount    =%d", nvLightning.prevCount);
    log_d("events       =%d", nvLightning.events);
    log_d("distance     =%d", nvLightning.distance);
    log_d("timestamp    =%s", String(nvLightning.timestamp).c_str());
    preferences.end();
}

void Lightning::prefs_save(void)
{
    preferences.begin("BWS-LGT", false);
    preferences.putULong64("lastUpdate", nvLightning.lastUpdate);
    preferences.putBool("startupPrev", nvLightning.startupPrev);
    preferences.putShort("preStCount", nvLightning.preStCount);
    preferences.putUInt("accCount", nvLightning.accCount);
    preferences.putUShort("prevCount", nvLightning.prevCount);
    preferences.putUShort("events", nvLightning.events);
    preferences.putUChar("distance", nvLightning.distance);
    preferences.putULong64("timestamp", nvLightning.timestamp);
    //preferences.putBytes("hist", nvLightning.hist, sizeof(nvLightning.hist));
    // Optimization: Reduces number of Flash writes
    for (int i=0; i<LIGHTNING_HIST_SIZE; i++) {
        char buf[7];
        sprintf(buf, "hist%02d", i);
        preferences.putShort(buf, nvLightning.hist[i]);
    }
    preferences.end();
}
#endif

void
Lightning::update(time_t timestamp, int16_t count, uint8_t distance, bool startup)
{
    #if defined(LIGHTNING_USE_PREFS)  && !defined(INSIDE_UNITTEST)
        prefs_load();
    #endif

    if (nvLightning.lastUpdate == 0) {
        // Initialize history
        hist_init();
    }

    if (nvLightning.prevCount == -1) {
        // No previous count or counter reset
        nvLightning.prevCount = count;
        nvLightning.lastUpdate = timestamp;

        #if defined(LIGHTNING_USE_PREFS)  && !defined(INSIDE_UNITTEST)
            prefs_save();
        #endif
    }
    
    currCount = nvLightning.accCount + count;

    if (currCount < nvLightning.prevCount) {
       // Startup change 0->1 detected
       if (!nvLightning.startupPrev && startup) {
           // Add last rain gauge reading before startup
           nvLightning.accCount += nvLightning.preStCount;
       } else {
           // Add counter overflow
           nvLightning.accCount += LIGHTNINGCOUNT_MAX_VALUE;
       }
    }

    currCount = nvLightning.accCount + count;
    nvLightning.startupPrev = startup;
    nvLightning.preStCount = count;

    /**
     * \verbatim
     * Total number of events during past 60 minutes
     * ----------------------------------------------
     * 
     * In each update():
     * - timestamp (time_t) ->                  t (localtime, struct tm)
     * - calculate index into hist[]:           idx = t.tm_min / LIGHTNING_UPD_RATE
     * - expired time since last update:        t_delta = timestamp - nvLightning.lastUpdate
     * - number of events since last update:    delta = currCount - nvLightning.prevCount
     * - t_delta
     *      < 0:                                something is wrong, e.g. RTC was not set correctly -> ignore, return
     *      t_delta < expected update rate:
     *          idx same as in previous cycle:  hist[idx] += delta
     *          idx changed by 1:               hist[idx] = delta
     *      t_delta >= history size:            mark all history entries as invalid
     *      else (index changed > 1):           mark all history entries in interval [expected_index, current_index) as invalid
     *                                          hist[idx] = delta
     *
     *   ---------------     -----------
     *  |   |   |   |   |...|   |   |   |   hist[LIGHTNING_HIST_SIZE]
     *   ---------------     -----------
     *        ^
     *        |
     *       idx = t.tm_min / LIGHTNING_UPD_RATE
     *
     * - Calculate hourly rate:
     *   pastHour = sum of all valid hist[] entries
     *
     * \endverbatim
     */

    // Delta time between last update and current time
    time_t t_delta = timestamp - nvLightning.lastUpdate;
    log_d("t_delta: %ld", t_delta);

    // t_delta < 0: something is wrong, e.g. RTC was not set correctly
    if (t_delta < 0) {
        log_w("Negative time span since last update!?");
        return; 
    }


    int16_t delta = currCount - nvLightning.prevCount;
    deltaEvents = delta;

    if (delta > 0) {
        // Save detected event
        nvLightning.events = delta;
        nvLightning.distance = distance;
        nvLightning.timestamp = timestamp;
    }


    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    int idx = timeinfo.tm_min / LIGHTNING_UPD_RATE;

    if (t_delta / 60 < LIGHTNING_UPD_RATE) {
        // t_delta shorter than expected update rate
        if (nvLightning.hist[idx] < 0)
            nvLightning.hist[idx] = 0;
        struct tm t_prev;
        localtime_r(&nvLightning.lastUpdate, &t_prev);
        if (t_prev.tm_min / LIGHTNING_UPD_RATE == idx) {
            // same index as in previous cycle - add value
            nvLightning.hist[idx] += delta;
            log_d("hist[%d]=%d (upd)", idx, nvLightning.hist[idx]);
        } else {
            // different index - new value
            nvLightning.hist[idx] = delta;
            log_d("hist[%d]=%d (new)", idx, nvLightning.hist[idx]);
        }
    }
    else if (t_delta >= LIGHTNING_HIST_SIZE * LIGHTNING_UPD_RATE * 60) {
        // t_delta >= LIGHTNING_HIST_SIZE * LIGHTNING_UPDATE_RATE -> reset history
        log_w("History time frame expired, resetting!");
        hist_init();
    }
    else {
        // Some other index
        
        // Mark all history entries in interval [expected_index, current_index) as invalid
        // N.B.: excluding current index!
        for (time_t ts = nvLightning.lastUpdate + (LIGHTNING_UPD_RATE * 60); ts < timestamp; ts += LIGHTNING_UPD_RATE * 60) {
            log_d("ts: %ld, timestamp: %ld", ts, timestamp);
            localtime_r(&ts, &timeinfo);
            int idx = timeinfo.tm_min / LIGHTNING_UPD_RATE;
            nvLightning.hist[idx] = -1;
            log_d("hist[%d]=-1", idx);
        }
        
        // Write delta
        nvLightning.hist[idx] = delta;
        log_d("hist[%d]=%d", idx, delta);
    }
    
    #if CORE_DEBUG_LEVEL == ARDUHAL_LOG_LEVEL_DEBUG
        String buf;
        buf = String("hist[]={");
        for (size_t i=0; i<LIGHTNING_HIST_SIZE; i++) {
            buf += String(nvLightning.hist[i]) + String(", ");
        }
        buf += String("}");
        log_d("%s", buf.c_str());
    #endif

    nvLightning.lastUpdate = timestamp;
    nvLightning.prevCount = currCount;

    #if defined(LIGHTNING_USE_PREFS)  && !defined(INSIDE_UNITTEST)
        prefs_save();
    #endif
}

int 
Lightning::lastCycle(void)
{
    return deltaEvents;
}

bool
Lightning::lastEvent(time_t &timestamp, int &events, uint8_t &distance)
{
    if (nvLightning.events == -1) {
        events = -1;
        return false;
    }

    events = nvLightning.events;
    distance = nvLightning.distance;
    timestamp = nvLightning.timestamp;

    return true;
}

int
Lightning::pastHour(bool *valid, int *quality)
{
    int _quality = 0;
    int sum = 0;

    // Sum of all valid entries
    for (size_t i=0; i<LIGHTNING_HIST_SIZE; i++){
        if (nvLightning.hist[i] >= 0) {
            sum += nvLightning.hist[i];
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

    return sum;
}
