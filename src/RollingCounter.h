///////////////////////////////////////////////////////////////////////////////////////////////////
// RollingCounter.h
//
// Base class for rolling counter implementations (RainGauge, Lightning, etc.)
// Provides common functionality for history buffer management and calculations
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 02/2026
//
//
// MIT License
//
// Copyright (c) 2026 Matthias Prinke
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
// 20260211 Created from common code in RainGauge and Lightning
// 20260221 Improved generalization, documentation, and code deduplication
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _ROLLINGCOUNTER_H
#define _ROLLINGCOUNTER_H

#if defined(ESP32) || defined(ESP8266)
#include <sys/time.h>
#else
#include <time.h>
#include <stdint.h>
#include <stddef.h>
#endif

/**
 * \def
 *
 * Default update rate [min]
 */
#define ROLLING_COUNTER_UPD_RATE 6

/**
 * \def
 *
 * Fraction of valid entries required for valid result
 */
#define DEFAULT_QUALITY_THRESHOLD 0.8

/**
 * \class RollingCounter
 *
 * \brief Base class for rolling counter implementations
 *
 * Provides common functionality for managing rolling history buffers,
 * handling timestamps, calculating time-based aggregates with quality metrics.
 */
class RollingCounter
{
protected:
    float qualityThreshold;
    time_t lastUpdate;
    uint8_t updateRate;

    /**
     * \typedef HistInitCallback
     *
     * \brief Callback function type for history buffer initialization
     */
    typedef void (*HistInitCallback)();

    /**
     * \enum UpdateResult
     *
     * \brief Result codes for history buffer updates
     */
    enum UpdateResult
    {
        UPDATE_SUCCESS, ///< Update completed successfully
        UPDATE_EXPIRED  ///< History expired, initialization needed
    };

    /**
     * \struct History
     *
     * \brief History buffer configuration
     */
    typedef struct
    {
        int16_t *hist;      // pointer to buffer
        size_t size;        // number of bins
        uint8_t updateRate; // minutes per bin
    } History;

    /**
     * Calculate index into history buffer based on current time
     *
     * \param tm        time structure
     * \param rate      update rate in minutes
     *
     * \returns index into history buffer
     */
    int calculateIndex(const struct tm &tm, uint8_t rate) const;

    /**
     * Mark history entries as invalid for missed update cycles
     *
     * \param hist          history buffer
     * \param size          buffer size
     * \param lastUpdate    timestamp of last update
     * \param timestamp     current timestamp
     * \param rate          update rate in minutes
     */
    void markMissedEntries(int16_t *hist, size_t size, time_t lastUpdate,
                           time_t timestamp, uint8_t rate);

    /**
     * Update history buffer with new delta value (core logic without init)
     *
     * Handles three cases:
     * 1. Update within expected rate: adds or replaces value at current index
     * 2. History expired: returns UPDATE_EXPIRED (caller must handle init)
     * 3. Missed updates: marks missed entries and writes new value
     *
     * \param hist          history buffer
     * \param size          buffer size
     * \param idx           current index in history
     * \param delta         delta value to add
     * \param t_delta       time since last update (seconds)
     * \param timestamp     current timestamp
     * \param lastUpdate    timestamp of last update
     * \param updateRate    update rate in minutes
     *
     * \returns UPDATE_SUCCESS or UPDATE_EXPIRED
     */
    UpdateResult updateHistoryBufferCore(int16_t *hist, size_t size, int idx, int16_t delta,
                                         time_t t_delta, time_t timestamp, time_t lastUpdate,
                                         uint8_t updateRate);

    /**
     * Update history buffer with new delta value
     *
     * Handles three cases:
     * 1. Update within expected rate: adds or replaces value at current index
     * 2. History expired: calls hist_init() to reset
     * 3. Missed updates: marks missed entries and writes new value
     *
     * \param hist          history buffer
     * \param size          buffer size
     * \param idx           current index in history
     * \param delta         delta value to add
     * \param t_delta       time since last update (seconds)
     * \param timestamp     current timestamp
     * \param lastUpdate    timestamp of last update
     * \param updateRate    update rate in minutes
     */
    void updateHistoryBuffer(int16_t *hist, size_t size, int idx, int16_t delta,
                             time_t t_delta, time_t timestamp, time_t lastUpdate,
                             uint8_t updateRate);

    /**
     * Initialize history buffer - must be implemented by derived classes
     *
     * Called when history time frame has expired
     *
     * \param value     initial value for history entries (default: -1 = invalid)
     */
    virtual void hist_init(int16_t value = -1) = 0;

    /**
     * Sum all valid entries in a history buffer
     *
     * \param h          History buffer to sum
     * \param valid      pointer to bool indicating if result is valid (optional)
     * \param nbins      pointer to int for number of valid bins (optional)
     * \param quality    pointer to float for quality metric (optional)
     * \param scale      scaling factor to apply to values (default: 1.0)
     *
     * \returns sum of all valid entries
     */
    float sumHistory(const History &h, bool *valid = nullptr, int *nbins = nullptr,
                     float *quality = nullptr, float scale = 1.0);

public:
    /**
     * Constructor
     *
     * \param quality_threshold fraction of valid entries required for valid result
     */
    RollingCounter(const float quality_threshold = DEFAULT_QUALITY_THRESHOLD) : qualityThreshold(quality_threshold),
                                                                                lastUpdate(0),
                                                                                updateRate(ROLLING_COUNTER_UPD_RATE) {};

    /**
     * Virtual destructor for proper cleanup in derived classes
     */
    virtual ~RollingCounter() = default;

    /**
     * Get last update timestamp
     *
     * \returns last update timestamp
     */
    time_t getLastUpdate() const { return lastUpdate; }

    /**
     * Get update rate
     *
     * \returns update rate in minutes
     */
    uint8_t getUpdateRate() const { return updateRate; }
};

#endif // _ROLLINGCOUNTER_H
