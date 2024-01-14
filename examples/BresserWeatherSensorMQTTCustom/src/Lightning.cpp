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
// 20230721 Created
// 20231105 Added data storage via Preferences, modified history implementation
// 20240113 Fixed timestamp format string and hourly history calculation
// 20240114 Implemented counter overflow and startup handling
//
// ToDo: 
//
// Notes:
// Maximum number of lighning strikes on earth:
// https://en.wikipedia.org/wiki/Catatumbo_lightning
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "Lightning.h"

#if defined(LIGHTNING_USE_PREFS)
#include <Preferences.h>
#endif

#if !defined(ESP32) && !defined(LIGHTNING_USE_PREFS)
   #pragma message("Lightning with SLEEP_EN and data in RTC RAM only supported on ESP32!")
#endif

/**
 * \typedef nvData_t
 *
 * \brief Data structure for lightning sensor to be stored in non-volatile memory
 *
 * On ESP32, this data is stored in the RTC RAM. 
 */
typedef struct {
    /* Timestamp of last update */
    time_t    lastUpdate;   //!< Timestamp of last update

    /* Startup handling */
    bool      startupPrev;  //!< Previous startup flag value

    /* Data of last lightning event */
    int16_t   prevCount;    //!< Previous counter value
    int16_t   events;       //!< Number of events reported at last event
    uint8_t   distance;     //!< Distance at last event
    time_t    timestamp;    //!< Timestamp of last event

    /* Data of past 60 minutes */
    int16_t   hist[LIGHTNING_HIST_SIZE];

} nvLightning_t;

#if defined(LIGHTNING_USE_PREFS)
static Preferences preferences;
#else
RTC_DATA_ATTR
#endif
nvLightning_t nvLightning = {
    .lastUpdate = 0,
    .startupPrev = false,
    .prevCount = -1,
    .events = 0,
    .distance = 0,
    .timestamp = 0,
    .hist = {0}
};


void
Lightning::reset(void)
{
    nvLightning.lastUpdate = 0;
    nvLightning.startupPrev = false;
    nvLightning.prevCount = -1;
    nvLightning.events = -1;
    nvLightning.distance = 0;
    nvLightning.timestamp = 0;
}

void
Lightning::hist_init(int16_t count)
{
    for (int i=0; i<LIGHTNING_HIST_SIZE; i++) {
        nvLightning.hist[i] = count;
    }
}

#if defined(LIGHTNING_USE_PREFS)
void Lightning::prefs_load(void)
{
    preferences.begin("BWS-LGT", false);
    nvLightning.lastUpdate   = preferences.getULong64("lastUpdate", 0);
    nvLightning.startupPrev  = preferences.getBool("startupPrev", false);
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
    log_d("Preferences: lastUpdate   =%s", String(nvLightning.lastUpdate).c_str());
    log_d("Preferences: startupPrev  =%d", nvLightning.startupPrev);
    log_d("Preferences: prevCount    =%d", nvLightning.prevCount);
    log_d("Preferences: events       =%d", nvLightning.events);
    log_d("Preferences: distance     =%d", nvLightning.distance);
    log_d("Preferences: timestamp    =%s", String(nvLightning.timestamp).c_str());
    preferences.end();
}

void Lightning::prefs_save(void)
{
    preferences.begin("BWS-LGT", false);
    preferences.putULong64("lastUpdate", nvLightning.lastUpdate);
    preferences.putBool("startupPrev", nvLightning.startupPrev);
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
    #if defined(LIGHTNING_USE_PREFS)
        prefs_load();
    #endif

    // No previous count OR startup change 0->1 detected
    if ((nvLightning.prevCount == -1) || (!nvLightning.startupPrev && startup)) {
        // Initialize histogram
        hist_init();

        nvLightning.prevCount = count;
        nvLightning.lastUpdate = timestamp;
        nvLightning.startupPrev = startup;

        #if defined(LIGHTNING_USE_PREFS)
            prefs_save();
        #endif
        return;
    }
    
    int16_t delta = 0;
    if (count < nvLightning.prevCount) {
        delta = count + LIGHTNINGCOUNT_MAX_VALUE - nvLightning.prevCount;
    } else {
        delta = count - nvLightning.prevCount;
    }

    if (delta > 0) {
        // Save detected event
        nvLightning.events = delta;
        nvLightning.distance = distance;
        nvLightning.timestamp = timestamp;
    }


    // Delta time between last update and current time
    
    // 0 < t_delta < 2 * LIGHTNUNG_UPDATE_RATE                  -> update history
    // 2 * LIGHTNING_UPDATE_RATE <= t_delta < LIGHTNING_HIST_SIZE * LIGHTNING_UPDATE_RATE
    //                                                          -> update history, mark missing history entries as invalid
    time_t t_delta = timestamp - nvLightning.lastUpdate;
    
    // t_delta < 0: something is wrong, e.g. RTC was not set correctly -> keep or reset history (TBD)
    if (t_delta < 0) {
        log_w("Negative time span since last update!?");
        nvLightning.prevCount = count;
        nvLightning.lastUpdate = timestamp;
        nvLightning.startupPrev = startup;
        #if defined(LIGHTNING_USE_PREFS)
            prefs_save();
        #endif
        return; 
    }

    // t_delta >= LIGHTNING_HIST_SIZE * LIGHTNING_UPDATE_RATE -> reset history
    if (t_delta >= LIGHTNING_HIST_SIZE * LIGHTNING_UPD_RATE * 60) {
        log_w("History time frame expired, resetting!");
        hist_init();
    }

    struct tm timeinfo;

    localtime_r(&timestamp, &timeinfo);
    int min = timeinfo.tm_min;
    int idx = min / LIGHTNING_UPD_RATE;

    if (t_delta / 60 < 2 * LIGHTNING_UPD_RATE) {
        // Add entry to history or update entry
        nvLightning.hist[idx] = delta;
        log_d("hist[%d]=%d", idx, delta);
    }

    // Mark all history entries in interval [expected_index, current_index) as invalid
    // N.B.: excluding current index!
    for (time_t ts = nvLightning.lastUpdate + (LIGHTNING_UPD_RATE * 60); ts < timestamp; ts += LIGHTNING_UPD_RATE * 60) {
        localtime_r(&ts, &timeinfo);
        int min = timeinfo.tm_min;
        int idx = min / LIGHTNING_UPD_RATE;
        nvLightning.hist[idx] = -1;
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

    nvLightning.prevCount = count;
    nvLightning.lastUpdate = timestamp;
    nvLightning.startupPrev = startup;
    #if defined(LIGHTNING_USE_PREFS)
        prefs_save();
    #endif
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

bool
Lightning::pastHour(int &events)
{
    bool res = false;
    int sum = 0;

    // Sum of all valid entries
    for (size_t i=0; i<LIGHTNING_HIST_SIZE; i++){
        if (nvLightning.hist[i] != -1) {
            res = true;
            sum += nvLightning.hist[i];
        }
    }
    events = sum;
    return res;
}
