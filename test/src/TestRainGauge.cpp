///////////////////////////////////////////////////////////////////////////////////////////////////
// TestRainGauge.cpp
//
// CppUTest unit tests for RainGauge - artificial test cases
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 09/2022
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
// 20240124 Fixed setTime(), fixed test cases / adjusted test cases to new algorithm
// 20250323 Added tests for changing update rate (effective history buffer size) at run-time
//          Updated tests for modified pastHour() return values
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CppUTest/TestHarness.h"

#define TOLERANCE 0.1
#define TOLERANCE_QUAL 0.001
#include "RainGauge.h"

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

TEST_GROUP(TestRainGaugeHour) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourTimeBack) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourShortInterval) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourLongInterval) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourExtremeInterval) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeDaily) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeWeekly) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeMonthly) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourOv) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourOvMidnight) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourRate10) {
  void setup() {
  }

  void teardown() {
  } 
};

TEST_GROUP(TestRainGaugeDailyOv) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeWeeklyOv) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeMonthlyOv) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeStartup) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeInvReq) {
  void setup() {
  }

  void teardown() {
  }
};


/*
 * Test rainfall during past hour (no rain gauge overflow)
 */
TEST(TestRainGaugeHour, Test_RainHour) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;
  bool      val;
  int       nbins;
  float     qual;

  printf("< RainHour >\n");
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(1, nbins);
  DOUBLES_EQUAL(0.1, qual, TOLERANCE_QUAL);

  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(ts, rainSensor=10.1);
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(2, nbins);
  DOUBLES_EQUAL(0.2, qual, TOLERANCE_QUAL);

  setTime("2022-09-06 8:12", tm, ts);
  rainGauge.update(ts, rainSensor=10.3);
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(3, nbins);

  setTime("2022-09-06 8:18", tm, ts);
  rainGauge.update(ts, rainSensor=10.6);
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(4, nbins);
  
  setTime("2022-09-06 8:24", tm, ts);
  rainGauge.update(ts, rainSensor=11.0);
  DOUBLES_EQUAL(1.0, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(5, nbins);

  setTime("2022-09-06 8:30", tm, ts);
  rainGauge.update(ts, rainSensor=11.5);
  DOUBLES_EQUAL(1.5, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(6, nbins);

  setTime("2022-09-06 8:36", tm, ts);
  rainGauge.update(ts, rainSensor=12.1);
  DOUBLES_EQUAL(2.1, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(7, nbins);

  setTime("2022-09-06 8:42", tm, ts);
  rainGauge.update(ts, rainSensor=12.8);
  DOUBLES_EQUAL(2.8, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(8, nbins);
  
  setTime("2022-09-06 8:48", tm, ts);
  rainGauge.update(ts, rainSensor=13.6);
  DOUBLES_EQUAL(3.6, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(9, nbins);

  setTime("2022-09-06 8:54", tm, ts);
  rainGauge.update(ts, rainSensor=14.5);
  DOUBLES_EQUAL(4.5, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(10, nbins);
  
  setTime("2022-09-06 9:00", tm, ts);
  rainGauge.update(ts, rainSensor=15.5);
  DOUBLES_EQUAL(5.5, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(10, nbins);
  
  setTime("2022-09-06 9:06", tm, ts);
  rainGauge.update(ts, rainSensor=16.6);
  DOUBLES_EQUAL(16.6 - 10.1, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(10, nbins);
  
  setTime("2022-09-06 9:12", tm, ts);
  rainGauge.update(ts, rainSensor=17.8);
  DOUBLES_EQUAL(17.8 - 10.3, rainGauge.pastHour(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(10, nbins);
}


/*
 * Test rainfall during past hour - time jumping back
 */
TEST(TestRainGaugeHourTimeBack, Test_RainHourTimeBack) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainHourTimeBack >\n");
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(ts, rainSensor=10.1);
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, rainSensor=10.1);
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 8:12", tm, ts);
  rainGauge.update(ts, rainSensor=10.3);
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:18", tm, ts);
  rainGauge.update(ts, rainSensor=10.6);
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test rainfall during past hour (no rain gauge overflow),
 * short update interval (5 minutes)
 */
TEST(TestRainGaugeHourShortInterval, Test_RainHourShort) {
  RainGauge rainGauge(100);
  rainGauge.reset();
  
  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainHourShort >\n");
  
  setTime("2022-09-11 15:00", tm, ts);
  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:05", tm, ts);
  rainGauge.update(ts, rainSensor=10.1);
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:10", tm, ts);
  rainGauge.update(ts, rainSensor=10.3);
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:15", tm, ts);
  rainGauge.update(ts, rainSensor=10.6);
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:20", tm, ts);
  rainGauge.update(ts, rainSensor=11.0);
  DOUBLES_EQUAL(1.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:25", tm, ts);
  rainGauge.update(ts, rainSensor=11.5);
  DOUBLES_EQUAL(1.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 15:30", tm, ts);
  rainGauge.update(ts, rainSensor=12.1);
  DOUBLES_EQUAL(2.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 15:35", tm, ts);
  rainGauge.update(ts, rainSensor=12.8);
  DOUBLES_EQUAL(2.8, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:40", tm, ts);
  rainGauge.update(ts, rainSensor=13.6);
  DOUBLES_EQUAL(3.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:45", tm, ts);
  rainGauge.update(ts, rainSensor=14.5);
  DOUBLES_EQUAL(4.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:50", tm, ts);
  rainGauge.update(ts, rainSensor=15.5);
  DOUBLES_EQUAL(5.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:55", tm, ts);
  rainGauge.update(ts, rainSensor=16.6);
  DOUBLES_EQUAL(6.6, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 16:00", tm, ts);
  rainGauge.update(ts, rainSensor=17.8);
  DOUBLES_EQUAL(7.7, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 16:05", tm, ts);
  rainGauge.update(ts, rainSensor=18.8);
  DOUBLES_EQUAL(8.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 16:10", tm, ts);
  rainGauge.update(ts, rainSensor=19.9);
  DOUBLES_EQUAL(9.5, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test rainfall during past hour (no rain gauge overflow),
 * long update interval (10 minutes)
 * The ring buffer will not be filled completely.
 */
TEST(TestRainGaugeHourLongInterval, Test_RainHourLong) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainHourLong >\n");
  
  setTime("2022-09-11 15:00", tm, ts);
  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:10", tm, ts);
  rainGauge.update(ts, rainSensor=10.1);
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:20", tm, ts);
  rainGauge.update(ts, rainSensor=10.3);
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:30", tm, ts);
  rainGauge.update(ts, rainSensor=10.6);
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:40", tm, ts);
  rainGauge.update(ts, rainSensor=11.0);
  DOUBLES_EQUAL(1.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:50", tm, ts);
  rainGauge.update(ts, rainSensor=11.5);
  DOUBLES_EQUAL(1.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 16:00", tm, ts);
  rainGauge.update(ts, rainSensor=12.1);
  DOUBLES_EQUAL(2.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 16:10", tm, ts);
  rainGauge.update(ts, rainSensor=12.8);
  DOUBLES_EQUAL(12.8 - 10.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 16:20", tm, ts);
  rainGauge.update(ts, rainSensor=13.6);
  DOUBLES_EQUAL(13.6 - 10.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 16:30", tm, ts);
  rainGauge.update(ts, rainSensor=14.5);
  DOUBLES_EQUAL(14.5 - 10.6, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 16:40", tm, ts);
  rainGauge.update(ts, rainSensor=15.5);
  DOUBLES_EQUAL(15.5 - 11.0, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 16:50", tm, ts);
  rainGauge.update(ts, rainSensor=16.6);
  DOUBLES_EQUAL(16.6 - 11.5, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test rainfall during past hour (no rain gauge overflow),
 * extremely long update interval (65 minutes)
 * The distance between head and tail will be > 1h.
 */
TEST(TestRainGaugeHourExtremeInterval, Test_RainHourExtreme) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainHourExtreme >\n");
  
  setTime("2022-09-11 15:00", tm, ts);
  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 16:05", tm, ts);
  rainGauge.update(ts, rainSensor=10.1);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 17:10", tm, ts);
  rainGauge.update(ts, rainSensor=10.3);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 18:15", tm, ts);
  rainGauge.update(ts, rainSensor=10.6);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 19:20", tm, ts);
  rainGauge.update(ts, rainSensor=11.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 20:25", tm, ts);
  rainGauge.update(ts, rainSensor=11.5);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 21:40", tm, ts);
  rainGauge.update(ts, rainSensor=12.1);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);  
}


/*
 * Test rainfall during past hour (set update rate to 10 minutes and back to 6 minutes)
 */
TEST(TestRainGaugeHourRate10, Test_RainHourRate10) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;
  bool      val;
  int       nbins;
  float     qual;

  printf("< RainHourRate10 >\n");
  
  setTime("2025-03-23 8:00", tm, ts);
  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(1, nbins);
  DOUBLES_EQUAL(0.1, qual, TOLERANCE_QUAL);

  // Change expected update rate from 6 (default) to 10 minutes
  rainGauge.setUpdateRate(10);

  setTime("2025-03-23 8:10", tm, ts);
  rainGauge.update(ts, rainSensor=10.1);
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(1, nbins);
  DOUBLES_EQUAL(0.166, qual, TOLERANCE_QUAL);

  // No change in expected rate!
  rainGauge.setUpdateRate(10);

  setTime("2025-03-23 8:20", tm, ts);
  rainGauge.update(ts, rainSensor=10.3);
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(2, nbins);
  DOUBLES_EQUAL(0.333, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 8:30", tm, ts);
  rainGauge.update(ts, rainSensor=10.6);
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(3, nbins);
  DOUBLES_EQUAL(0.5, qual, TOLERANCE_QUAL);
  
  setTime("2025-03-23 8:40", tm, ts);
  rainGauge.update(ts, rainSensor=11.0);
  DOUBLES_EQUAL(1.0, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(4, nbins);
  DOUBLES_EQUAL(0.666, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 8:50", tm, ts);
  rainGauge.update(ts, rainSensor=11.5);
  DOUBLES_EQUAL(1.5, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(5, nbins);
  DOUBLES_EQUAL(0.833, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 9:00", tm, ts);
  rainGauge.update(ts, rainSensor=12.1);
  DOUBLES_EQUAL(2.1, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(6, nbins);
  DOUBLES_EQUAL(1, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 9:10", tm, ts);
  rainGauge.update(ts, rainSensor=12.8);
  DOUBLES_EQUAL(2.7, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(6, nbins);
  DOUBLES_EQUAL(1, qual, TOLERANCE_QUAL);
  
  setTime("2025-03-23 9:20", tm, ts);
  rainGauge.update(ts, rainSensor=13.6);
  DOUBLES_EQUAL(3.3, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(6, nbins);
  DOUBLES_EQUAL(1, qual, TOLERANCE_QUAL);

  // Change expected update rate from 10 to 6 minutes (default)
  rainGauge.setUpdateRate(6);

  setTime("2025-03-23 9:26", tm, ts);
  rainGauge.update(ts, rainSensor=14.5);
  DOUBLES_EQUAL(0.9, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(1, nbins);
  DOUBLES_EQUAL(0.1, qual, TOLERANCE_QUAL);
  
  setTime("2025-03-23 9:32", tm, ts);
  rainGauge.update(ts, rainSensor=15.5);
  DOUBLES_EQUAL(1.9, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(2, nbins);
  DOUBLES_EQUAL(0.2, qual, TOLERANCE_QUAL);
  
  setTime("2025-03-23 9:38", tm, ts);
  rainGauge.update(ts, rainSensor=16.6);
  DOUBLES_EQUAL(3.0, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(3, nbins);
  DOUBLES_EQUAL(0.3, qual, TOLERANCE_QUAL);
  
  setTime("2025-03-23 9:44", tm, ts);
  rainGauge.update(ts, rainSensor=17.8);
  DOUBLES_EQUAL(4.2, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(4, nbins);
  DOUBLES_EQUAL(0.4, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 9:50", tm, ts);
  rainGauge.update(ts, rainSensor=19.0);
  DOUBLES_EQUAL(5.4, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(5, nbins);
  DOUBLES_EQUAL(0.5, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 9:56", tm, ts);
  rainGauge.update(ts, rainSensor=20.3);
  DOUBLES_EQUAL(6.7, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(6, nbins);
  DOUBLES_EQUAL(0.6, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 10:00", tm, ts);
  rainGauge.update(ts, rainSensor=21.7);
  DOUBLES_EQUAL(8.1, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(7, nbins);
  DOUBLES_EQUAL(0.7, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 10:06", tm, ts);
  rainGauge.update(ts, rainSensor=23.2);
  DOUBLES_EQUAL(9.6, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(8, nbins);
  DOUBLES_EQUAL(0.8, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 10:12", tm, ts);
  rainGauge.update(ts, rainSensor=24.8);
  DOUBLES_EQUAL(11.2, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(9, nbins);
  DOUBLES_EQUAL(0.9, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 10:18", tm, ts);
  rainGauge.update(ts, rainSensor=26.5);
  DOUBLES_EQUAL(12.9, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(10, nbins);
  DOUBLES_EQUAL(1, qual, TOLERANCE_QUAL);

  setTime("2025-03-23 10:24", tm, ts);
  rainGauge.update(ts, rainSensor=28.3);
  DOUBLES_EQUAL(13.8, rainGauge.pastHour(&val, &nbins, &qual), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(10, nbins);
}


/*
 * Test daily rainfall (no rain gauge overflow)
 */
TEST(TestRainGaugeDaily, Test_RainDaily) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainDaily >\n");
  setTime("2022-09-06 8:00", tm, ts);
  DOUBLES_EQUAL(-1, rainGauge.currentDay(), TOLERANCE);

  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 12:00", tm, ts);
  rainGauge.update(ts, rainSensor=12.0);
  DOUBLES_EQUAL(2, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 16:00", tm, ts);
  rainGauge.update(ts, rainSensor=14.0);
  DOUBLES_EQUAL(4, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 20:00", tm, ts);
  rainGauge.update(ts, rainSensor=16.0);
  DOUBLES_EQUAL(6, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 23:59", tm, ts);
  rainGauge.update(ts, rainSensor=18.0);
  DOUBLES_EQUAL(8, rainGauge.currentDay(), TOLERANCE);

  // Next Day
  setTime("2022-09-07 00:00", tm, ts);
  rainGauge.update(ts, rainSensor=20.0);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);  

  setTime("2022-09-07 04:00", tm, ts);
  rainGauge.update(ts, rainSensor=22.0);
  DOUBLES_EQUAL(2, rainGauge.currentDay(), TOLERANCE);  
}


/*
 * Test weekly rainfall (no rain gauge overflow)
 */
TEST(TestRainGaugeWeekly, Test_RainWeekly) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainWeekly >\n");
  
  setTime("2022-09-06 8:00:00", tm, ts);
  DOUBLES_EQUAL(-1, rainGauge.currentWeek(), TOLERANCE);

  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);

  setTime("2022-09-06 16:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=15.0);
  DOUBLES_EQUAL(5, rainGauge.currentWeek(), TOLERANCE);
  
  setTime("2022-09-06 23:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=20.0);
  DOUBLES_EQUAL(10, rainGauge.currentWeek(), TOLERANCE);
  
  setTime("2022-09-07 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=25.0);
  DOUBLES_EQUAL(15, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-08 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=30.0);
  DOUBLES_EQUAL(20, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-09 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=35.0);
  DOUBLES_EQUAL(25, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-10 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=40.0);
  DOUBLES_EQUAL(30, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-11 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=45.0);
  DOUBLES_EQUAL(35, rainGauge.currentWeek(), TOLERANCE);  

  // Next Week
  setTime("2022-09-12 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=50.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-13 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=50.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);  
}


/*
 * Test monthly rainfall (no rain gauge overflow)
 */
TEST(TestRainGaugeMonthly, Test_RainMonthly) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor = 0;
  float     rainMonthly;
    
  printf("< RainMonthly >\n");
  
  setTime("2022-09-06 12:00:00", tm, ts);
  DOUBLES_EQUAL(-1, rainGauge.currentMonth(), TOLERANCE);

  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly = 0, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-07 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-08 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-09 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-10 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-11 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-12 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-13 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-14 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-15 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-16 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-17 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-18 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-19 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-20 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-21 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-22 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-23 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-24 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-25 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-26 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-27 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-28 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-29 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-30 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  // New month
  setTime("2022-10-01 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly = 0, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-10-02 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
}


/*
 * Test rainfall during past hour (with rain gauge overflow)
 */
TEST(TestRainGaugeHourOv, Test_RainHourOv) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;

  printf("< RainHourOv >\n");
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, 10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(ts, 10.1);
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 8:12", tm, ts);
  rainGauge.update(ts, 60.3);
  DOUBLES_EQUAL(50.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:18", tm, ts);
  rainGauge.update(ts, 0.6);
  DOUBLES_EQUAL(90.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:24", tm, ts);
  rainGauge.update(ts, 10.0);
  DOUBLES_EQUAL(100.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:30", tm, ts);
  rainGauge.update(ts, 11.5);
  DOUBLES_EQUAL(101.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 8:36", tm, ts);
  rainGauge.update(ts, 12.1);
  DOUBLES_EQUAL(102.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 8:42", tm, ts);
  rainGauge.update(ts, 92.8);
  DOUBLES_EQUAL(182.8, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 8:48", tm, ts);
  rainGauge.update(ts, 23.6);
  DOUBLES_EQUAL(213.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:54", tm, ts);
  rainGauge.update(ts, 14.5);
  DOUBLES_EQUAL(304.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:00", tm, ts);
  rainGauge.update(ts, 15.5);
  DOUBLES_EQUAL(305.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:06", tm, ts);
  rainGauge.update(ts, 5.5);
  DOUBLES_EQUAL(405.5 - 10.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:12", tm, ts);
  rainGauge.update(ts, 17.8);
  DOUBLES_EQUAL(417.8 - 60.3, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test rainfall during past hour (with rain gauge overflow),
 * timestamps across Midnight
 */
TEST(TestRainGaugeHourOvMidnight, Test_RainHourOvMidnight) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;

  printf("< RainHourOvMidnight >\n");
  
  setTime("2022-09-06 23:00", tm, ts);
  rainGauge.update(ts, 10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:06", tm, ts);
  rainGauge.update(ts, 10.1);
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 23:12", tm, ts);
  rainGauge.update(ts, 60.3);
  DOUBLES_EQUAL(50.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:18", tm, ts);
  rainGauge.update(ts, 0.6);
  DOUBLES_EQUAL(90.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:24", tm, ts);
  rainGauge.update(ts, 10.0);
  DOUBLES_EQUAL(100.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:30", tm, ts);
  rainGauge.update(ts, 11.5);
  DOUBLES_EQUAL(101.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 23:36", tm, ts);
  rainGauge.update(ts, 12.1);
  DOUBLES_EQUAL(102.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 23:42", tm, ts);
  rainGauge.update(ts, 92.8);
  DOUBLES_EQUAL(182.8, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 23:48", tm, ts);
  rainGauge.update(ts, 23.6);
  DOUBLES_EQUAL(213.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:54", tm, ts);
  rainGauge.update(ts, 14.5);
  DOUBLES_EQUAL(304.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-07 00:00", tm, ts);
  rainGauge.update(ts, 15.5);
  DOUBLES_EQUAL(305.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-07 00:06", tm, ts);
  rainGauge.update(ts, 5.5);
  DOUBLES_EQUAL(405.5 - 10.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-07 00:12", tm, ts);
  rainGauge.update(ts, 17.8);
  DOUBLES_EQUAL(417.8 - 60.3, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-07 00:18", tm, ts);
  rainGauge.update(ts, 17.8);
  DOUBLES_EQUAL(417.8 - 100.6, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test daily rainfall (with rain gauge overflow)
 */
TEST(TestRainGaugeDailyOv, Test_RainDailyOv) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;
  float     rainDaily = 0.0;

  printf("< RainDailyOv >\n");

  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, rainSensor = 0.0);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);

  rainGauge.update(ts, rainSensor += 10.0);
  DOUBLES_EQUAL(rainDaily += 10, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 12:00", tm, ts);
  rainGauge.update(ts, rainSensor += 42.0);
  DOUBLES_EQUAL(rainDaily += 42, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 16:00", tm, ts);
  rainGauge.update(ts, rainSensor =  2.0);
  DOUBLES_EQUAL(rainDaily += 50, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 20:00", tm, ts);
  rainGauge.update(ts, rainSensor += 53.0);
  DOUBLES_EQUAL(rainDaily += 53, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 23:59", tm, ts);
  rainGauge.update(ts, rainSensor = 5.0);
  DOUBLES_EQUAL(rainDaily += 50, rainGauge.currentDay(), TOLERANCE);

  // Next Day
  setTime("2022-09-07 00:00", tm, ts);
  rainGauge.update(ts, rainSensor=42.0);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);  

  setTime("2022-09-07 04:00", tm, ts);
  rainGauge.update(ts, rainSensor= 2.0);
  DOUBLES_EQUAL(60, rainGauge.currentDay(), TOLERANCE);  
}

/*
 * Test weekly rainfall (with rain gauge overflow)
 */
TEST(TestRainGaugeWeeklyOv, Test_RainWeeklyOv) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;
    
  printf("< RainWeeklyOv >\n");
   
  setTime("2022-09-06 8:00:00", tm, ts);
  DOUBLES_EQUAL(-1, rainGauge.currentWeek(), TOLERANCE);

  rainGauge.update(ts, rainSensor = 10.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);

  setTime("2022-09-06 16:00:00", tm, ts);
  rainGauge.update(ts, rainSensor =  0);
  DOUBLES_EQUAL(90, rainGauge.currentWeek(), TOLERANCE);
  
  setTime("2022-09-06 23:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 60.0);
  DOUBLES_EQUAL(150, rainGauge.currentWeek(), TOLERANCE);
  
  setTime("2022-09-07 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 20.0);
  DOUBLES_EQUAL(210, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-08 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 10.0);
  DOUBLES_EQUAL(300, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-09 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=  5.0);
  DOUBLES_EQUAL(395, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-10 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 10.0);
  DOUBLES_EQUAL(400, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-11 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 35.0);
  DOUBLES_EQUAL(425, rainGauge.currentWeek(), TOLERANCE);  

  // Next Week
  setTime("2022-09-12 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=50.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-13 04:00:00", tm, ts);
  rainGauge.update(ts, rainSensor=80.0);
  DOUBLES_EQUAL(30, rainGauge.currentWeek(), TOLERANCE);  

}


/*
 * Test monthly rainfall (no rain gauge overflow)
 */
TEST(TestRainGaugeMonthlyOv, Test_RainMonthlyOv) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor = 0;
  float     rainMonthly;
    
  printf("< RainMonthlyOv >\n");
  
  setTime("2022-09-06 12:00:00", tm, ts);
  DOUBLES_EQUAL(-1, rainGauge.currentMonth(), TOLERANCE);

  rainGauge.update(ts, rainSensor = 5);
  DOUBLES_EQUAL(rainMonthly = 0, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-07 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 50);
  DOUBLES_EQUAL(rainMonthly = 45, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-08 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 5);
  DOUBLES_EQUAL(rainMonthly = 100, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-09 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 55);
  DOUBLES_EQUAL(rainMonthly = 150, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-10 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 90);
  DOUBLES_EQUAL(rainMonthly = 185, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-11 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 55);
  DOUBLES_EQUAL(rainMonthly = 250, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-12 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 5);
  DOUBLES_EQUAL(rainMonthly = 300, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-13 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 65);
  DOUBLES_EQUAL(rainMonthly = 360, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-14 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 95);
  DOUBLES_EQUAL(rainMonthly = 390, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-15 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 0);
  DOUBLES_EQUAL(rainMonthly = 395, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-16 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 99);
  DOUBLES_EQUAL(rainMonthly = 494, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-17 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 1);
  DOUBLES_EQUAL(rainMonthly = 496, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-18 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 5);
  DOUBLES_EQUAL(rainMonthly = 500, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-19 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 25);
  DOUBLES_EQUAL(rainMonthly = 520, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-20 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 20);
  DOUBLES_EQUAL(rainMonthly = 615, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-21 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 80);
  DOUBLES_EQUAL(rainMonthly = 675, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-22 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 30);
  DOUBLES_EQUAL(rainMonthly = 725, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-23 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 40);
  DOUBLES_EQUAL(rainMonthly = 735, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-24 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 60);
  DOUBLES_EQUAL(rainMonthly = 755, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-25 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 90);
  DOUBLES_EQUAL(rainMonthly = 785, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-26 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 30);
  DOUBLES_EQUAL(rainMonthly = 825, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-27 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 80);
  DOUBLES_EQUAL(rainMonthly = 875, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-28 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 40);
  DOUBLES_EQUAL(rainMonthly = 935, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-29 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 10);
  DOUBLES_EQUAL(rainMonthly = 1005, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-30 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 15);
  DOUBLES_EQUAL(rainMonthly = 1010, rainGauge.currentMonth(), TOLERANCE);

  // New month
  setTime("2022-10-01 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor = 20);
  DOUBLES_EQUAL(rainMonthly = 0, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-10-02 12:00:00", tm, ts);
  rainGauge.update(ts, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
}


/*
 * Test that rain gauge values are preserved after sensor startup,
 * i.e. sensor reset or battery change
 */
TEST(TestRainGaugeStartup, TestRainStartup) {
  RainGauge rainGauge(100);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;
    
  printf("< RainStartup >\n");
   
  setTime("2023-07-16 8:00:00", tm, ts);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);
  DOUBLES_EQUAL(-1, rainGauge.currentDay(), TOLERANCE);
  DOUBLES_EQUAL(-1, rainGauge.currentWeek(), TOLERANCE);

  setTime("2023-07-16 8:05:00", tm, ts);
  rainGauge.update(ts, rainSensor = 10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);

  setTime("2023-07-16 8:10:00", tm, ts);
  rainGauge.update(ts, rainSensor = 15.0);
  DOUBLES_EQUAL(5, rainGauge.pastHour(), TOLERANCE);
  DOUBLES_EQUAL(5, rainGauge.currentDay(), TOLERANCE);
  DOUBLES_EQUAL(5, rainGauge.currentWeek(), TOLERANCE);

  setTime("2023-07-16 8:15:00", tm, ts);
  rainGauge.update(ts, rainSensor = 0, true);
  DOUBLES_EQUAL(5, rainGauge.pastHour(), TOLERANCE);
  DOUBLES_EQUAL(5, rainGauge.currentDay(), TOLERANCE);
  DOUBLES_EQUAL(5, rainGauge.currentWeek(), TOLERANCE);
}

/*
 * Test that methods indicate an invalid request if
 * called before initial invocation of update()
 */
TEST(TestRainGaugeInvReq, TestRainInvReq) {
  RainGauge rainGauge(100);
    
  printf("< RainInvReq >\n");

  DOUBLES_EQUAL(-1, rainGauge.currentDay(), TOLERANCE);
  DOUBLES_EQUAL(-1, rainGauge.currentWeek(), TOLERANCE);
  DOUBLES_EQUAL(-1, rainGauge.currentMonth(), TOLERANCE);
}

TEST_GROUP(TestRainGauge24Hours) {
  void setup() {
  }

  void teardown() {
  }
};

/*
 * Test rainfall during past 24 hours
 */
TEST(TestRainGauge24Hours, Test_Rain24Hours) {
  RainGauge rainGauge(1000);
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;
  bool      val;
  int       nbins;
  float     qual;

  printf("< Rain24Hours >\n");
  
  // Start at 8:00 AM
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.past24Hours(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(1, nbins);
  DOUBLES_EQUAL(1.0/24.0, qual, TOLERANCE_QUAL);

  // Update every hour for 5 hours
  setTime("2022-09-06 9:00", tm, ts);
  rainGauge.update(ts, rainSensor=11.0);
  DOUBLES_EQUAL(1.0, rainGauge.past24Hours(&val, &nbins, &qual), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(2, nbins);
  DOUBLES_EQUAL(2.0/24.0, qual, TOLERANCE_QUAL);

  setTime("2022-09-06 10:00", tm, ts);
  rainGauge.update(ts, rainSensor=12.5);
  DOUBLES_EQUAL(2.5, rainGauge.past24Hours(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(3, nbins);

  setTime("2022-09-06 11:00", tm, ts);
  rainGauge.update(ts, rainSensor=14.0);
  DOUBLES_EQUAL(4.0, rainGauge.past24Hours(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(4, nbins);
  
  setTime("2022-09-06 12:00", tm, ts);
  rainGauge.update(ts, rainSensor=16.0);
  DOUBLES_EQUAL(6.0, rainGauge.past24Hours(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(5, nbins);

  // Continue over multiple hours to build up history
  float currentRain = 16.0;
  for (int hour = 13; hour <= 20; hour++) {
    char timeStr[20];
    sprintf(timeStr, "2022-09-06 %d:00", hour);
    setTime(timeStr, tm, ts);
    currentRain += 1.0;
    rainGauge.update(ts, rainSensor=currentRain);
  }
  
  // After 20:00, we should have 13 hours of data
  // Rain from hour 8-20: (11-10) + (12.5-11) + (14-12.5) + (16-14) + (17-16) + ... + (24-23) = 14.0
  DOUBLES_EQUAL(14.0, rainGauge.past24Hours(&val, &nbins), TOLERANCE);
  CHECK_FALSE(val);
  CHECK_EQUAL(13, nbins);

  // Add more hours to reach the threshold
  for (int hour = 21; hour <= 23; hour++) {
    char timeStr[20];
    sprintf(timeStr, "2022-09-06 %d:00", hour);
    setTime(timeStr, tm, ts);
    currentRain += 1.0;
    rainGauge.update(ts, rainSensor=currentRain);
  }

  // Now continue into the next day
  for (int hour = 0; hour <= 6; hour++) {
    char timeStr[20];
    sprintf(timeStr, "2022-09-07 %02d:00", hour);
    setTime(timeStr, tm, ts);
    currentRain += 1.0;
    rainGauge.update(ts, rainSensor=currentRain);
  }
  
  // After 6:00 on day 2, we should have 23 hours of data
  // Total rain from hour 8 (day 1) to hour 6 (day 2) = 34.0 - 10.0 = 24.0
  // Quality = 23/24 = 0.958 > 0.8, so it should be valid!
  DOUBLES_EQUAL(24.0, rainGauge.past24Hours(&val, &nbins), TOLERANCE);
  CHECK(val);  // Valid because 23/24 > 0.8
  CHECK_EQUAL(23, nbins);
  
  // Add one more hour to reach 24 hours
  setTime("2022-09-07 7:00", tm, ts);
  rainGauge.update(ts, rainSensor=currentRain+=1.0);
  // Total rain from hour 8 (day 1) to hour 7 (day 2) = 35.0 - 10.0 = 25.0
  DOUBLES_EQUAL(25.0, rainGauge.past24Hours(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(24, nbins);
  
  // Move forward one more hour - should overwrite hour 8 (day 1)
  setTime("2022-09-07 8:00", tm, ts);
  rainGauge.update(ts, rainSensor=currentRain+=1.0);
  // The past 24 hours at hour 8 (day 2) should be from hour 8 (day 1) to hour 8 (day 2)
  // Rain = sensor[8, day 2] - sensor[8, day 1] = 36.0 - 10.0 = 26.0
  DOUBLES_EQUAL(26.0, rainGauge.past24Hours(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(24, nbins);
  
  // Move forward another hour
  setTime("2022-09-07 9:00", tm, ts);
  rainGauge.update(ts, rainSensor=currentRain+=1.0);
  // The past 24 hours at hour 9 (day 2) should be from hour 9 (day 1) to hour 9 (day 2)
  // Rain = sensor[9, day 2] - sensor[9, day 1] = 37.0 - 11.0 = 26.0
  DOUBLES_EQUAL(26.0, rainGauge.past24Hours(&val, &nbins), TOLERANCE);
  CHECK(val);
  CHECK_EQUAL(24, nbins);
}

TEST_GROUP(TestRainGaugeConstructor) {
  void setup() {
  }

  void teardown() {
  }
};

/*
 * Test constructor with custom raingauge_max value
 */
TEST(TestRainGaugeConstructor, Test_Constructor_CustomMax) {
  // Test with default value
  RainGauge rainGauge1;
  
  // Test with custom max value
  RainGauge rainGauge2(500);
  RainGauge rainGauge3(2000);
  
  printf("< Constructor_CustomMax >\n");
  
  // Set up test scenario - all should work with their respective max values
  tm        tm;
  time_t    ts;
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge1.update(ts, 10.0);
  rainGauge2.update(ts, 10.0);
  rainGauge3.update(ts, 10.0);
  
  setTime("2022-09-06 8:06", tm, ts);
  rainGauge1.update(ts, 15.0);
  rainGauge2.update(ts, 15.0);
  rainGauge3.update(ts, 15.0);
  
  // All should report same rain
  DOUBLES_EQUAL(5.0, rainGauge1.pastHour(), TOLERANCE);
  DOUBLES_EQUAL(5.0, rainGauge2.pastHour(), TOLERANCE);
  DOUBLES_EQUAL(5.0, rainGauge3.pastHour(), TOLERANCE);
}

/*
 * Test constructor with custom quality_threshold
 */
TEST(TestRainGaugeConstructor, Test_Constructor_QualityThreshold) {
  // Low threshold (10%) - easier to get valid results
  RainGauge rainGauge1(100, 0.1);
  
  // High threshold (95%) - harder to get valid results
  RainGauge rainGauge2(100, 0.95);
  
  printf("< Constructor_QualityThreshold >\n");
  
  tm        tm;
  time_t    ts;
  bool      val1, val2;
  
  // Set up scenario with minimal data
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge1.update(ts, 10.0);
  rainGauge2.update(ts, 10.0);
  
  setTime("2022-09-06 8:06", tm, ts);
  rainGauge1.update(ts, 11.0);
  rainGauge2.update(ts, 11.0);
  
  rainGauge1.pastHour(&val1);
  rainGauge2.pastHour(&val2);
  
  // 10% threshold: should be valid with minimal data
  CHECK(val1);
  
  // 95% threshold: should be invalid with minimal data
  CHECK_FALSE(val2);
}

TEST_GROUP(TestRainGaugeSetMax) {
  void setup() {
  }

  void teardown() {
  }
};

/*
 * Test set_max() function
 */
TEST(TestRainGaugeSetMax, Test_SetMax) {
  RainGauge rainGauge(100);
  
  printf("< SetMax >\n");
  
  tm        tm;
  time_t    ts;
  
  // Initial update
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, 95.0);
  
  // Update near old max (100)
  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(ts, 99.0);
  DOUBLES_EQUAL(4.0, rainGauge.pastHour(), TOLERANCE);
  
  // Now change max to 200
  rainGauge.set_max(200);
  
  // Continue with values above old max but below new max
  setTime("2022-09-06 8:12", tm, ts);
  rainGauge.update(ts, 105.0);
  DOUBLES_EQUAL(10.0, rainGauge.pastHour(), TOLERANCE);
  
  // Test overflow with new max (200)
  setTime("2022-09-06 8:18", tm, ts);
  rainGauge.update(ts, 5.0);  // Overflow from 105 to 205 (wraps at 200) -> 5
  // Expected rain: 5.0 + 200 - 105 = 100.0
  DOUBLES_EQUAL(110.0, rainGauge.pastHour(), TOLERANCE);
}

TEST_GROUP(TestRainGaugeReset) {
  void setup() {
  }

  void teardown() {
  }
};

/*
 * Test reset() with individual flags
 */
TEST(TestRainGaugeReset, Test_Reset_IndividualFlags) {
  printf("< Reset_IndividualFlags >\n");
  
  tm        tm;
  time_t    ts;
  
  // Test RESET_RAIN_H (hourly)
  {
    RainGauge rainGauge(100);
    setTime("2022-09-06 8:00", tm, ts);
    rainGauge.update(ts, 10.0);
    setTime("2022-09-06 8:06", tm, ts);
    rainGauge.update(ts, 15.0);
    DOUBLES_EQUAL(5.0, rainGauge.pastHour(), TOLERANCE);
    
    rainGauge.reset(RESET_RAIN_H);
    DOUBLES_EQUAL(0.0, rainGauge.pastHour(), TOLERANCE);
  }
  
  // Test RESET_RAIN_D (daily)
  {
    RainGauge rainGauge(100);
    setTime("2022-09-06 8:00", tm, ts);
    rainGauge.update(ts, 10.0);
    setTime("2022-09-06 10:00", tm, ts);  // Stay in same day
    rainGauge.update(ts, 20.0);
    float beforeReset = rainGauge.currentDay();
    CHECK(beforeReset > 0);  // Should have valid data in same day
    
    rainGauge.reset(RESET_RAIN_D);
    // After reset, the next day boundary should start fresh
    setTime("2022-09-07 8:00", tm, ts);
    rainGauge.update(ts, 25.0);
    setTime("2022-09-07 10:00", tm, ts);
    rainGauge.update(ts, 26.0);
    float afterReset = rainGauge.currentDay();
    // Should have new data (not accumulated from before reset)
    CHECK(afterReset >= 0);
    CHECK(afterReset <= beforeReset);  // Should be less than or equal
  }
  
  // Test RESET_RAIN_W (weekly)
  {
    RainGauge rainGauge(100);
    setTime("2022-09-06 8:00", tm, ts);  // Tuesday
    rainGauge.update(ts, 10.0);
    setTime("2022-09-06 10:00", tm, ts);  // Same day
    rainGauge.update(ts, 20.0);
    float weekBefore = rainGauge.currentWeek();
    CHECK(weekBefore >= 0);  // Should have valid data
    
    rainGauge.reset(RESET_RAIN_W);
    // Move to next week to see reset effect
    setTime("2022-09-13 8:00", tm, ts);  // Next Tuesday
    rainGauge.update(ts, 25.0);
    setTime("2022-09-13 9:00", tm, ts);
    rainGauge.update(ts, 26.0);
    float weekAfter = rainGauge.currentWeek();
    CHECK(weekAfter >= 0);  // Should have data from new week
    CHECK(weekAfter <= weekBefore);  // Should be less than or equal
  }
  
  // Test RESET_RAIN_M (monthly)
  {
    RainGauge rainGauge(100);
    setTime("2022-09-06 8:00", tm, ts);
    rainGauge.update(ts, 10.0);
    setTime("2022-09-06 10:00", tm, ts);  // Same day/month
    rainGauge.update(ts, 20.0);
    float monthBefore = rainGauge.currentMonth();
    CHECK(monthBefore >= 0);  // Should have valid data
    
    rainGauge.reset(RESET_RAIN_M);
    // Move to next month to see reset effect
    setTime("2022-10-06 8:00", tm, ts);  // Next month
    rainGauge.update(ts, 25.0);
    setTime("2022-10-06 9:00", tm, ts);
    rainGauge.update(ts, 26.0);
    float monthAfter = rainGauge.currentMonth();
    CHECK(monthAfter >= 0);  // Should have data from new month
    CHECK(monthAfter <= monthBefore);  // Should be less than or equal
  }
}

/*
 * Test reset() with RESET_RAIN_24H flag
 */
TEST(TestRainGaugeReset, Test_Reset_24H) {
  RainGauge rainGauge(100);
  
  printf("< Reset_24H >\n");
  
  tm        tm;
  time_t    ts;
  
  // Build up 24h history
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, 10.0);
  
  setTime("2022-09-06 9:00", tm, ts);
  rainGauge.update(ts, 15.0);
  
  setTime("2022-09-06 10:00", tm, ts);
  rainGauge.update(ts, 20.0);
  
  DOUBLES_EQUAL(10.0, rainGauge.past24Hours(), TOLERANCE);
  
  // Reset 24h history
  rainGauge.reset(RESET_RAIN_24H);
  DOUBLES_EQUAL(0.0, rainGauge.past24Hours(), TOLERANCE);
  
  // Verify other data is not affected
  // (pastHour might have data depending on implementation)
}

/*
 * Test reset() with combined flags
 */
TEST(TestRainGaugeReset, Test_Reset_Combined) {
  RainGauge rainGauge(100);
  
  printf("< Reset_Combined >\n");
  
  tm        tm;
  time_t    ts;
  
  // Build up data
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, 10.0);
  
  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(ts, 15.0);
  
  setTime("2022-09-06 9:00", tm, ts);
  rainGauge.update(ts, 20.0);
  
  setTime("2022-09-07 8:00", tm, ts);
  rainGauge.update(ts, 25.0);
  
  // Verify data exists
  CHECK(rainGauge.past24Hours() > 0);
  
  // Reset multiple counters
  rainGauge.reset(RESET_RAIN_H | RESET_RAIN_D | RESET_RAIN_24H);
  
  // These should be reset
  DOUBLES_EQUAL(0.0, rainGauge.pastHour(), TOLERANCE);
  DOUBLES_EQUAL(0.0, rainGauge.past24Hours(), TOLERANCE);
}

/*
 * Test full reset (all flags)
 */
TEST(TestRainGaugeReset, Test_Reset_Full) {
  RainGauge rainGauge(100);
  
  printf("< Reset_Full >\n");
  
  tm        tm;
  time_t    ts;
  
  // Build up complete data
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, 10.0);
  
  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(ts, 15.0);
  
  setTime("2022-09-06 9:00", tm, ts);
  rainGauge.update(ts, 20.0);
  
  setTime("2022-09-07 8:00", tm, ts);
  rainGauge.update(ts, 25.0);
  
  // Full reset using default parameter (resets H, D, W, M but not 24H by default)
  rainGauge.reset();
  
  // After full reset, hourly history counter should be clear
  DOUBLES_EQUAL(0.0, rainGauge.pastHour(), TOLERANCE);
  // Note: past24Hours may still have data if RESET_RAIN_24H not included in default
  
  // After reset, next update should start fresh
  setTime("2022-09-08 9:00", tm, ts);
  rainGauge.update(ts, 30.0);
  
  setTime("2022-09-08 9:06", tm, ts);
  rainGauge.update(ts, 32.0);
  
  DOUBLES_EQUAL(2.0, rainGauge.pastHour(), TOLERANCE);
}

TEST_GROUP(TestRainGaugeEdgeCases) {
  void setup() {
  }

  void teardown() {
  }
};

/*
 * Test behavior with very small raingaugeMax
 */
TEST(TestRainGaugeEdgeCases, Test_SmallMaxValue) {
  RainGauge rainGauge(10);  // Very small max
  
  printf("< SmallMaxValue >\n");
  
  tm        tm;
  time_t    ts;
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, 5.0);
  
  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(ts, 9.0);
  DOUBLES_EQUAL(4.0, rainGauge.pastHour(), TOLERANCE);
  
  // Overflow
  setTime("2022-09-06 8:12", tm, ts);
  rainGauge.update(ts, 2.0);  // 9 -> 12 (wraps) -> 2
  DOUBLES_EQUAL(7.0, rainGauge.pastHour(), TOLERANCE);
}

/*
 * Test accumulator near boundary
 */
TEST(TestRainGaugeEdgeCases, Test_AccumulatorBoundary) {
  RainGauge rainGauge(100);
  
  printf("< AccumulatorBoundary >\n");
  
  tm        tm;
  time_t    ts;
  
  // Start near boundary
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, 95.0);
  
  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(ts, 98.0);
  DOUBLES_EQUAL(3.0, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 8:12", tm, ts);
  rainGauge.update(ts, 99.9);
  DOUBLES_EQUAL(4.9, rainGauge.pastHour(), TOLERANCE);
  
  // Cross boundary
  setTime("2022-09-06 8:18", tm, ts);
  rainGauge.update(ts, 0.5);  // Overflow: 99.9 -> 100.5 (wraps at 100) -> 0.5
  DOUBLES_EQUAL(5.5, rainGauge.pastHour(), TOLERANCE);
}

/*
 * Test zero rainfall over extended period
 */
TEST(TestRainGaugeEdgeCases, Test_NoRainExtended) {
  RainGauge rainGauge(100);
  
  printf("< NoRainExtended >\n");
  
  tm        tm;
  time_t    ts;
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, 10.0);
  
  // Multiple updates with no rain
  for (int i = 1; i <= 10; i++) {
    char timeStr[20];
    sprintf(timeStr, "2022-09-06 8:%02d", i * 6);
    setTime(timeStr, tm, ts);
    rainGauge.update(ts, 10.0);  // Same value = no rain
  }
  
  DOUBLES_EQUAL(0.0, rainGauge.pastHour(), TOLERANCE);
  DOUBLES_EQUAL(0.0, rainGauge.currentDay(), TOLERANCE);
}

/*
 * Test continuous light rain
 */
TEST(TestRainGaugeEdgeCases, Test_LightContinuousRain) {
  RainGauge rainGauge(100);
  
  printf("< LightContinuousRain >\n");
  
  tm        tm;
  time_t    ts;
  float     rain = 10.0;
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(ts, rain);
  
  // Very light rain (0.1mm every 6 minutes)
  for (int i = 1; i <= 9; i++) {
    char timeStr[20];
    sprintf(timeStr, "2022-09-06 8:%02d", i * 6);
    setTime(timeStr, tm, ts);
    rain += 0.1;
    rainGauge.update(ts, rain);
  }
  
  DOUBLES_EQUAL(0.9, rainGauge.pastHour(), TOLERANCE);
}
