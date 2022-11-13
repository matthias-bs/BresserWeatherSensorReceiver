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
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CppUTest/TestHarness.h"

#define TOLERANCE 0.1
#include "RainGauge.h"

//#define _DEBUG_CIRCULAR_BUFFER_

#if defined(_DEBUG_CIRCULAR_BUFFER_)
    #define DEBUG_CB() { rainGauge.printCircularBuffer(); }

#else
  #define DEBUG_CB() {}
#endif



static RainGauge rainGauge;

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

TEST_GROUP(TestRainGaugeHour) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourShortInterval) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourLongInterval) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourExtremeInterval) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeDaily) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeWeekly) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeMonthly) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourOv) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeHourOvMidnight) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeDailyOv) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeWeeklyOv) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};

TEST_GROUP(TestRainGaugeMonthlyOv) {
  void setup() {
      rainGauge.reset();
  }

  void teardown() {
  }
};


/*
 * Test rainfall during past hour (no rain gauge overflow)
 */
TEST(TestRainGaugeHour, Test_RainHour) {
  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainHour >\n");
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(tm, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(tm, rainSensor=10.1);
  DEBUG_CB();
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 8:12", tm, ts);
  rainGauge.update(tm, rainSensor=10.3);
  DEBUG_CB();
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:18", tm, ts);
  rainGauge.update(tm, rainSensor=10.6);
  DEBUG_CB();
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:24", tm, ts);
  rainGauge.update(tm, rainSensor=11.0);
  DEBUG_CB();
  DOUBLES_EQUAL(1.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:30", tm, ts);
  rainGauge.update(tm, rainSensor=11.5);
  DEBUG_CB();
  DOUBLES_EQUAL(1.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 8:36", tm, ts);
  rainGauge.update(tm, rainSensor=12.1);
  DEBUG_CB();
  DOUBLES_EQUAL(2.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 8:42", tm, ts);
  rainGauge.update(tm, rainSensor=12.8);
  DEBUG_CB();
  DOUBLES_EQUAL(2.8, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 8:48", tm, ts);
  rainGauge.update(tm, rainSensor=13.6);
  DEBUG_CB();
  DOUBLES_EQUAL(3.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:54", tm, ts);
  rainGauge.update(tm, rainSensor=14.5);
  DEBUG_CB();
  DOUBLES_EQUAL(4.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:00", tm, ts);
  rainGauge.update(tm, rainSensor=15.5);
  DEBUG_CB();
  DOUBLES_EQUAL(5.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:06", tm, ts);
  rainGauge.update(tm, rainSensor=16.6);
  DEBUG_CB();
  DOUBLES_EQUAL(16.6 - 10.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:12", tm, ts);
  rainGauge.update(tm, rainSensor=17.8);
  DEBUG_CB();
  DOUBLES_EQUAL(17.8 - 10.3, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test rainfall during past hour (no rain gauge overflow),
 * short update interval (5 minutes)
 */
TEST(TestRainGaugeHourShortInterval, Test_RainHourShort) {
  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainHourShort >\n");
  
  setTime("2022-09-11 15:00", tm, ts);
  rainGauge.update(tm, rainSensor=10.0);
  DEBUG_CB();
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:05", tm, ts);
  rainGauge.update(tm, rainSensor=10.1);
  DEBUG_CB();
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:10", tm, ts);
  rainGauge.update(tm, rainSensor=10.3);
  DEBUG_CB();
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:15", tm, ts);
  rainGauge.update(tm, rainSensor=10.6);
  DEBUG_CB();
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:20", tm, ts);
  rainGauge.update(tm, rainSensor=11.0);
  DEBUG_CB();
  DOUBLES_EQUAL(1.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:25", tm, ts);
  rainGauge.update(tm, rainSensor=11.5);
  DEBUG_CB();
  DOUBLES_EQUAL(1.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 15:30", tm, ts);
  rainGauge.update(tm, rainSensor=12.1);
  DEBUG_CB();
  DOUBLES_EQUAL(2.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 15:35", tm, ts);
  rainGauge.update(tm, rainSensor=12.8);
  DEBUG_CB();
  DOUBLES_EQUAL(2.8, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:40", tm, ts);
  rainGauge.update(tm, rainSensor=13.6);
  DEBUG_CB();
  DOUBLES_EQUAL(3.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:45", tm, ts);
  rainGauge.update(tm, rainSensor=14.5);
  DEBUG_CB();
  DOUBLES_EQUAL(4.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:50", tm, ts);
  rainGauge.update(tm, rainSensor=15.5);
  DEBUG_CB();
  DOUBLES_EQUAL(5.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:55", tm, ts);
  rainGauge.update(tm, rainSensor=16.6);
  DEBUG_CB();
  DOUBLES_EQUAL(6.6, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 16:00", tm, ts);
  rainGauge.update(tm, rainSensor=17.8);
  DEBUG_CB();
  DOUBLES_EQUAL(7.8, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 16:05", tm, ts);
  rainGauge.update(tm, rainSensor=18.8);
  DEBUG_CB();
  DOUBLES_EQUAL(18.8 - 10.1, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 16:10", tm, ts);
  rainGauge.update(tm, rainSensor=19.9);
  DEBUG_CB();
  DOUBLES_EQUAL(19.9 - 10.3, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test rainfall during past hour (no rain gauge overflow),
 * long update interval (10 minutes)
 * The ring buffer will not be filled completely.
 */
TEST(TestRainGaugeHourLongInterval, Test_RainHourLong) {
  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainHourLong >\n");
  
  setTime("2022-09-11 15:00", tm, ts);
  rainGauge.update(tm, rainSensor=10.0);
  DEBUG_CB();
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:10", tm, ts);
  rainGauge.update(tm, rainSensor=10.1);
  DEBUG_CB();
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 15:20", tm, ts);
  rainGauge.update(tm, rainSensor=10.3);
  DEBUG_CB();
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:30", tm, ts);
  rainGauge.update(tm, rainSensor=10.6);
  DEBUG_CB();
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:40", tm, ts);
  rainGauge.update(tm, rainSensor=11.0);
  DEBUG_CB();
  DOUBLES_EQUAL(1.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 15:50", tm, ts);
  rainGauge.update(tm, rainSensor=11.5);
  DEBUG_CB();
  DOUBLES_EQUAL(1.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 16:00", tm, ts);
  rainGauge.update(tm, rainSensor=12.1);
  DEBUG_CB();
  DOUBLES_EQUAL(2.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 16:10", tm, ts);
  rainGauge.update(tm, rainSensor=12.8);
  DEBUG_CB();
  DOUBLES_EQUAL(12.8 - 10.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 16:20", tm, ts);
  rainGauge.update(tm, rainSensor=13.6);
  DEBUG_CB();
  DOUBLES_EQUAL(13.6 - 10.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 16:30", tm, ts);
  rainGauge.update(tm, rainSensor=14.5);
  DEBUG_CB();
  DOUBLES_EQUAL(14.5 - 10.6, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 16:40", tm, ts);
  rainGauge.update(tm, rainSensor=15.5);
  DEBUG_CB();
  DOUBLES_EQUAL(15.5 - 11.0, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 16:50", tm, ts);
  rainGauge.update(tm, rainSensor=16.6);
  DEBUG_CB();
  DOUBLES_EQUAL(16.6 - 11.5, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test rainfall during past hour (no rain gauge overflow),
 * extremely long update interval (65 minutes)
 * The distance between head and tail will be > 1h.
 */
TEST(TestRainGaugeHourExtremeInterval, Test_RainHourExtreme) {
  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainHourExtreme >\n");
  
  setTime("2022-09-11 15:00", tm, ts);
  rainGauge.update(tm, rainSensor=10.0);
  DEBUG_CB();
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 16:05", tm, ts);
  rainGauge.update(tm, rainSensor=10.1);
  DEBUG_CB();
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-11 17:10", tm, ts);
  rainGauge.update(tm, rainSensor=10.3);
  DEBUG_CB();
  DOUBLES_EQUAL(0.2, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 18:15", tm, ts);
  rainGauge.update(tm, rainSensor=10.6);
  DEBUG_CB();
  DOUBLES_EQUAL(0.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 19:20", tm, ts);
  rainGauge.update(tm, rainSensor=11.0);
  DEBUG_CB();
  DOUBLES_EQUAL(0.4, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-11 20:25", tm, ts);
  rainGauge.update(tm, rainSensor=11.5);
  DEBUG_CB();
  DOUBLES_EQUAL(0.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-11 21:40", tm, ts);
  rainGauge.update(tm, rainSensor=12.1);
  DEBUG_CB();
  DOUBLES_EQUAL(0.6, rainGauge.pastHour(), TOLERANCE);  
}


/*
 * Test daily rainfall (no rain gauge overflow)
 */
TEST(TestRainGaugeDaily, Test_RainDaily) {
  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainDaily >\n");
  setTime("2022-09-06 8:00", tm, ts);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);

  rainGauge.update(tm, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 12:00", tm, ts);
  rainGauge.update(tm, rainSensor=12.0);
  DOUBLES_EQUAL(2, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 16:00", tm, ts);
  rainGauge.update(tm, rainSensor=14.0);
  DOUBLES_EQUAL(4, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 20:00", tm, ts);
  rainGauge.update(tm, rainSensor=16.0);
  DOUBLES_EQUAL(6, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 23:59", tm, ts);
  rainGauge.update(tm, rainSensor=18.0);
  DOUBLES_EQUAL(8, rainGauge.currentDay(), TOLERANCE);

  // Next Day
  setTime("2022-09-07 00:00", tm, ts);
  rainGauge.update(tm, rainSensor=20.0);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);  

  setTime("2022-09-07 04:00", tm, ts);
  rainGauge.update(tm, rainSensor=22.0);
  DOUBLES_EQUAL(2, rainGauge.currentDay(), TOLERANCE);  
}


/*
 * Test weekly rainfall (no rain gauge overflow)
 */
TEST(TestRainGaugeWeekly, Test_RainWeekly) {
  tm        tm;
  time_t    ts;
  float     rainSensor;

  printf("< RainWeekly >\n");
  
  setTime("2022-09-06 8:00:00", tm, ts);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);

  rainGauge.update(tm, rainSensor=10.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);

  setTime("2022-09-06 16:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=15.0);
  DOUBLES_EQUAL(5, rainGauge.currentWeek(), TOLERANCE);
  
  setTime("2022-09-06 23:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=20.0);
  DOUBLES_EQUAL(10, rainGauge.currentWeek(), TOLERANCE);
  
  setTime("2022-09-07 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=25.0);
  DOUBLES_EQUAL(15, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-08 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=30.0);
  DOUBLES_EQUAL(20, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-09 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=35.0);
  DOUBLES_EQUAL(25, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-10 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=40.0);
  DOUBLES_EQUAL(30, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-11 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=45.0);
  DOUBLES_EQUAL(35, rainGauge.currentWeek(), TOLERANCE);  

  // Next Week
  setTime("2022-09-12 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=50.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-13 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=50.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);  
}


/*
 * Test monthly rainfall (no rain gauge overflow)
 */
TEST(TestRainGaugeMonthly, Test_RainMonthly) {
  tm        tm;
  time_t    ts;
  float     rainSensor = 0;
  float     rainMonthly;
    
  printf("< RainMonthly >\n");
  
  setTime("2022-09-06 12:00:00", tm, ts);
  DOUBLES_EQUAL(rainMonthly=0, rainGauge.currentMonth(), TOLERANCE);

  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly = 0, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-07 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-08 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-09 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-10 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-11 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-12 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-13 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-14 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-15 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-16 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-17 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-18 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-19 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-20 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-21 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-22 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-23 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-24 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-25 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-26 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-27 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-28 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-29 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-30 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);

  // New month
  setTime("2022-10-01 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly = 0, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-10-02 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
}


/*
 * Test rainfall during past hour (with rain gauge overflow)
 */
TEST(TestRainGaugeHourOv, Test_RainHourOv) {
  tm        tm;
  time_t    ts;

  printf("< RainHourOv >\n");
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(tm, 10.0);
  DEBUG_CB();
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:06", tm, ts);
  rainGauge.update(tm, 10.1);
  DEBUG_CB();
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 8:12", tm, ts);
  rainGauge.update(tm, 60.3);
  DEBUG_CB();
  DOUBLES_EQUAL(50.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:18", tm, ts);
  rainGauge.update(tm, 0.6);
  DEBUG_CB();
  DOUBLES_EQUAL(90.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:24", tm, ts);
  rainGauge.update(tm, 10.0);
  DEBUG_CB();
  DOUBLES_EQUAL(100.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:30", tm, ts);
  rainGauge.update(tm, 11.5);
  DEBUG_CB();
  DOUBLES_EQUAL(101.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 8:36", tm, ts);
  rainGauge.update(tm, 12.1);
  DEBUG_CB();
  DOUBLES_EQUAL(102.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 8:42", tm, ts);
  rainGauge.update(tm, 92.8);
  DEBUG_CB();
  DOUBLES_EQUAL(182.8, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 8:48", tm, ts);
  rainGauge.update(tm, 23.6);
  DEBUG_CB();
  DOUBLES_EQUAL(213.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 8:54", tm, ts);
  rainGauge.update(tm, 14.5);
  DEBUG_CB();
  DOUBLES_EQUAL(304.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:00", tm, ts);
  rainGauge.update(tm, 15.5);
  DEBUG_CB();
  DOUBLES_EQUAL(305.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:06", tm, ts);
  rainGauge.update(tm, 5.5);
  DEBUG_CB();
  DOUBLES_EQUAL(405.5 - 10.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 9:12", tm, ts);
  rainGauge.update(tm, 17.8);
  DEBUG_CB();
  DOUBLES_EQUAL(417.8 - 60.3, rainGauge.pastHour(), TOLERANCE);
}


/*
 * Test rainfall during past hour (with rain gauge overflow),
 * timestamps across Midnight
 */
TEST(TestRainGaugeHourOvMidnight, Test_RainHourOvMidnight) {
  tm        tm;
  time_t    ts;

  printf("< RainHourOvMidnight >\n");
  
  setTime("2022-09-06 23:00", tm, ts);
  rainGauge.update(tm, 10.0);
  DEBUG_CB();
  DOUBLES_EQUAL(0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:06", tm, ts);
  rainGauge.update(tm, 10.1);
  DEBUG_CB();
  DOUBLES_EQUAL(0.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 23:12", tm, ts);
  rainGauge.update(tm, 60.3);
  DEBUG_CB();
  DOUBLES_EQUAL(50.3, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:18", tm, ts);
  rainGauge.update(tm, 0.6);
  DEBUG_CB();
  DOUBLES_EQUAL(90.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:24", tm, ts);
  rainGauge.update(tm, 10.0);
  DEBUG_CB();
  DOUBLES_EQUAL(100.0, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:30", tm, ts);
  rainGauge.update(tm, 11.5);
  DEBUG_CB();
  DOUBLES_EQUAL(101.5, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 23:36", tm, ts);
  rainGauge.update(tm, 12.1);
  DEBUG_CB();
  DOUBLES_EQUAL(102.1, rainGauge.pastHour(), TOLERANCE);  

  setTime("2022-09-06 23:42", tm, ts);
  rainGauge.update(tm, 92.8);
  DEBUG_CB();
  DOUBLES_EQUAL(182.8, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-06 23:48", tm, ts);
  rainGauge.update(tm, 23.6);
  DEBUG_CB();
  DOUBLES_EQUAL(213.6, rainGauge.pastHour(), TOLERANCE);

  setTime("2022-09-06 23:54", tm, ts);
  rainGauge.update(tm, 14.5);
  DEBUG_CB();
  DOUBLES_EQUAL(304.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-07 00:00", tm, ts);
  rainGauge.update(tm, 15.5);
  DEBUG_CB();
  DOUBLES_EQUAL(305.5, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-07 00:06", tm, ts);
  rainGauge.update(tm, 5.5);
  DEBUG_CB();
  DOUBLES_EQUAL(405.5 - 10.1, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-07 00:12", tm, ts);
  rainGauge.update(tm, 17.8);
  DEBUG_CB();
  DOUBLES_EQUAL(417.8 - 60.3, rainGauge.pastHour(), TOLERANCE);
  
  setTime("2022-09-07 00:18", tm, ts);
  rainGauge.update(tm, 17.8);
  DEBUG_CB();
  DOUBLES_EQUAL(417.8 - 100.6, rainGauge.pastHour(), TOLERANCE);

}


/*
 * Test daily rainfall (with rain gauge overflow)
 */
TEST(TestRainGaugeDailyOv, Test_RainDailyOv) {
  tm        tm;
  time_t    ts;
  float     rainSensor;
  float     rainDaily = 0.0;

  printf("< RainDailyOv >\n");

  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(tm, rainSensor = 0.0);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);

  rainGauge.update(tm, rainSensor += 10.0);
  DOUBLES_EQUAL(rainDaily += 10, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 12:00", tm, ts);
  rainGauge.update(tm, rainSensor += 42.0);
  DOUBLES_EQUAL(rainDaily += 42, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 16:00", tm, ts);
  rainGauge.update(tm, rainSensor =  2.0);
  DOUBLES_EQUAL(rainDaily += 50, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 20:00", tm, ts);
  rainGauge.update(tm, rainSensor += 53.0);
  DOUBLES_EQUAL(rainDaily += 53, rainGauge.currentDay(), TOLERANCE);

  setTime("2022-09-06 23:59", tm, ts);
  rainGauge.update(tm, rainSensor = 5.0);
  DOUBLES_EQUAL(rainDaily += 50, rainGauge.currentDay(), TOLERANCE);

  // Next Day
  setTime("2022-09-07 00:00", tm, ts);
  rainGauge.update(tm, rainSensor=42.0);
  DOUBLES_EQUAL(0, rainGauge.currentDay(), TOLERANCE);  

  setTime("2022-09-07 04:00", tm, ts);
  rainGauge.update(tm, rainSensor= 2.0);
  DOUBLES_EQUAL(60, rainGauge.currentDay(), TOLERANCE);  
}

/*
 * Test weekly rainfall (with rain gauge overflow)
 */
TEST(TestRainGaugeWeeklyOv, Test_RainWeeklyOv) {
  tm        tm;
  time_t    ts;
  float     rainSensor;
    
  printf("< RainWeeklyOv >\n");
   
  setTime("2022-09-06 8:00:00", tm, ts);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);

  rainGauge.update(tm, rainSensor = 10.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);

  setTime("2022-09-06 16:00:00", tm, ts);
  rainGauge.update(tm, rainSensor =  0);
  DOUBLES_EQUAL(90, rainGauge.currentWeek(), TOLERANCE);
  
  setTime("2022-09-06 23:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 60.0);
  DOUBLES_EQUAL(150, rainGauge.currentWeek(), TOLERANCE);
  
  setTime("2022-09-07 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 20.0);
  DOUBLES_EQUAL(210, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-08 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 10.0);
  DOUBLES_EQUAL(300, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-09 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=  5.0);
  DOUBLES_EQUAL(395, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-10 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 10.0);
  DOUBLES_EQUAL(400, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-11 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 35.0);
  DOUBLES_EQUAL(425, rainGauge.currentWeek(), TOLERANCE);  

  // Next Week
  setTime("2022-09-12 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=50.0);
  DOUBLES_EQUAL(0, rainGauge.currentWeek(), TOLERANCE);  

  setTime("2022-09-13 04:00:00", tm, ts);
  rainGauge.update(tm, rainSensor=80.0);
  DOUBLES_EQUAL(30, rainGauge.currentWeek(), TOLERANCE);  

}


/*
 * Test monthly rainfall (no rain gauge overflow)
 */
TEST(TestRainGaugeMonthlyOv, Test_RainMonthlyOv) {
  tm        tm;
  time_t    ts;
  float     rainSensor = 0;
  float     rainMonthly;
    
  printf("< RainMonthlyOv >\n");
  
  setTime("2022-09-06 12:00:00", tm, ts);
  DOUBLES_EQUAL(rainMonthly=0, rainGauge.currentMonth(), TOLERANCE);

  rainGauge.update(tm, rainSensor = 5);
  DOUBLES_EQUAL(rainMonthly = 0, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-07 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 50);
  DOUBLES_EQUAL(rainMonthly = 45, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-08 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 5);
  DOUBLES_EQUAL(rainMonthly = 100, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-09 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 55);
  DOUBLES_EQUAL(rainMonthly = 150, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-10 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 90);
  DOUBLES_EQUAL(rainMonthly = 185, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-11 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 55);
  DOUBLES_EQUAL(rainMonthly = 250, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-12 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 5);
  DOUBLES_EQUAL(rainMonthly = 300, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-13 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 65);
  DOUBLES_EQUAL(rainMonthly = 360, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-14 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 95);
  DOUBLES_EQUAL(rainMonthly = 390, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-15 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 0);
  DOUBLES_EQUAL(rainMonthly = 395, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-16 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 99);
  DOUBLES_EQUAL(rainMonthly = 494, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-17 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 1);
  DOUBLES_EQUAL(rainMonthly = 496, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-18 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 5);
  DOUBLES_EQUAL(rainMonthly = 500, rainGauge.currentMonth(), TOLERANCE);
  
  setTime("2022-09-19 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 25);
  DOUBLES_EQUAL(rainMonthly = 520, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-20 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 20);
  DOUBLES_EQUAL(rainMonthly = 615, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-21 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 80);
  DOUBLES_EQUAL(rainMonthly = 675, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-22 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 30);
  DOUBLES_EQUAL(rainMonthly = 725, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-23 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 40);
  DOUBLES_EQUAL(rainMonthly = 735, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-24 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 60);
  DOUBLES_EQUAL(rainMonthly = 755, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-25 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 90);
  DOUBLES_EQUAL(rainMonthly = 785, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-26 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 30);
  DOUBLES_EQUAL(rainMonthly = 825, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-27 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 80);
  DOUBLES_EQUAL(rainMonthly = 875, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-28 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 40);
  DOUBLES_EQUAL(rainMonthly = 935, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-29 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 10);
  DOUBLES_EQUAL(rainMonthly = 1005, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-09-30 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 15);
  DOUBLES_EQUAL(rainMonthly = 1010, rainGauge.currentMonth(), TOLERANCE);

  // New month
  setTime("2022-10-01 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor = 20);
  DOUBLES_EQUAL(rainMonthly = 0, rainGauge.currentMonth(), TOLERANCE);

  setTime("2022-10-02 12:00:00", tm, ts);
  rainGauge.update(tm, rainSensor+=5);
  DOUBLES_EQUAL(rainMonthly += 5, rainGauge.currentMonth(), TOLERANCE);
  
}

