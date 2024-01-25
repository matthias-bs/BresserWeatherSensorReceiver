//
//    FILE: unit_test_001.cpp
//  AUTHOR: Rob Tillaart
//    DATE: 2020-12-27
// PURPOSE: unit tests for the MCP4725
//          https://github.com/RobTillaart/MCP4725
//          https://github.com/Arduino-CI/arduino_ci/blob/master/REFERENCE.md
//

// supported assertions
// https://github.com/Arduino-CI/arduino_ci/blob/master/cpp/unittest/Assertion.h#L33-L42
// ----------------------------
// assertEqual(expected, actual)
// assertNotEqual(expected, actual)
// assertLess(expected, actual)
// assertMore(expected, actual)
// assertLessOrEqual(expected, actual)
// assertMoreOrEqual(expected, actual)
// assertTrue(actual)
// assertFalse(actual)
// assertNull(actual)
// assertNotNull(actual)

#include <ArduinoUnitTests.h>


#include "Arduino.h"
#include "../src/RainGauge.h"
#define TOLERANCE 0.11

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

unittest_setup()
{
}


unittest_teardown()
{
}


unittest(test_constructor)
{
  RainGauge rainGauge;
  rainGauge.reset();

  tm        tm;
  time_t    ts;
  float     rainSensor;

  fprintf(stderr, "test start\n");
  printf("< RainHour >\n");
  
  setTime("2022-09-06 8:00", tm, ts);
  rainGauge.update(tm, rainSensor=10.0);
  assertEqualFloat(0, rainGauge.pastHour(), TOLERANCE);  

}



unittest_main()

// --------
