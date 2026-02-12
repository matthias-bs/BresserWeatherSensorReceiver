# GitHub Copilot Instructions for BresserWeatherSensorReceiver

## Project Overview

**BresserWeatherSensorReceiver** is an Arduino library for receiving and decoding 868 MHz wireless data from Bresser weather sensors using ESP32, ESP8266, or RP2040 microcontrollers with RF transceivers (CC1101, SX1276/RFM95W, SX1262, or LR1121).

### Key Responsibilities
- Receive and decode RF messages from multiple Bresser sensor types (5-in-1, 6-in-1, 7-in-1, 8-in-1 weather stations, lightning sensors, water leakage sensors, etc.)
- Support multiple simultaneous sensors with ID filtering and slot management
- Provide post-processing for rain gauge statistics and lightning detection
- Integrate with MQTT, WiFi, OLED displays, SD cards, and Home Assistant

## Architecture

### Core Classes

| Class | File | Purpose |
|-------|------|---------|
| `WeatherSensor` | `WeatherSensor.h/.cpp` | Main receiver class - RF reception, message decoding, multi-sensor management |
| `RainGauge` | `RainGauge.h/.cpp` | Rain statistics - hourly/daily/weekly/monthly accumulation, overflow handling |
| `Lightning` | `Lightning.h/.cpp` | Lightning strike counting, hourly history, post-processing |
| `InitBoard` | `InitBoard.h/.cpp` | Board-specific initialization for 15+ ESP32/ESP8266 variants |

### Key Design Patterns

1. **Union-Based Memory Efficiency**: Sensor data stored in unions to minimize memory footprint
2. **Conditional Compilation**: Features selectable via `WeatherSensorCfg.h` macros
3. **Circular History Buffers**: 60-minute history for rain/lightning with wraparound
4. **Preferences Storage**: Flash-based persistence for sensor configs, rain/lightning history
5. **Slot-Based Multi-Sensor**: Dynamic `std::vector<Sensor>` for managing multiple sensors
6. **Decoder Chain**: Try decoders in order (7-in-1 → 6-in-1 → 5-in-1 → Lightning → Leakage) until success

## Coding Conventions

### Naming Conventions
- **Classes**: `CamelCase` (e.g., `WeatherSensor`, `RainGauge`)
- **Functions/Methods**: `camelCase` (e.g., `getMessage()`, `getData()`)
- **Variables**: `snake_case` (e.g., `sensor_id`, `wind_speed_ms`)
- **Constants/Macros**: `UPPER_SNAKE_CASE` (e.g., `NUM_SENSORS`, `BRESSER_5_IN_1`)
- **Private Members**: No specific prefix convention

### File Organization
```
src/
├── WeatherSensor.h/.cpp         # Core receiver & decoder orchestration
├── WeatherSensorCfg.h           # User configuration (pins, sensors, decoders)
├── WeatherSensorConfig.cpp      # Runtime configuration via JSON
├── WeatherSensorDecoders.cpp    # Protocol decoders (5/6/7-in-1, Lightning, Leakage)
├── WeatherUtils.h/.cpp          # Utility functions (dew point, wind chill, etc.)
├── RainGauge.h/.cpp            # Rain statistics module
├── Lightning.h/.cpp            # Lightning post-processing module
└── InitBoard.h/.cpp            # Board initialization

examples/
├── BresserWeatherSensorBasic/          # Simple receive & print
├── BresserWeatherSensorMQTT/           # MQTT integration
├── BresserWeatherSensorMQTTCustom/     # Advanced MQTT with local copy of library
└── [12 other examples]

test/
└── src/                         # CppUTest unit tests
```

### Header File Structure
Every source file must include:
1. **File header comment block** with:
   - File name and purpose
   - Project URL: `https://github.com/matthias-bs/BresserWeatherSensorReceiver`
   - Credits/references to original work
   - MIT License text
   - Detailed history log with dates and changes
   - Author: Matthias Prinke
2. **Include guards** (not `#pragma once`)
3. **Arduino.h include** for Arduino types
4. **Doxygen-style comments** for public API

### Documentation Standards
- Use `///` for Doxygen comments on functions/classes
- Use `//` for inline explanations
- Add `@brief`, `@param`, `@return` tags for public methods
- Reference rtl_433 project sources when applicable
- Document protocol details and sensor quirks extensively

### Code Style
- **Indentation**: 4 spaces (no tabs)
- **Braces**: Opening brace on same line for functions/classes
- **Line Length**: No strict limit, but keep readable (~100-120 chars preferred)
- **Comments**: Extensive inline documentation explaining RF protocols, sensor behavior, edge cases
- **Debug Output**: Use `log_d()`, `log_i()`, `log_w()`, `log_e()` macros (ESP32/ESP8266)
- **Conditional Features**: Wrap optional features in `#ifdef FEATURE_NAME`

### Error Handling
- Return `DecodeStatus` enum from decoders (e.g., `DECODE_OK`, `DECODE_DIG_ERR`, `DECODE_CHK_ERR`)
- Use validation flags in structs (e.g., `temp_ok`, `humidity_ok`, `wind_ok`)
- Check sensor startup flag to prevent false rain/lightning counts after battery replacement
- Validate CRC/checksums using `lfsr_digest16()` or `crc16()`

## Common Patterns & Idioms

### Conditional Compilation
Use macros in `WeatherSensorCfg.h` to enable/disable features:
```cpp
#define BRESSER_5_IN_1      // Enable 5-in-1 decoder
#define BRESSER_6_IN_1      // Enable 6-in-1 decoder
#define BRESSER_7_IN_1      // Enable 7-in-1 decoder
#define BRESSER_LIGHTNING   // Enable lightning decoder
#define LIGHTNING_USE_PREFS // Store lightning history in flash
```

### Preferences API (Flash Storage)
```cpp
#include <Preferences.h>
Preferences preferences;
preferences.begin("namespace", false);  // false = read/write
uint32_t value = preferences.getUInt("key", default_value);
preferences.putUInt("key", value);
preferences.end();
```

### Rain Gauge Usage Pattern
```cpp
RainGauge rainGauge;
rainGauge.reset();  // Reset all statistics
rainGauge.update(timestamp, rain_mm, startup_flag);
float hourly = rainGauge.pastHour();
float daily = rainGauge.currentDay();
```

### Lightning Detection Pattern
```cpp
Lightning lightning;
lightning.reset();
lightning.update(timestamp, strike_count, distance_km, startup_flag);
uint8_t strikes_past_hour = lightning.pastHour();
```

### Multi-Sensor Slot Management
```cpp
// Add sensor configuration
std::vector<uint32_t> sensor_ids = {0x39582376};
weatherSensor.setSensorsCfg(sensor_ids);

// Get data with timeout
int decode_status = weatherSensor.getData(RX_TIMEOUT, DATA_COMPLETE);
if (decode_status == DECODE_OK) {
    for (size_t i = 0; i < weatherSensor.sensor.size(); i++) {
        if (weatherSensor.sensor[i].valid) {
            uint32_t id = weatherSensor.sensor[i].sensor_id;
            // Process sensor data...
        }
    }
}
```

### Decoder Implementation
Decoders should:
1. Check message length: `if (msg_size != EXPECTED_SIZE) return DECODE_INVALID;`
2. Validate CRC/checksum: `if (calculated_crc != expected_crc) return DECODE_CHK_ERR;`
3. Extract sensor ID and type
4. Decode fields with proper scaling/units
5. Set validity flags for each field
6. Return `DecodeStatus`

## Platform-Specific Notes

### Supported Platforms
- **ESP32** (primary target) - full feature support
- **ESP8266** - limited RAM, use carefully with multiple sensors
- **RP2040** - limited testing, basic features supported

### Board Configurations
Predefined board configs in `WeatherSensorCfg.h`:
- `LORAWAN_NODE` - Generic LoRaWAN Node
- `TTGO_LORA32` - TTGO ESP32 with integrated SX1276
- `ADAFRUIT_FEATHER_ESP32S2` - Adafruit Feather with RFM95W
- `FIREBEETLE_ESP32` - FireBeetle ESP32
- `ESP32_LORA_V2` - Heltec ESP32 LoRa V2
- `M5CORE2_MODULE_LORA868` - M5Stack Core2 with LoRa module

### Pin Configuration
Define pins in `WeatherSensorCfg.h`:
```cpp
#define PIN_RECEIVER_CS   27   // SPI chip select
#define PIN_RECEIVER_IRQ  21   // CC1101: GDO0 / SX127x: DIO0
#define PIN_RECEIVER_GPIO 33   // CC1101: GDO2 / SX127x: DIO1
#define PIN_RECEIVER_RST  32   // Reset pin (SX127x) or RADIOLIB_NC (CC1101)
```

## Testing

### Framework
- **CppUTest** for unit tests (desktop)
- **arduino-ci** for Arduino library validation

### Test Structure
```cpp
#include "CppUTest/TestHarness.h"
TEST_GROUP(TestGroupName) {
    void setup() { /* Initialize */ }
    void teardown() { /* Cleanup */ }
};

TEST(TestGroupName, TestName) {
    // Arrange
    // Act
    // Assert
    DOUBLES_EQUAL(expected, actual, tolerance);
}
```

### Running Tests
```bash
# Run CppUTest
cd test
make

# Run Arduino CI
# Automated via GitHub Actions
```

## Dependencies

### Core Dependencies
- **RadioLib** (7.5.0) - RF transceiver abstraction layer
- **ArduinoJson** (7.4.2) - JSON parsing for configuration
- **Preferences** (2.2.2) - Flash storage API (ESP32/ESP8266)

### Optional Dependencies
- **MQTT** (2.5.2) - MQTT client for IoT integrations
- **WiFiManager** (2.0.17) - WiFi credential management
- **ESP_DoubleResetDetector** (1.3.2) - Factory reset via double-reset
- **ESPAsyncWebServer** (3.9.4) - Web interface for configuration
- **Adafruit_SSD1306** (2.5.16) - OLED display support
- **RTClib** (2.1.4) - Real-time clock support

## Common Tasks

### Adding a New Sensor Decoder
1. Add sensor type constant in `WeatherSensor.h` (e.g., `#define SENSOR_TYPE_NEW 0xNN`)
2. Add decoder function in `WeatherSensorDecoders.cpp`:
   ```cpp
   DecodeStatus decodeBresserNewPayload(const uint8_t *msg, uint8_t msgSize) {
       // Validate length and CRC
       // Extract sensor ID and type
       // Decode fields with proper units
       // Set validity flags
       return DECODE_OK;
   }
   ```
3. Add decoder to chain in `WeatherSensor::decodeMessage()`
4. Add conditional compilation guard: `#ifdef BRESSER_NEW`
5. Document protocol details in comments

### Adding a New Board Configuration
1. Add board definition to `WeatherSensorCfg.h`:
   ```cpp
   #elif defined(MY_BOARD)
       #define PIN_RECEIVER_CS   XX
       #define PIN_RECEIVER_IRQ  XX
       #define PIN_RECEIVER_GPIO XX
       #define PIN_RECEIVER_RST  XX
   ```
2. Document pin connections
3. Test RF reception and decoding

### Adding Support for a New RF Transceiver
1. Add transceiver type macro: `#define USE_NEW_RADIO`
2. Add RadioLib module initialization in `WeatherSensor::begin()`
3. Configure modulation parameters (FSK, frequency, bandwidth, etc.)
4. Test message reception with known sensors

## Best Practices

### Memory Management
- ESP8266 has limited RAM (~40KB usable) - minimize heap allocations
- Use `std::vector.reserve()` if max sensor count is known
- Prefer stack allocation for temporary buffers
- Use unions for mutually exclusive sensor data types

### RF Reception
- Set appropriate RX timeout based on sensor transmit intervals (typically 30-60 seconds)
- Handle incomplete 6-in-1 messages (alternating temp/humidity and rain/wind)
- Clear slots before each RX cycle to prevent stale data
- Check `startup` flag to prevent false rain/lightning accumulation

### Sensor ID Filtering
- Support both include and exclude lists
- Use `rxFlags.DATA_COMPLETE` cautiously - some sensors never provide complete data
- Handle broadcast ID (`0xFFFFFFFF`) for sensors without unique IDs

### Debugging
- Use `CORE_DEBUG_LEVEL` (ESP32) or `WeatherSensorCfg.h` defines (ESP8266)
- Add debug output with `log_d()`, `log_i()`, `log_v()` macros
- Use `DEBUG_OUTPUT.md` for debug level documentation

### Version Management
- Follow semantic versioning (MAJOR.MINOR.PATCH)
- Update `library.properties` version field
- Document changes in file history blocks
- Update `CHANGELOG.md` (if present)

## Examples Organization

Examples demonstrate progressive complexity:
1. **Basic** - Simple message reception and printing
2. **Waiting/Callback** - Advanced reception strategies
3. **Options** - Configuration customization
4. **OLED** - Display integration
5. **MQTT** - IoT platform integration
6. **MQTTCustom** - Advanced MQTT with local library copy (for development)
7. **Domoticz/M5Core2/CanvasGauges/SDCard** - Specific use cases

## Integration Guidelines

### MQTT Integration
- Publish sensor data as JSON to topic: `<base_topic>/data`
- Support MQTT auto-discovery (Home Assistant format)
- Include sensor ID, type, and all available measurements
- Handle TLS/SSL for secure connections

### Home Assistant Integration
- Use MQTT auto-discovery protocol
- Publish discovery configs to: `homeassistant/<component>/<object_id>/config`
- Set appropriate device classes (temperature, humidity, etc.)
- Include unique IDs based on sensor ID

## Security Considerations
- Never commit WiFi credentials or MQTT passwords
- Use `secrets.h` (gitignored) for sensitive data
- Define `SECRETS` macro when using separate secrets file
- Support secure WiFi (WPA2) and secure MQTT (TLS/SSL)

## References & Credits

### Original Work
- **Bresser5in1-CC1101** by Sean Siford - Initial CC1101 implementation
- **RadioLib** by Jan Gromeš - RF transceiver library
- **rtl_433** by Benjamin Larsson - Protocol specifications and decoders

### Protocol Documentation
- [rtl_433 bresser_5in1.c](https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_5in1.c)
- [rtl_433 bresser_6in1.c](https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_6in1.c)
- [rtl_433 bresser_7in1.c](https://github.com/merbanan/rtl_433/blob/master/src/devices/bresser_7in1.c)

### Project Resources
- **Repository**: https://github.com/matthias-bs/BresserWeatherSensorReceiver
- **Wiki**: https://github.com/matthias-bs/BresserWeatherSensorReceiver/wiki
- **Issues**: https://github.com/matthias-bs/BresserWeatherSensorReceiver/issues
- **Discussions**: Use GitHub Issues for questions and feature requests

## License
This project is licensed under the MIT License. All contributions must maintain the MIT license header in source files.
