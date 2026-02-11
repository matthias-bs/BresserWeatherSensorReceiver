///////////////////////////////////////////////////////////////////////////////////////////////////
// TestWeatherUtils.cpp
//
// CppUTest unit tests for WeatherUtils - weather calculation functions
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
// 20260210 Created
//
// ToDo: 
// -
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "CppUTest/TestHarness.h"
#include "WeatherUtils.h"

#define TOLERANCE 0.1

TEST_GROUP(TestDewPoint) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestWindChill) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestHeatIndex) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestHumidex) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestWetBulb) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestWBGT) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestPerceivedTemp) {
  void setup() {
  }

  void teardown() {
  }
};

TEST_GROUP(TestWindConversions) {
  void setup() {
  }

  void teardown() {
  }
};

/*
 * Test dew point calculation with positive temperatures
 */
TEST(TestDewPoint, Test_DewPoint_Positive) {
  // Test case: 20°C, 65% humidity
  // Expected dew point: approximately 13.2°C
  float dewpoint = calcdewpoint(20.0, 65.0);
  DOUBLES_EQUAL(13.2, dewpoint, TOLERANCE);

  // Test case: 25°C, 50% humidity
  // Expected dew point: approximately 13.9°C
  dewpoint = calcdewpoint(25.0, 50.0);
  DOUBLES_EQUAL(13.9, dewpoint, TOLERANCE);

  // Test case: 30°C, 70% humidity
  // Expected dew point: approximately 23.9°C
  dewpoint = calcdewpoint(30.0, 70.0);
  DOUBLES_EQUAL(23.9, dewpoint, TOLERANCE);
}

/*
 * Test dew point calculation with negative temperatures
 */
TEST(TestDewPoint, Test_DewPoint_Negative) {
  // Test case: -5°C, 80% humidity
  float dewpoint = calcdewpoint(-5.0, 80.0);
  // Expected dew point should be below -5°C
  CHECK(dewpoint < -5.0);
  CHECK(dewpoint > -10.0);

  // Test case: 0°C, 90% humidity
  dewpoint = calcdewpoint(0.0, 90.0);
  // Expected dew point should be close to 0°C
  DOUBLES_EQUAL(-1.3, dewpoint, TOLERANCE);
}

/*
 * Test dew point with extreme values
 */
TEST(TestDewPoint, Test_DewPoint_Extremes) {
  // 100% humidity - dew point equals temperature
  float dewpoint = calcdewpoint(20.0, 100.0);
  DOUBLES_EQUAL(20.0, dewpoint, TOLERANCE);

  // Very low humidity
  dewpoint = calcdewpoint(25.0, 10.0);
  CHECK(dewpoint < 0.0);
}

/*
 * Test windchill calculation
 */
TEST(TestWindChill, Test_WindChill_Normal) {
  // Test case: 5°C, 10 km/h (2.78 m/s)
  // Expected: approximately 2.7°C
  float windchill = calcwindchill(5.0, 2.78);
  DOUBLES_EQUAL(2.7, windchill, TOLERANCE);

  // Test case: 0°C, 20 km/h (5.56 m/s)
  // Expected: approximately -5.2°C
  windchill = calcwindchill(0.0, 5.56);
  DOUBLES_EQUAL(-5.2, windchill, 0.2);

  // Test case: -10°C, 30 km/h (8.33 m/s)
  // Expected: approximately -19.5°C
  windchill = calcwindchill(-10.0, 8.33);
  DOUBLES_EQUAL(-19.5, windchill, 0.2);
}

/*
 * Test windchill with various wind speeds
 */
TEST(TestWindChill, Test_WindChill_WindSpeeds) {
  // Higher wind speed should result in lower perceived temperature
  float wc1 = calcwindchill(5.0, 2.0);
  float wc2 = calcwindchill(5.0, 5.0);
  float wc3 = calcwindchill(5.0, 10.0);
  
  CHECK(wc2 < wc1);
  CHECK(wc3 < wc2);
}

/*
 * Test heat index calculation
 */
TEST(TestHeatIndex, Test_HeatIndex_Normal) {
  // Test case: 30°C, 60% humidity
  // Expected: approximately 32.8°C
  float heatindex = calcheatindex(30.0, 60.0);
  DOUBLES_EQUAL(32.8, heatindex, 0.5);

  // Test case: 35°C, 70% humidity
  // Expected: approximately 50.3°C
  heatindex = calcheatindex(35.0, 70.0);
  DOUBLES_EQUAL(50.3, heatindex, 1.0);

  // Test case: 25°C, 50% humidity
  // Expected: approximately 25.7°C
  heatindex = calcheatindex(25.0, 50.0);
  DOUBLES_EQUAL(25.7, heatindex, 0.5);
}

/*
 * Test heat index increases with humidity
 */
TEST(TestHeatIndex, Test_HeatIndex_Humidity) {
  // Higher humidity should result in higher perceived temperature
  float hi1 = calcheatindex(30.0, 40.0);
  float hi2 = calcheatindex(30.0, 60.0);
  float hi3 = calcheatindex(30.0, 80.0);
  
  CHECK(hi2 > hi1);
  CHECK(hi3 > hi2);
}

/*
 * Test Humidex calculation
 */
TEST(TestHumidex, Test_Humidex_Normal) {
  // Test case: 30°C, 80% humidity
  float humidex = calchumidex(30.0, 80.0);
  // Humidex should be higher than actual temperature
  CHECK(humidex > 30.0);
  CHECK(humidex < 50.0);

  // Test case: 25°C, 60% humidity
  humidex = calchumidex(25.0, 60.0);
  CHECK(humidex > 25.0);
  CHECK(humidex < 35.0);
}

/*
 * Test natural wet bulb temperature
 */
TEST(TestWetBulb, Test_WetBulb_Normal) {
  // Test case: 30°C, 50% humidity
  float wetbulb = calcnaturalwetbulb(30.0, 50.0);
  // Wet bulb should be between dew point and temperature
  float dewpoint = calcdewpoint(30.0, 50.0);
  CHECK(wetbulb > dewpoint);
  CHECK(wetbulb < 30.0);

  // Test case: 20°C, 70% humidity
  wetbulb = calcnaturalwetbulb(20.0, 70.0);
  dewpoint = calcdewpoint(20.0, 70.0);
  CHECK(wetbulb > dewpoint);
  CHECK(wetbulb < 20.0);
}

/*
 * Test WBGT calculation
 */
TEST(TestWBGT, Test_WBGT_Normal) {
  // Test case: wet bulb=25°C, globe=35°C, dry=30°C
  // WBGT = 0.7*25 + 0.2*35 + 0.1*30 = 17.5 + 7.0 + 3.0 = 27.5
  float wbgt = calcwbgt(25.0, 35.0, 30.0);
  DOUBLES_EQUAL(27.5, wbgt, 0.01);

  // Test case: all same temperature
  wbgt = calcwbgt(20.0, 20.0, 20.0);
  DOUBLES_EQUAL(20.0, wbgt, 0.01);
}

/*
 * Test WBGT weighted formula
 */
TEST(TestWBGT, Test_WBGT_Weights) {
  // Wet bulb has most weight (0.7)
  float wbgt1 = calcwbgt(30.0, 25.0, 25.0);
  float wbgt2 = calcwbgt(25.0, 30.0, 25.0);
  
  // WBGT with higher wet bulb should be higher
  CHECK(wbgt1 > wbgt2);
}

/*
 * Test perceived temperature - windchill case
 */
TEST(TestPerceivedTemp, Test_PerceivedTemp_WindChill) {
  // Conditions for windchill: temp <= 10°C, wind > 4.8 km/h
  float perceived = perceived_temperature(5.0, 2.0, 50.0);
  // Should apply windchill (2.0 m/s = 7.2 km/h > 4.8)
  CHECK(perceived < 5.0);

  // Very cold with wind
  perceived = perceived_temperature(-5.0, 5.0, 50.0);
  CHECK(perceived < -5.0);
}

/*
 * Test perceived temperature - heat index case
 */
TEST(TestPerceivedTemp, Test_PerceivedTemp_HeatIndex) {
  // Conditions for heat index: temp >= 16.7°C, humidity > 40%
  float perceived = perceived_temperature(30.0, 1.0, 60.0);
  // Should apply heat index
  CHECK(perceived > 30.0);

  // Hot and humid
  perceived = perceived_temperature(35.0, 0.5, 70.0);
  CHECK(perceived > 35.0);
}

/*
 * Test perceived temperature - neutral case
 */
TEST(TestPerceivedTemp, Test_PerceivedTemp_Neutral) {
  // Conditions that don't meet windchill or heat index criteria
  float perceived = perceived_temperature(15.0, 1.0, 30.0);
  // Should return actual temperature
  DOUBLES_EQUAL(15.0, perceived, 0.01);

  perceived = perceived_temperature(12.0, 0.5, 50.0);
  DOUBLES_EQUAL(12.0, perceived, 0.01);
}

/*
 * Test wind speed to Beaufort scale conversion
 */
TEST(TestWindConversions, Test_Beaufort_Calm) {
  // 0 Bft: < 0.9 m/s
  CHECK_EQUAL(0, windspeed_ms_to_bft(0.0));
  CHECK_EQUAL(0, windspeed_ms_to_bft(0.5));
  CHECK_EQUAL(0, windspeed_ms_to_bft(0.8));
}

TEST(TestWindConversions, Test_Beaufort_Light) {
  // 1 Bft: 0.9 - 1.5 m/s
  CHECK_EQUAL(1, windspeed_ms_to_bft(1.0));
  CHECK_EQUAL(1, windspeed_ms_to_bft(1.5));
  
  // 2 Bft: 1.6 - 3.3 m/s
  CHECK_EQUAL(2, windspeed_ms_to_bft(2.0));
  CHECK_EQUAL(2, windspeed_ms_to_bft(3.0));
  
  // 3 Bft: 3.4 - 5.4 m/s
  CHECK_EQUAL(3, windspeed_ms_to_bft(4.0));
  CHECK_EQUAL(3, windspeed_ms_to_bft(5.0));
}

TEST(TestWindConversions, Test_Beaufort_Moderate) {
  // 4 Bft: 5.5 - 7.9 m/s
  CHECK_EQUAL(4, windspeed_ms_to_bft(6.0));
  CHECK_EQUAL(4, windspeed_ms_to_bft(7.5));
  
  // 5 Bft: 8.0 - 10.7 m/s
  CHECK_EQUAL(5, windspeed_ms_to_bft(9.0));
  CHECK_EQUAL(5, windspeed_ms_to_bft(10.5));
  
  // 6 Bft: 10.8 - 13.8 m/s
  CHECK_EQUAL(6, windspeed_ms_to_bft(12.0));
  CHECK_EQUAL(6, windspeed_ms_to_bft(13.5));
  
  // 7 Bft: 13.9 - 17.1 m/s
  CHECK_EQUAL(7, windspeed_ms_to_bft(15.0));
  CHECK_EQUAL(7, windspeed_ms_to_bft(17.0));
}

TEST(TestWindConversions, Test_Beaufort_Strong) {
  // 8 Bft: 17.2 - 20.7 m/s
  CHECK_EQUAL(8, windspeed_ms_to_bft(18.0));
  CHECK_EQUAL(8, windspeed_ms_to_bft(20.0));
  
  // 9 Bft: 20.8 - 24.4 m/s
  CHECK_EQUAL(9, windspeed_ms_to_bft(22.0));
  CHECK_EQUAL(9, windspeed_ms_to_bft(24.0));
  
  // 10 Bft: 24.5 - 28.4 m/s
  CHECK_EQUAL(10, windspeed_ms_to_bft(26.0));
  CHECK_EQUAL(10, windspeed_ms_to_bft(28.0));
  
  // 11 Bft: 28.5 - 32.6 m/s
  CHECK_EQUAL(11, windspeed_ms_to_bft(30.0));
  CHECK_EQUAL(11, windspeed_ms_to_bft(32.0));
  
  // 12 Bft: >= 32.7 m/s
  CHECK_EQUAL(12, windspeed_ms_to_bft(33.0));
  CHECK_EQUAL(12, windspeed_ms_to_bft(40.0));
  CHECK_EQUAL(12, windspeed_ms_to_bft(50.0));
}

TEST(TestWindConversions, Test_Beaufort_Boundaries) {
  // Test boundary conditions
  CHECK_EQUAL(0, windspeed_ms_to_bft(0.89));
  CHECK_EQUAL(1, windspeed_ms_to_bft(0.91));
  CHECK_EQUAL(1, windspeed_ms_to_bft(1.59));
  CHECK_EQUAL(2, windspeed_ms_to_bft(1.6));
  CHECK_EQUAL(2, windspeed_ms_to_bft(3.39));
  CHECK_EQUAL(3, windspeed_ms_to_bft(3.4));
}

#if defined(ESP32) || defined(ESP8266)
/*
 * Test wind direction to string conversion
 * Note: This test is only available on ESP32/ESP8266
 */
TEST(TestWindConversions, Test_WindDirection_Cardinals) {
  char buf[4];
  
  // North
  winddir_flt_to_str(0.0, buf);
  STRCMP_EQUAL("N", buf);
  
  winddir_flt_to_str(360.0, buf);
  STRCMP_EQUAL("N", buf);
  
  // East
  winddir_flt_to_str(90.0, buf);
  STRCMP_EQUAL("E", buf);
  
  // South
  winddir_flt_to_str(180.0, buf);
  STRCMP_EQUAL("S", buf);
  
  // West
  winddir_flt_to_str(270.0, buf);
  STRCMP_EQUAL("W", buf);
}

TEST(TestWindConversions, Test_WindDirection_Ordinals) {
  char buf[4];
  
  // Northeast
  winddir_flt_to_str(45.0, buf);
  STRCMP_EQUAL("NE", buf);
  
  // Southeast
  winddir_flt_to_str(135.0, buf);
  STRCMP_EQUAL("SE", buf);
  
  // Southwest
  winddir_flt_to_str(225.0, buf);
  STRCMP_EQUAL("SW", buf);
  
  // Northwest
  winddir_flt_to_str(315.0, buf);
  STRCMP_EQUAL("NW", buf);
}

TEST(TestWindConversions, Test_WindDirection_Secondary) {
  char buf[4];
  
  // North-Northeast
  winddir_flt_to_str(22.5, buf);
  STRCMP_EQUAL("NNE", buf);
  
  // East-Northeast
  winddir_flt_to_str(67.5, buf);
  STRCMP_EQUAL("ENE", buf);
  
  // South-Southwest
  winddir_flt_to_str(202.5, buf);
  STRCMP_EQUAL("SSW", buf);
  
  // West-Northwest
  winddir_flt_to_str(292.5, buf);
  STRCMP_EQUAL("WNW", buf);
}
#endif
