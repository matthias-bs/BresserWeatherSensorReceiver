# Bug Fix: Static Initialization Order Fiasco (SIOF) in SPIClass for LilyGo T3S3 Boards

## Symptom

Running `BresserWeatherSensorOLED.ino` on a **LilyGo T3-S3 SX1262** causes a core dump immediately in `setup()`:

```
assert failed: xQueueSemaphoreTake queue.c:1709 (( pxQueue ))
```

Decoded backtrace (excerpt):
```
SPIClass::beginTransaction(SPISettings)     SPI.cpp:200
ArduinoHal::spiBeginTransaction()           ArduinoHal.cpp:103
SX126x::standby()                           SX126x_commands.cpp:32
SX126x::reset()                             SX126x.cpp:196
SX126x::beginFSK()                          SX126x.cpp:81
WeatherSensor::begin()                      WeatherSensor.cpp:216
setup()                                     BresserWeatherSensorOLED.ino:120
```

The sketch `BresserWeatherSensorBasic.ino` runs on the same board without any issue.

---

## Root Cause

**Static Initialization Order Fiasco (SIOF)** in `src/WeatherSensor.cpp`.

### Background

In `WeatherSensor.cpp`, a dedicated `SPIClass` instance for the radio is created as a namespace-scope static object:

```cpp
// Before fix:
SPIClass *spi = new SPIClass(SPI);  // copies global 'SPI' object
```

`SPIClass(uint8_t spi_bus)` — the bus-number constructor — creates a FreeRTOS mutex semaphore (`paramLock`) via `xSemaphoreCreateMutex()`. This `paramLock` is required by `SPIClass::beginTransaction()`:

```cpp
// SPI.cpp
void SPIClass::beginTransaction(SPISettings settings) {
  SPI_PARAM_LOCK();  // asserts if paramLock is NULL
  ...
}

#define SPI_PARAM_LOCK() \
  do { } while (xSemaphoreTake(paramLock, portMAX_DELAY) != pdPASS)
```

The **compiler-generated copy constructor** used by `new SPIClass(SPI)` blindly copies the `paramLock` pointer from the global `SPI` object. It does **not** create a new semaphore.

C++ does **not** guarantee the initialization order of static objects across different translation units. If `WeatherSensorReceiver::spi` is initialized before the global `SPI` object in [SPI.cpp](https://github.com/esp8266/Arduino/blob/master/libraries/SPI/SPI.cpp) has run its constructor (which calls `xSemaphoreCreateMutex()`), then `SPI.paramLock` is `NULL` at copy time, and `spi->paramLock` remains `NULL` permanently.

### Why `BresserWeatherSensorBasic` works but `BresserWeatherSensorOLED` does not

The initialization order of namespace-scope objects depends on the **link order** of translation units. Linking additional libraries (Adafruit GFX, Adafruit SSD1306) shifts the link order such that `WeatherSensorReceiver::spi` is now initialized **before** the global `SPI` object's constructor has a chance to create `paramLock`, triggering the assert. With fewer linked objects, the order happened to be safe — by chance.

---

## Fix

Use a **statically allocated `SPIClass`** with the integer bus-number constructor instead of
heap-allocating a copy of the global `SPI` object:

```cpp
// After fix (src/WeatherSensor.cpp):
// Use a statically allocated SPIClass with the integer bus-number constructor instead of
// copying the global SPI object, to avoid a Static Initialization Order Fiasco (SIOF):
// the compiler-generated copy constructor copies paramLock from SPI, which may be NULL
// if SPI's constructor has not run yet. The integer constructor always creates its own
// paramLock via xSemaphoreCreateMutex().
SPIClass spi(FSPI);
```

`FSPI` is the correct SPI bus number for the LilyGo T3-S3 (as declared in `SPI.cpp`: `SPIClass SPI(FSPI)`). The integer constructor **always** calls `xSemaphoreCreateMutex()`, guaranteeing a valid `paramLock` regardless of static initialization order.

The static object is passed by reference instead of dereferencing a pointer:
```cpp
// Before:
SPIClass *spi = new SPIClass(SPI);
RADIO_CHIP radio = new Module(..., *spi);
spi->begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

// After:
SPIClass spi(FSPI);
RADIO_CHIP radio = new Module(..., spi);
spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
```

### Why static allocation is preferred over `new SPIClass(FSPI)`

Both `new SPIClass(FSPI)` and `SPIClass spi(FSPI)` fix the SIOF, since both use the integer
constructor. However, static allocation is cleaner:

| Approach | Fixes SIOF? | Notes |
|---|---|---|
| `new SPIClass(SPI)` | ❌ No | SIOF — copies `paramLock`, which may be `NULL` |
| `new SPIClass(FSPI)` | ✅ Yes | Integer constructor, but heap-allocates |
| `SPIClass spi(FSPI)` | ✅ Yes | Integer constructor, no heap allocation, no pointer — preferred |

---

## Files Changed

| File | Change |
|------|--------|
| `src/WeatherSensor.cpp` | Replace `SPIClass *spi = new SPIClass(SPI)` with `SPIClass spi(FSPI)`; update all callsites (`->` → `.`, `*spi` → `spi`) |

---

## References

- [C++ Static Initialization Order Fiasco](https://en.cppreference.com/w/cpp/language/siof)
- [`SPIClass` implementation](https://github.com/espressif/arduino-esp32/blob/master/libraries/SPI/src/SPI.cpp)
- LilyGo T3-S3 SX1262 variant: `/home/mp/.arduino15/packages/esp32/hardware/esp32/3.3.7/variants/lilygo_t3_s3_sx1262/pins_arduino.h`
