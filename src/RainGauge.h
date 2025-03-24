///////////////////////////////////////////////////////////////////////////////////////////////////
// RainGauge.h
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
// 20230330 Added changes for Adafruit Feather 32u4 LoRa Radio
// 20230716 Implemented sensor startup handling
// 20230817 Implemented partial reset
// 20231227 Added prerequisites for storing rain data in preferences
// 20231218 Implemented storing of rain data in preferences, 
//          new algotrithm for past 60 minutes rainfall
// 20240118 Changed raingaugeMax to class member set by constructor
// 20240119 Changed preferences to class member
//          Modified update at the same index as before
//          Modified pastHour() algorithm and added features
//          Changed hist[] width
// 20240120 Added set_max()
//          Removed old implementation
// 20240122 Changed scope of nvData -
//          Using RTC RAM: global
//          Using Preferences, Unit Tests: class member
// 20250323 Added configuration of expected update rate at run-time
//          pastHour(): modified parameters
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _RAINGAUGE_H
#define _RAINGAUGE_H

#include "time.h"
#if defined(ESP32) || defined(ESP8266)
  #include <sys/time.h>
#endif
#if defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
    #include <Preferences.h>
#endif

/**
 * \def
 * 
 * Set to the value which leads to a reset of the rain gauge output to zero.
 */
#define RAINGAUGE_MAX_VALUE 1000
//#define RAINGAUGE_MAX_VALUE 20000

/**
 * \def
 * 
 * Lightning sensor update rate [min]
 */
#define RAINGAUGE_UPD_RATE 6

/**
 * \def
 * 
 * Set to 3600 [sec] / min_update_rate_rate [sec]
 */
#define RAIN_HIST_SIZE 10


/**
 * \def
 * 
 * Fraction of valid rain_hist entries required for valid result
 */
#define DEFAULT_QUALITY_THRESHOLD 0.8

/**
 * \defgroup Reset rain counters
 */
 #define RESET_RAIN_H 1
 #define RESET_RAIN_D 2
 #define RESET_RAIN_W 4
 #define RESET_RAIN_M 8


/**
 * \typedef nvData_t
 *
 * \brief Data structure for rain statistics to be stored in non-volatile memory
 */
typedef struct {
    /* Timestamp of last update */
    time_t    lastUpdate;

    /* Data of past 60 minutes */
    int16_t   hist[RAIN_HIST_SIZE];

    /* Sensor startup handling */
    bool      startupPrev; // previous state of startup
    float     rainPreStartup; // previous rain gauge reading (before startup)

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
    float     rainAcc; // accumulated rain (overflows and startups)

    uint8_t   updateRate; // update rate for pastHour() calculation
} nvData_t;

/**
 * \class RainGauge
 *
 * \brief Calculation of hourly (past 60 minutes), daily, weekly and monthly rainfall
 *
 * Additionally overflow of the rain gauge is handled when reaching RAINGAUGE_MAX_VALUE. 
 */
class RainGauge {
private:
    float rainCurr;
    float raingaugeMax;
    float qualityThreshold;

    #if defined(RAINGAUGE_USE_PREFS) || defined(INSIDE_UNITTEST)
    nvData_t nvData = {
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
        .rainAcc = 0,
        .updateRate = RAINGAUGE_UPD_RATE
    };
    #endif
    #if defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
    Preferences preferences;
    #endif

public:
    /**
     * Constructor
     * 
     * \param raingauge_max     raingauge value which causes a counter overflow
     * \param quality_threshold fraction of valid rain_hist entries required for valid pastHour() result
     */
    RainGauge(const float raingauge_max = RAINGAUGE_MAX_VALUE, const float quality_threshold = DEFAULT_QUALITY_THRESHOLD) :
        raingaugeMax(raingauge_max),
        qualityThreshold(quality_threshold)
    {};

    /**
     * Set maximum rain counter value
     * 
     * \param raingauge_max     raingauge value which causes a counter overflow
     */
    void set_max(float raingauge_max)
    {
        raingaugeMax = raingauge_max;
    }
    
    /**
     * \brief Set expected update rate for pastHour() calculation
     * 
     * RAIN_HIST_SIZE: number of entries in rain_hist[]
     * updateRate: update rate in minutes
     * 
     * 60 minutes / updateRate = no_of_hist_bins
     * The resulting number of history bins must be an integer value which
     * does not exceed RAIN_HIST_SIZE.
     * 
     * Examples: 
     * 
     * 1. updateRate =  6 -> 60 / 6 = 10 entries
     * 2. updateRate = 12 -> 60 / 12 = 5 entries
     * 
     * Changing the update rate will reset the history buffer, therefore
     * the caller should avoid frequent changes.
     * 
     * Actual update intervals shorter than updateRate will lead to a reduced
     * resolution of the pastHour() result and a higher risk of an invalid
     * result if a bin in the history buffer was missed.
     * 
     * Actual update intervals longer than updateRate will lead to an invalid
     * result, because bins in the history buffer will be missed.
     * 
     * \param rate    update rate in minutes (default: 6)
     */
    void setUpdateRate(uint8_t rate = RAINGAUGE_UPD_RATE) {
        #if !defined(INSIDE_UNITTEST)
        preferences.begin("BWS-RAIN", false);
        uint8_t updateRatePrev = preferences.getUChar("updateRate", RAINGAUGE_UPD_RATE);
        preferences.putUChar("updateRate", rate);
        preferences.end();
        #else
        static uint8_t updateRatePrev = RAINGAUGE_UPD_RATE;
        updateRatePrev = nvData.updateRate;
        #endif
        nvData.updateRate = rate;
        if (nvData.updateRate != updateRatePrev) {
            hist_init();
        }
    }

    /**
     * Reset non-volatile data and current rain counter value
     * 
     * \param flags Flags defining what to reset:
     */
    void reset(uint8_t flags=0xF);
    
    /**
     * Initialize history buffer for hourly (past 60 minutes) rainfall
     */
    void hist_init(int16_t rain = -1);

    #if defined(RAINGAUGE_USE_PREFS) && !defined(INSIDE_UNITTEST)
    void prefs_load(void);
    void prefs_save(void);
    #endif

    /**
     * \fn update
     * 
     * \brief Update rain gauge statistics
     * 
     * \param ts           timestamp
     * 
     * \param rain         rain gauge raw value (in mm/m²)
     * 
     * \param startup      sensor startup flag
     */
    void  update(time_t ts, float rain, bool startup = false);
    
    /**
     * Rainfall during past 60 minutes
     * 
     * \param valid     number of valid entries in rain_hist >= qualityThreshold * 60 / updateRate
     * \param nbins     number of valid entries in rain_hist
     * \param quality   fraction of valid entries in rain_hist (0..1); quality = nbins / (60 / updateRate)
     * 
     * \returns amount of rain during past 60 minutes
     */
    float pastHour(bool *valid = nullptr, int *nbins = nullptr, float *quality = nullptr);

    /**
     * Rainfall of current calendar day
     * 
     * \returns amount of rain
     */
    float currentDay(void);
    
    /**
     * Rainfall of current calendar week
     * 
     * \returns amount of rain
     */
    float currentWeek(void);
    
    /**
     * Rainfall of current calendar month
     * 
     * \returns amount of rain
     */
    float currentMonth(void);
};
#endif // _RAINGAUGE_H