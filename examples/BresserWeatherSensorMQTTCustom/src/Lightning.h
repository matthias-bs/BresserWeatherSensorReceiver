///////////////////////////////////////////////////////////////////////////////////////////////////
// Lightning.h
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
// 20240116 Corrected LIGHTNINGCOUNT_MAX_VALUE
// 20240119 Changed preferences to class member
// 20240123 Changed scope of nvLightning -
//          Using RTC RAM: global
//          Using Preferences, Unit Tests: class member
//          Modified for unit testing
//          Modified pastHour()
//          Added qualityThreshold
// 20240124 Fixed handling of overflow, startup and missing update cycles
// 20240125 Added lastCycle()
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "time.h"
#if defined(ESP32) || defined(ESP8266)
  #include <sys/time.h>
#endif
#include "WeatherSensorCfg.h"

#if defined(LIGHTNING_USE_PREFS)
#include <Preferences.h>
#endif



/**
 * \def
 * 
 * Set to the value which leads to a reset of the lightning sensor counter output to zero (overflow).
 */
#define LIGHTNINGCOUNT_MAX_VALUE 1600

/**
 * \def
 * 
 * Lightning sensor update rate [min]
 */
#define LIGHTNING_UPD_RATE 6

/**
 * \def
 * 
 * Set to 3600 [sec] / update_rate_rate [sec]
 */
#define LIGHTNING_HIST_SIZE 10

/**
 * \def
 * 
 * Number of valid rain_hist entries required for valid result
 */
#define DEFAULT_QUALITY_THRESHOLD 8


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
    int16_t   preStCount;   //!< Previous raw sensor counter (before startup)
    uint32_t  accCount;     //!< Accumulated counts (overflows and startups)

    /* Data of last lightning event */
    int16_t   prevCount;    //!< Previous counter value
    int16_t   events;       //!< Number of events reported at last event
    uint8_t   distance;     //!< Distance at last event
    time_t    timestamp;    //!< Timestamp of last event

    /* Data of past 60 minutes */
    int16_t   hist[LIGHTNING_HIST_SIZE];

} nvLightning_t;


/**
 * \class Lightning
 *
 * \brief Calculation number of lightning events during last sensor update cycle and 
 *        during last hour (past 60 minutes); storing timestamp and distance of last event.
 */
class Lightning {

private:
    int qualityThreshold;
    int currCount;
    int deltaEvents = -1;

    #if defined(LIGHTNING_USE_PREFS) || defined(INSIDE_UNITTEST)
    nvLightning_t nvLightning = {
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
    
    #if defined(LIGHTNING_USE_PREFS) && !defined(INSIDE_UNITTEST)
    Preferences preferences;
    #endif

public:
    /**
     * Constructor
     *
     * \param quality_threshold number of valid hist entries required for valid pastHour() result
     */
    Lightning(const int quality_threshold = DEFAULT_QUALITY_THRESHOLD) :
        qualityThreshold(quality_threshold)
    {};
    
    /**
     * Initialize/reset non-volatile data
     */
    void  reset(void);
    
    
    /**
     * Initialize histogram of hourly (past 60 minutes) events
     * 
     * \param count     number of events
     */
    void  hist_init(int16_t count = -1);
    
    #if defined(LIGHTNING_USE_PREFS)  && !defined(INSIDE_UNITTEST)
    void prefs_load(void);
    void prefs_save(void);
    #endif

    /**
     * \fn update
     * 
     * \brief Update lightning data
     * 
     * \param timestamp         timestamp (epoch)
     * 
     * \param count             accumulated number of events
     * 
     * \param startup           sensor startup flag
     * 
     * \param lightningCountMax overflow value; when reached, the sensor's counter is reset to zero
     */  
    void update(time_t timestamp, int16_t count, uint8_t distance, bool startup = false /*, uint16_t lightningCountMax = LIGHTNINGCOUNT_MAX */);
    
    
    /**
     * \fn pastHour
     * 
     * \brief Get number of lightning events during past 60 minutes
     * 
     * \param valid     number of valid entries in hist >= qualityThreshold
     * \param quality   number of valid entries in hist
     * 
     * \return number of events during past 60 minutes
     */
    int pastHour(bool *valid = nullptr, int *quality = nullptr);

    /*
     * \fn lastCycle
     * 
     * \brief Get number of events during last update cycle
     * 
     * \return number of lightning events
     */
    int lastCycle(void);

    /*
     * \fn lastEvent
     *
     * \brief Get data of last lightning event
     * 
     * \param timestamp     timestamp of last event
     * \param events        number of lightning strikes
     * \param distance      estimated distance
     * 
     * \return true if valid    
     */
    bool lastEvent(time_t &timestamp, int &events, uint8_t &distance);
};
