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
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "time.h"
#if defined(ESP32) || defined(ESP8266)
  #include <sys/time.h>
#endif

/**
 * \def
 * 
 * Set to the value which leads to a reset of the rain gauge output to zero.
 */
#define RAINGAUGE_MAX_VALUE 100
//#define RAINGAUGE_MAX_VALUE 20000

/**
 * \def
 * 
 * Set to (3600 [sec] / update_rate_rate [sec]) + 1
 */ 
#define RAINGAUGE_BUF_SIZE 11

/**
 * \defgroup Reset rain counters
 */
 #define RESET_RAIN_H 1
 #define RESET_RAIN_D 2
 #define RESET_RAIN_W 4
 #define RESET_RAIN_M 8

/**
 * \def
 * Enable printing of circular buffer data and head/tail indices.
 */
//#define _DEBUG_CIRCULAR_BUFFER_


/**
 * \class RainGauge
 *
 * \brief Calculation of hourly (past 60 minutes), daily, weekly and monthly rainfall
 *
 * Additionally overflow of the rain gauge is handled when reaching RAINGAUGE_MAX_VALUE. 
 */
class RainGauge {
public:
    float rainCurr;
    
    RainGauge() {};
#ifdef _DEBUG_CIRCULAR_BUFFER_
    /**
     * Print circular buffer for rainfall of past 60 minutes
     */
    void  printCircularBuffer(void);
#endif
    
    /**
     * Reset non-volatile data and current rain counter value
     */
    void  reset(uint8_t flags=0xF);
    
    
    /**
     * Initialize circular buffer for hourly (past 60 minutes) rainfall
     */
    void  init(tm t, float rain);
    
    
    /**
     * \fn update
     * 
     * \brief Update rain gauge statistics
     * 
     * \param timeinfo     date and time (struct tm)
     * 
     * \param rain         rain gauge raw value
     * 
     * \param startup      sensor startup flag
     * 
     * \param rainGaugeMax overflow value; when reached, the rain gauge is reset to zero
     */  
    void  update(tm timeinfo, float rain, bool startup = false, float raingaugeMax = RAINGAUGE_MAX_VALUE);
    
    
    /**
     * Rainfall during past 60 minutes
     */
    float pastHour(void);
    
    /**
     * Rainfall of current calendar day
     */
    float currentDay(void);
    
    /**
     * Rainfall of current calendar week
     */
    float currentWeek(void);
    
    /**
     * Rainfall of current calendar month
     */
    float currentMonth(void);
    
private:
    /**
     * Calculate seconds since midnight from given time and date
     *
     * \param t date and time (struct tm)
     * 
     * \returns Seconds since midnight
     */
    uint32_t timeStamp(tm t);
};
