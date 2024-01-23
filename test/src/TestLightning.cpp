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
  tm = {0};
  strptime(time, "%Y-%m-%d %H:%M", &tm);
  tm.tm_isdst = -1;
  ts = mktime(&tm);
}

TEST_GROUP(TG_LightningBasic) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TG_LightningHourly) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TG_LightningDouble) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TG_LightningSkip) {
  void setup() {
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
  time_t    exp_ts;
  int       res_events;
  uint8_t   res_distance;
  Lightning lightning;
  
  printf("< LightningBasic >\n");
  
  setTime("2023-07-22 8:00", tm, ts);
  lightning.update(ts, 48, 5);
  res = lightning.lastEvent(res_ts, res_events, res_distance);
  CHECK(res);

  // Step 1
  setTime("2023-07-22 8:06", tm, ts);
  lightning.update(ts, 50, 7);
  res = lightning.lastEvent(res_ts, res_events, res_distance);
  CHECK(res);
  CHECK_EQUAL(exp_ts=ts, res_ts);
  CHECK_EQUAL(2, res_events);
  CHECK_EQUAL(7, res_distance);

  // Step 2
  // Counter not changed
  // Same state as State 1
  setTime("2023-07-22 8:12", tm, ts);
  lightning.update(ts, 50, 12);
  res = lightning.lastEvent(res_ts, res_events, res_distance);
  CHECK(res);
  CHECK_EQUAL(exp_ts, res_ts);
  CHECK_EQUAL(2, res_events);
  CHECK_EQUAL(7, res_distance);

  // Step 3
  // Counter +5, Distance 30
  setTime("2023-07-22 8:18", tm, ts);
  lightning.update(ts, 55, 30);
  res = lightning.lastEvent(res_ts, res_events, res_distance);
  CHECK(res);
  CHECK_EQUAL(ts, res_ts);
  CHECK_EQUAL(5, res_events);
  CHECK_EQUAL(30, res_distance);

  // Step 4
  // Reset
  setTime("2023-07-22 8:24", tm, ts);
  lightning.reset();
  res = lightning.lastEvent(res_ts, res_events, res_distance);
  CHECK_FALSE(res);
}

/*
 * Test hourly lightning events
 */
TEST(TG_LightningHourly, Test_LightningHourly) {
  tm        tm;
  time_t    ts;
  bool      res;
  int       counter;
  int       res_events;
  int       exp_events;
  Lightning lightning;

  printf("< LightningHourly >\n");
  
  setTime("2023-07-22 8:00", tm, ts);
  lightning.hist_init();
  lightning.update(ts, counter=48, 5);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events=0, res_events);

  // Step 1
  // Counter +2
  setTime("2023-07-22 8:06", tm, ts);
  counter += 2;
  exp_events += 2;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 2
  // Counter +3
  setTime("2023-07-22 8:12", tm, ts);
  counter += 3;
  exp_events += 3;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 3
  // Counter +4
  setTime("2023-07-22 8:18", tm, ts);
  counter += 4;
  exp_events += 4;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 4
  // Counter +5
  setTime("2023-07-22 8:24", tm, ts);
  counter += 5;
  exp_events += 5;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 5
  // Counter +6
  setTime("2023-07-22 8:30", tm, ts);
  counter += 6;
  exp_events += 6;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 6
  // Counter +7
  setTime("2023-07-22 8:36", tm, ts);
  counter += 7;
  exp_events += 7;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 7
  // Counter +8
  setTime("2023-07-22 8:42", tm, ts);
  counter += 8;
  exp_events += 8;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 8
  // Counter +9
  setTime("2023-07-22 8:48", tm, ts);
  counter += 9;
  exp_events += 9;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 9
  // Counter +10
  setTime("2023-07-22 8:54", tm, ts);
  counter += 10;
  exp_events += 10;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 10
  // Counter +11
  setTime("2023-07-22 9:00", tm, ts);
  counter += 11;
  exp_events += 11;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 11
  // Counter +12
  // Events from Step 1 are discarded!
  setTime("2023-07-22 9:06", tm, ts);
  counter += 12;
  exp_events += 12;
  exp_events -= 2;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK(res);
  CHECK_EQUAL(exp_events, res_events);

  // Step 12
  // Counter +13
  // Events from Step 2 are discarded!
  setTime("2023-07-22 9:12", tm, ts);
  counter += 13;
  exp_events += 13;
  exp_events -= 3;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour(&res);
  CHECK(res);
  CHECK_EQUAL(exp_events, res_events);
}


/*
 * Test hourly lightning events
 * Two updates during the same time slot
 */
TEST(TG_LightningDouble, Test_LightningDouble) {
  tm        tm;
  time_t    ts;
  bool      res;
  int       counter;
  int       res_events;
  int       exp_events;
  Lightning lightning;

  printf("< LightningDouble >\n");
  
  setTime("2023-07-22 8:00", tm, ts);
  lightning.hist_init();
  lightning.update(ts, counter=48, 5);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events=0, res_events);

  // Step 1
  // Counter +2
  setTime("2023-07-22 8:06", tm, ts);
  counter = 50;
  exp_events = 2;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 2
  // Counter +2
  setTime("2023-07-22 8:06", tm, ts);
  counter = 53;
  exp_events = 5;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);
}


/*
 * Test hourly lightning events
 * Deliberately skip entry at 8:06
 */
TEST(TG_LightningSkip, Test_LightningSkip) {
  tm        tm;
  time_t    ts;
  bool      res;
  int       counter;
  int       res_events;
  int       exp_events;
  Lightning lightning;

  printf("< LightningSkip >\n");
  
  setTime("2023-07-22 8:00", tm, ts);
  lightning.hist_init();
  lightning.update(ts, counter=48, 5);
  res_events = lightning.pastHour(&res);
  CHECK_FALSE(res);
  CHECK_EQUAL(exp_events=0, res_events);

  // Step 1
  // Counter +2
  setTime("2023-07-22 8:06", tm, ts);
  counter += 2;
  exp_events += 2;
  // ---
  // No update!!!
  // ---

  // Step 2
  // Counter +3
  setTime("2023-07-22 8:12", tm, ts);
  counter += 3;
  exp_events += 3;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 3
  // Counter +4
  setTime("2023-07-22 8:18", tm, ts);
  counter += 4;
  exp_events += 4;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 4
  // Counter +5
  setTime("2023-07-22 8:24", tm, ts);
  counter += 5;
  exp_events += 5;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 5
  // Counter +6
  setTime("2023-07-22 8:30", tm, ts);
  counter += 6;
  exp_events += 6;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 6
  // Counter +7
  setTime("2023-07-22 8:36", tm, ts);
  counter += 7;
  exp_events += 7;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 7
  // Counter +8
  setTime("2023-07-22 8:42", tm, ts);
  counter += 8;
  exp_events += 8;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 8
  // Counter +9
  setTime("2023-07-22 8:48", tm, ts);
  counter += 9;
  exp_events += 9;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 9
  // Counter +10
  setTime("2023-07-22 8:54", tm, ts);
  counter += 10;
  exp_events += 10;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 10
  // Counter +11
  // Events from Step 0 (was 0) are replaced!
  setTime("2023-07-22 9:00", tm, ts);
  counter += 11;
  exp_events += 11;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 11
  // Counter +12
  // Events from Step 1 (was skipped) are replaced!
  setTime("2023-07-22 9:06", tm, ts);
  counter += 12;
  exp_events += 12;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 12
  // Counter +13
  // Events from Step 2 are discarded!
  setTime("2023-07-22 9:12", tm, ts);
  counter += 13;
  exp_events += 13;
  exp_events -= 3;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);

  // Step 13
  // Counter +14
  // Events from Step 3 are discarded!
  setTime("2023-07-22 9:12", tm, ts);
  counter += 14;
  exp_events += 14;
  exp_events -= 4;
  lightning.update(ts, counter, 7);
  res_events = lightning.pastHour();
  CHECK_EQUAL(exp_events, res_events);
}
