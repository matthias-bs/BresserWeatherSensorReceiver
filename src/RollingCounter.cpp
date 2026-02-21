///////////////////////////////////////////////////////////////////////////////////////////////////
// RollingCounter.cpp
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

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "RollingCounter.h"

int 
RollingCounter::calculateIndex(const struct tm& tm, uint8_t rate) const
{
    if (rate >= 60) {
        // Hourly or greater - use hour of day (0-23)
        return tm.tm_hour;
    } else {
        // Sub-hourly - use minute within hour divided by rate
        return tm.tm_min / rate;
    }
}

void 
RollingCounter::markMissedEntries(int16_t* hist, size_t size, time_t lastUpdate, 
                                  time_t timestamp, uint8_t rate)
{
    // Guard against invalid rate values to avoid division by zero
    if (rate == 0) {
        log_w("markMissedEntries called with invalid rate=0, skipping history update");
        return;
    }

    // Mark all history entries in interval [expected_index, current_index) as invalid
    // N.B.: excluding current index!
    for (time_t ts = lastUpdate + (rate * 60); ts < timestamp; ts += rate * 60) {
        struct tm timeinfo;
        localtime_r(&ts, &timeinfo);
        int idx = calculateIndex(timeinfo, rate);
        
        // Use provided size to guard against out-of-bounds writes
        if (idx < 0 || static_cast<size_t>(idx) >= size) {
            log_w("markMissedEntries: computed index %d out of bounds (size=%u, hour=%d, minute=%d, rate=%u)",
                  idx, static_cast<unsigned>(size), timeinfo.tm_hour, timeinfo.tm_min, rate);
            continue;
        }
        
        hist[idx] = -1;
        log_d("hist[%d]=-1", idx);
    }
}

float 
RollingCounter::sumHistory(const History& h, bool *valid, int *nbins, float *quality, float scale)
{
    int entries = 0;
    float res = 0;

    // Validate updateRate to avoid division by zero
    if (h.updateRate == 0) {
        log_w("sumHistory called with invalid updateRate=0");
        if (nbins != nullptr)
            *nbins = 0;
        if (valid != nullptr)
            *valid = false;
        if (quality != nullptr)
            *quality = 0.0f;
        return 0.0f;
    }

    // Calculate the effective number of bins based on size and update rate
    // For hourly buffer: 60 minutes / updateRate = number of bins
    // For 24h buffer: size is already correct (24 bins for 24 hours)
    size_t effectiveBins;
    if (h.updateRate == 60) {
        // 24-hour buffer: size is already the effective bin count
        effectiveBins = h.size;
    } else if (h.updateRate > 60) {
        // Invalid rate for hourly buffer, can't have update rate > 60 minutes
        log_w("sumHistory called with updateRate=%u > 60 minutes", h.updateRate);
        effectiveBins = 1; // Fallback to avoid division by zero
    } else {
        // Hourly buffer: calculate bins based on update rate
        effectiveBins = 60 / h.updateRate;
        // Constrain to actual buffer size
        if (effectiveBins > h.size) {
            effectiveBins = h.size;
        }
    }

    // Sum of all valid entries, but only count bins within the effective range
    size_t binsToCheck = (effectiveBins < h.size) ? effectiveBins : h.size;
    for (size_t i = 0; i < binsToCheck; i++){
        if (h.hist[i] >= 0) {
            res += h.hist[i] * scale;
            entries++;
        }
    }

    // Optional: return number of valid bins
    if (nbins != nullptr)
        *nbins = entries;
    
    // Optional: return valid flag
    if (valid != nullptr) {
        if (entries >= qualityThreshold * effectiveBins) {
            *valid = true;
        } else {
            *valid = false;
        }
    }

    // Optional: return quality
    if (quality != nullptr) {
        if (effectiveBins > 0) {
            *quality = static_cast<float>(entries) / effectiveBins;
        } else {
            *quality = 0.0f;
        }
    }

    return res;
}

RollingCounter::UpdateResult
RollingCounter::updateHistoryBufferCore(int16_t* hist, size_t size, int idx, int16_t delta,
                                       time_t t_delta, time_t timestamp, time_t lastUpdate,
                                       uint8_t updateRate)
{
    if (t_delta / 60 < updateRate) {
        // t_delta shorter than expected update rate
        if (hist[idx] < 0)
            hist[idx] = 0;
        struct tm t_prev;
        localtime_r(&lastUpdate, &t_prev);
        if (calculateIndex(t_prev, updateRate) == idx) {
            // same index as in previous cycle - add value
            hist[idx] += delta;
            log_d("hist[%d]=%d (upd)", idx, hist[idx]);
        } else {
            // different index - new value
            hist[idx] = delta;
            log_d("hist[%d]=%d (new)", idx, hist[idx]);
        }
        return UPDATE_SUCCESS;
    }
    else if (t_delta >= size * updateRate * 60) {
        // t_delta >= HIST_SIZE * UPDATE_RATE -> reset history
        log_w("History time frame expired, resetting!");
        return UPDATE_EXPIRED;
    }
    else {
        // Some other index
        
        // Mark missed entries
        markMissedEntries(hist, size, lastUpdate, timestamp, updateRate);
        
        // Write delta
        hist[idx] = delta;
        log_d("hist[%d]=%d", idx, delta);
        return UPDATE_SUCCESS;
    }
}

void
RollingCounter::updateHistoryBuffer(int16_t* hist, size_t size, int idx, int16_t delta,
                                   time_t t_delta, time_t timestamp, time_t lastUpdate,
                                   uint8_t updateRate)
{
    UpdateResult result = updateHistoryBufferCore(hist, size, idx, delta, t_delta, 
                                                  timestamp, lastUpdate, updateRate);
    if (result == UPDATE_EXPIRED) {
        hist_init();
    }
}
