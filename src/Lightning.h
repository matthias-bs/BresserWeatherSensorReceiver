///////////////////////////////////////////////////////////////////////////////////////////////////
// Lightning.h
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
//     * Number of strikes during past 60 minutes (TBD)
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
// - Handle sensor counter overflow and startup flag
// - Number of strikes during past 60 minutes
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "time.h"
#if defined(ESP32) || defined(ESP8266)
  #include <sys/time.h>
#endif

/**
 * \def
 * 
 * Set to the value which leads to a reset of the lightning sensor counter output to zero (overflow).
 */
#define LIGHTNINGCOUNT_MAX_VALUE 100

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
 * \class Lightning
 *
 * \brief Calculation number of lightning events during last sensor update cycle and 
 *        during last hour (past 60 minutes); storing timestamp and distance of last event.
 */
class Lightning {
public:
    float countCurr;
    
    Lightning() {};
    
    /**
     * Initialize/reset non-volatile data
     */
    void  reset(void);
    
    
    /**
     * Initialize memory for hourly (past 60 minutes) events
     * 
     * \param count     accumulated number of events
     */
    void  init(uint16_t count);
    
    
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
    void  update(time_t timestamp, int count, uint8_t distance, bool startup = false /*, uint16_t lightningCountMax = LIGHTNINGCOUNT_MAX */);
    
    
    /**
     * \fn pastHour
     * 
     * \brief Get number of lightning events during past 60 minutes
     * 
     * \return number of lightning events during past 60 minutes
     */
    int pastHour(time_t timestamp);

    /*
     * \fn lastCycle
     * 
     * \brief Get number of events during last update cycle with detected lightning events
     * 
     * \return number of lightning events
     */
    int lastCycle(void);

    bool lastEvent(time_t &timestamp, int &events, uint8_t &distance);

private:
    inline int inc(int x)
    {
        return ((x + 1) == LIGHTNING_HIST_SIZE) ? 0 : ++x;
    }

    inline int dec(int x)
    {
        return (x == 0) ? (LIGHTNING_HIST_SIZE - 1) : --x;
    }
};
