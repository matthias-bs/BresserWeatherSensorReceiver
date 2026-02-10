# Unit Tests for BresserWeatherSensorReceiver

This directory contains comprehensive unit tests for the BresserWeatherSensorReceiver library using the CppUTest framework.

## Test Coverage

### Current Test Statistics
- **Total Test Files**: 4
- **Total Tests**: 48 (44 active)
- **Total Assertions**: 548+
- **Test Framework**: CppUTest

### Tested Components

#### 1. RainGauge (17 tests)
Tests for rainfall tracking and calculation functionality:
- **Hourly tracking**: Past 60 minutes with various update intervals
- **24-hour tracking**: Rolling 24-hour rainfall measurement
- **Daily/Weekly/Monthly**: Calendar-based rainfall tracking
- **Edge cases**: Time jumps, overflow handling, sensor startup
- **Quality metrics**: Data validity and quality indicators

Files:
- `test/src/TestRainGauge.cpp`

#### 2. Lightning (8 tests)
Tests for lightning event tracking:
- **Basic tracking**: Event counting and distance estimation
- **Hourly statistics**: Past 60 minutes of lightning activity
- **Edge cases**: Counter overflow, sensor startup, irregular updates
- **Update rate changes**: Dynamic update rate configuration

Files:
- `test/src/TestLightning.cpp`

#### 3. WeatherUtils (22 tests)
Tests for weather calculation utility functions:

**Temperature Calculations:**
- `calcdewpoint()` - Dew point temperature (3 tests)
- `calcwindchill()` - Wind chill temperature (2 tests)
- `calcheatindex()` - Heat index (2 tests)
- `calchumidex()` - Humidex (1 test)
- `calcnaturalwetbulb()` - Wet bulb temperature (1 test)
- `calcwbgt()` - Wet bulb globe temperature (2 tests)
- `perceived_temperature()` - Perceived temperature (3 tests)

**Wind Conversions:**
- `windspeed_ms_to_bft()` - Beaufort scale conversion (5 tests)
- `winddir_flt_to_str()` - Wind direction to text (3 tests, ESP32/ESP8266 only)

Files:
- `test/src/TestWeatherUtils.cpp`

### Not Yet Tested
The following components currently lack unit tests:
- `WeatherSensor.cpp` - Main sensor interface (hardware dependent)
- `WeatherSensorConfig.cpp` - Configuration management
- `WeatherSensorDecoders.cpp` - Protocol decoders (hardware dependent)
- `InitBoard.cpp` - Hardware initialization

## Building and Running Tests

### Prerequisites
```bash
sudo apt-get install cpputest
```

### Build and Run All Tests
```bash
cd test
make clean
make
```

### Test Output
Tests will display:
- Individual test results
- Pass/fail status
- Assertion counts
- Execution time

Example output:
```
OK (44 tests, 44 ran, 548 checks, 0 ignored, 0 filtered out, 3 ms)
```

## Test Organization

### Test Groups
Tests are organized into logical groups using CppUTest's `TEST_GROUP` macro:
- `TestRainGaugeHour` - Hourly rainfall tests
- `TestRainGauge24Hours` - 24-hour rainfall tests
- `TestDewPoint` - Dew point calculation tests
- `TestWindChill` - Wind chill tests
- etc.

### Test Naming Convention
- Test files: `Test<Component>.cpp`
- Test cases: `Test_<Functionality>`
- Example: `TEST(TestDewPoint, Test_DewPoint_Positive)`

## Adding New Tests

### 1. Create Test File
```cpp
#include "CppUTest/TestHarness.h"
#include "YourComponent.h"

TEST_GROUP(TestYourComponent) {
  void setup() {
    // Test setup
  }
  void teardown() {
    // Test cleanup
  }
};

TEST(TestYourComponent, Test_YourFunction) {
  // Test implementation
  DOUBLES_EQUAL(expected, actual, tolerance);
}
```

### 2. Update Makefile
Edit `test/makefiles/Makefile_Tests.mk`:
```makefile
SRC_FILES = \
  $(PROJECT_SRC_DIR)/YourComponent.cpp

TEST_SRC_FILES = \
  $(UNITTEST_SRC_DIR)/TestYourComponent.cpp
```

### 3. Run Tests
```bash
cd test
make clean && make
```

## Test Utilities

### Time Manipulation
```cpp
static void setTime(const char *time, tm &tm, time_t &ts) {
  strptime(time, "%Y-%m-%d %H:%M", &tm);
  tm.tm_isdst = -1;
  ts = mktime(&tm);
}
```

### Common Assertions
- `DOUBLES_EQUAL(expected, actual, tolerance)` - Compare floating point values
- `CHECK_EQUAL(expected, actual)` - Compare integers
- `CHECK(condition)` - Boolean assertion
- `CHECK_FALSE(condition)` - Inverse boolean assertion
- `STRCMP_EQUAL(expected, actual)` - Compare strings

## Continuous Integration

Tests can be integrated into CI/CD pipelines:
```bash
#!/bin/bash
cd test
make clean
make
if [ $? -eq 0 ]; then
  echo "All tests passed!"
  exit 0
else
  echo "Tests failed!"
  exit 1
fi
```

## Coverage Improvements

### Recently Added (February 2026)
- ✅ WeatherUtils complete test suite (22 tests)
- ✅ RainGauge 24-hour tracking tests (1 test)
- ✅ 76% increase in total test count

### Future Improvements
- [ ] WeatherSensor mock tests
- [ ] Configuration management tests
- [ ] Decoder validation tests
- [ ] Integration tests for complete workflows

## Contributing

When adding new functionality:
1. Write tests first (TDD approach recommended)
2. Ensure all existing tests pass
3. Aim for >80% code coverage for new features
4. Document test cases and expected behavior
5. Include edge cases and boundary conditions

## License

Tests are licensed under the MIT License, same as the main project.
