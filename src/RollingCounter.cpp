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
    return tm.tm_min / rate;
}

void 
RollingCounter::markMissedEntries(int16_t* hist, size_t size, time_t lastUpdate, 
                                  time_t timestamp, uint8_t rate)
{
    (void)size; // Parameter reserved for future bounds checking
    // Mark all history entries in interval [expected_index, current_index) as invalid
    // N.B.: excluding current index!
    for (time_t ts = lastUpdate + (rate * 60); ts < timestamp; ts += rate * 60) {
        struct tm timeinfo;
        localtime_r(&ts, &timeinfo);
        int idx = timeinfo.tm_min / rate;
        hist[idx] = -1;
        log_d("hist[%d]=-1", idx);
    }
}

float 
RollingCounter::sumHistory(const History& h, bool *valid, int *nbins, float *quality, float scale)
{
    int entries = 0;
    float res = 0;

    // Calculate the effective number of bins based on size and update rate
    // For hourly buffer: 60 minutes / updateRate = number of bins
    // For 24h buffer: size is already correct (24 bins for 24 hours)
    size_t effectiveBins;
    if (h.updateRate == 60) {
        // 24-hour buffer: size is already the effective bin count
        effectiveBins = h.size;
    } else {
        // Hourly buffer: calculate bins based on update rate
        effectiveBins = 60 / h.updateRate;
    }

    // Sum of all valid entries
    for (size_t i=0; i<h.size; i++){
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
        *quality = static_cast<float>(entries) / effectiveBins;
    }

    return res;
}
