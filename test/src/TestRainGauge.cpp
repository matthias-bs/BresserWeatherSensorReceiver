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
