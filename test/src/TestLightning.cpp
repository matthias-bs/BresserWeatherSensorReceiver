///////////////////////////////////////////////////////////////////////////////////////////////////
// TestLightning.cpp
//
// CppUTest unit tests for Lightning - artificial test cases
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
// 20230722 Created
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CppUTest/TestHarness.h"

#include "Lightning.h"



static Lightning lightning;

/**
 * \example
 * struct tm tm;
 * time_t t;
 * strptime("6 Dec 2001 12:33:45", "%d %b %Y %H:%M:%S", &tm);
 * tm.tm_isdst = -1;      // Not set by strptime(); tells mktime()
 *                        // to determine whether daylight saving time
 *                        // is in effect
 * t = mktime(&tm);
 */

static void setTime(const char *time, tm &tm, time_t &ts)
{
  strptime(time, "%Y-%m-%d %H:%M", &tm);
  ts = mktime(&tm);
}

TEST_GROUP(TG_LightningBasic) {
  void setup() {
      lightning.reset();
  }

  void teardown() {
  }
};


/*
 * Test basic lightning functions
 */
TEST(TG_LightningBasic, Test_LightningBasic) {
  tm        tm;
  time_t    ts;
  bool      res;
  time_t    res_ts;
  int       res_events;
  uint8_t   res_distance;
  //float     rainSensor;

  printf("< LightningBasic >\n");
  
  setTime("2023-07-22 8:00", tm, ts);
  lightning.update(ts, 48, 5);
  res = lightning.lastEvent(res_ts, res_events, res_distance);
  CHECK_FALSE(res);

  setTime("2022-09-06 8:06", tm, ts);
  lightning.update(ts, 50, 7);
  res = lightning.lastEvent(res_ts, res_events, res_distance);
  CHECK(res);
  CHECK_EQUAL(ts, res_ts);
  CHECK_EQUAL(2, res_events);
  CHECK_EQUAL(7, res_distance);
}
