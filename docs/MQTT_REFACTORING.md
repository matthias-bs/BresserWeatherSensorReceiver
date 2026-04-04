# MQTT Communication Refactoring Guide

**Created**: February 21, 2026  
**Last updated**: April 3, 2026  
**Applied to**: All three MQTT example sketches (MQTT, MQTTCustom, MQTTWifiMgr)  
**Status**: Completed

## Overview

This guide documents the memory optimization and code organization refactoring applied to MQTT communication in the BresserWeatherSensorReceiver project. These changes reduce heap fragmentation, persistent memory usage, and improve code maintainability.

---

## Changes Applied (April 3, 2026)

### 5. Stack-Allocated Payload Buffers in `publishWeatherdata()`

**Files changed**: `src/mqtt_comm.h`, `src/mqtt_comm.cpp` (all 3 variants)

**Problem**: `publishWeatherdata()` allocated three `String` objects (`payloadSensor`, `payloadExtra`, `payloadCombined`) on every call. Each sensor iteration caused repeated heap alloc/free cycles, increasing fragmentation over time on the ESP32's heap.

**Solution (Option A — stack char[])**: Replace `String` payloads with stack-allocated `char[]` buffers. Use the size-limited overload of `serializeJson()` to write directly into the fixed arrays. Detect truncation via the return value.

**Changes to `mqtt_comm.h`**:

```cpp
#define PAYLOAD_SIZE 400
// Worst-case extra payload estimate:
//   wind_dir_txt(20) + wind_gust_bft(18) + wind_avg_bft(17) + dewpoint_c(22) +
//   perceived_temp_c(29) + wgbt(15) + JSON overhead(7) = 128 B.
//   160 provides a ~25% safety margin. Increase if new fields are added to jsonExtra.
#define PAYLOAD_EXTRA_SIZE 160  // maximum size for extra (derived) payload
```

**Changes to `mqtt_comm.cpp`** inside `publishWeatherdata()`:

```cpp
// Before
String payloadSensor;
String payloadExtra;
String payloadCombined;
// ...
serializeJson(jsonSensor, payloadSensor);
client.publish(mqtt_topic, payloadSensor.c_str(), retain, 0);
if (payloadExtra != "null") { ... }

// After
char payloadSensor[PAYLOAD_SIZE];
char payloadExtra[PAYLOAD_EXTRA_SIZE];
char payloadCombined[PAYLOAD_SIZE];
// ...
size_t json_size   = serializeJson(jsonSensor,    payloadSensor,    sizeof(payloadSensor));
size_t extra_size  = serializeJson(jsonExtra,     payloadExtra,     sizeof(payloadExtra));
// Overflow detection
if (json_size  >= PAYLOAD_SIZE       - 1) { log_e("payloadSensor truncated!"); }
if (extra_size >= PAYLOAD_EXTRA_SIZE - 1) { log_e("payloadExtra truncated!"); }
// ...
client.publish(mqtt_topic, payloadSensor, retain, 0);  // char* passed directly
if (strcmp(payloadExtra, "null") != 0) { ... }         // strcmp replaces String !=
```

**Stack cost**: `PAYLOAD_SIZE*2 + PAYLOAD_EXTRA_SIZE = 960 B`. Safe on ESP32 (8 KB default stack). Do not increase `PAYLOAD_SIZE` when compiling for ESP8266 (4 KB stack limit).

**Benefit**: Eliminates per-publish-cycle heap allocation for the three largest runtime buffers. Reduces long-term heap fragmentation on both ESP32 and ESP8266.

---

## Changes Applied (March 12, 2026)

### 6. Bug Fix: `sensorData` → `jsonSensor` in `DATA_TIMESTAMP` block

**Files changed**: `src/mqtt_comm.cpp` (all 3 variants)

**Problem**: In the `#ifdef DATA_TIMESTAMP` block inside `publishWeatherdata()`, `sensorData["timestamp"]` referenced an undefined variable (`sensorData`). The correct local `JsonDocument` variable is `jsonSensor`. The bug was latent — the `DATA_TIMESTAMP` feature was presumably never tested with active timestamps.

**Fix**:
```cpp
// Before
sensorData["timestamp"] = ...;

// After
jsonSensor["timestamp"] = ...;
```

---

### 7. Cleanup: Unused `topic` Variable in `haAutoDiscovery()`

**Files changed**: `src/mqtt_comm.cpp` (all 3 variants)

**Problem**: An outer `String topic;` declaration in `haAutoDiscovery()` was shadowed by an inner `String topic` declared in nested blocks. The outer declaration was never read or written to.

**Fix**: Removed the unused outer `String topic;` declaration.

---

### 8. Cleanup: Redundant `payload.clear()` in `publishRadio()`

**Files changed**: `src/mqtt_comm.cpp` (all 3 variants)

**Problem**: `publishRadio()` contained `payload = ""; payload.clear();` — the `clear()` call is a no-op because `payload` was already set to the empty string on the preceding line.

**Fix**: Removed the redundant `payload.clear();` call.

---

## Changes Applied (February 21, 2026)

### 1. Struct-Based MQTT Topics Organization

**File**: `src/mqtt_comm.h`

**Add struct definition** (before extern declarations):

```cpp
// MQTT topics structure for organized access
struct MQTTTopics {
    const char* pubStatus;
    const char* pubRadio;
    const char* pubData;
    const char* pubCombined;
    const char* pubRssi;
    const char* pubExtra;
    const char* pubInc;
    const char* pubExc;
    const char* subReset;
    const char* subGetInc;
    const char* subGetExc;
    const char* subSetInc;
    const char* subSetExc;
};
```

**Add extern declarations** (at end of file, before `#endif`):

```cpp
extern MQTTTopics mqttTopics;
extern String Hostname;
```

---

### 2. Global Declarations in Sketch

**File**: `BresserWeatherSensorMQTT.ino` (or equivalent sketch)

**Replace individual MQTT topic declarations with struct instance**:

```cpp
String Hostname = String(HOSTNAME);
MQTTTopics mqttTopics = {
    .pubStatus = "status",
    .pubRadio = "radio",
    .pubData = "data",
    .pubCombined = "combined",
    .pubRssi = "rssi",
    .pubExtra = "extra",
    .pubInc = "sensors_inc",
    .pubExc = "sensors_exc",
    .subReset = "reset",
    .subGetInc = "get_sensors_inc",
    .subGetExc = "get_sensors_exc",
    .subSetInc = "set_sensors_inc",
    .subSetExc = "set_sensors_exc"
};
```

**What to remove**:
- Individual declarations like: `const char* mqttPubStatus = "status";`
- All 13 individual topic variable declarations

---

### 3. Extern Declarations in Implementation

**File**: `src/mqtt_comm.cpp`

**Replace all individual extern declarations with**:

```cpp
extern String Hostname;
extern MQTTTopics mqttTopics;
```

**What to remove**:
```cpp
extern const char* mqttPubData;
extern const char* mqttPubCombined;
extern const char* mqttPubRssi;
extern const char* mqttPubStatus;
extern const char* mqttPubRadio;
extern const char* mqttPubExtra;
extern const char* mqttPubInc;
extern const char* mqttPubExc;
extern const char* mqttSubReset;
extern const char* mqttSubGetInc;
extern const char* mqttSubGetExc;
extern const char* mqttSubSetInc;
extern const char* mqttSubSetExc;
```

---

### 4. Usage Updates in Implementation

**File**: `src/mqtt_comm.cpp`

**Replace all MQTT topic variable references using the pattern**:

| Old | New |
|-----|-----|
| `mqttPubStatus` | `mqttTopics.pubStatus` |
| `mqttPubRadio` | `mqttTopics.pubRadio` |
| `mqttPubData` | `mqttTopics.pubData` |
| `mqttPubCombined` | `mqttTopics.pubCombined` |
| `mqttPubRssi` | `mqttTopics.pubRssi` |
| `mqttPubExtra` | `mqttTopics.pubExtra` |
| `mqttPubInc` | `mqttTopics.pubInc` |
| `mqttPubExc` | `mqttTopics.pubExc` |
| `mqttSubReset` | `mqttTopics.subReset` |
| `mqttSubGetInc` | `mqttTopics.subGetInc` |
| `mqttSubGetExc` | `mqttTopics.subGetExc` |
| `mqttSubSetInc` | `mqttTopics.subSetInc` |
| `mqttSubSetExc` | `mqttTopics.subSetExc` |

**Key locations to update**:
- `messageReceived()` function - all topic comparisons
- `publishWeatherdata()` function - snprintf calls
- `publishRadio()` function - topic references
- Any `client.publish()` calls using these topics

**Example**: 
```cpp
// Before
snprintf(mqtt_topic, sizeof(mqtt_topic), "%s/%s/%s", 
         Hostname.c_str(), sensor_str.c_str(), mqttPubData);

// After
snprintf(mqtt_topic, sizeof(mqtt_topic), "%s/%s/%s", 
         Hostname.c_str(), sensor_str.c_str(), mqttTopics.pubData);
```

---

## Files to Update

Apply these changes to the following example sketches:

1. ✅ `examples/BresserWeatherSensorMQTT/BresserWeatherSensorMQTT.ino`
2. ✅ `examples/BresserWeatherSensorMQTTCustom/BresserWeatherSensorMQTTCustom.ino`
3. ✅ `examples/BresserWeatherSensorMQTTWifiMgr/BresserWeatherSensorMQTTWifiMgr.ino`

(All sketches have now been updated)

---

## Benefits

| Aspect | Improvement |
|--------|-------------|
| **Code Organization** | Single struct instead of 13+ scattered declarations |
| **Maintainability** | Easier to add/remove MQTT topics - modify struct once |
| **Memory Usage** | No change in runtime memory (still const char*) |
| **Type Safety** | Struct enforces consistent member types |
| **Readability** | Grouped pub/sub topics make intent clear |

---

## Memory Optimization History

This section tracks all memory-related improvements to `mqtt_comm.cpp`:

1. **Change 1 (20260221)**: Refactored `publishWeatherdata()` — replaced String concatenation with snprintf
   - Reduced per-sensor temporaries from 5+ to 0
   - Benefit: Reduced heap fragmentation in sensor loop

2. **Change 2 (20260221)**: Refactored discovery functions — replaced raw string concatenation with JsonDocument
   - Reduced temporaries from 12–15 per call to 1–2
   - Benefit: Significantly reduced fragmentation in discovery

3. **Change 3 (20260221)**: Converted global String topics to `const char*`
   - Eliminated 14 `String` objects (~40–50 bytes each)
   - Benefit: ~560–700 bytes persistent heap savings

4. **Change 4 (20260221)**: Struct organization (`MQTTTopics`)
   - Grouped 13 individual `const char*` topic variables into one struct
   - No runtime memory impact; improves maintainability

5. **Change 5 (20260312)**: Bug fix — `sensorData` → `jsonSensor` in `DATA_TIMESTAMP` block
   - Latent undefined-variable bug in conditional timestamp code path
   - Applied to all 3 MQTT variants

6. **Change 6 (20260312)**: Removed unused outer `String topic` in `haAutoDiscovery()`
   - Eliminated a shadowed `String` allocation that was never read
   - Applied to all 3 MQTT variants

7. **Change 7 (20260312)**: Removed redundant `payload.clear()` in `publishRadio()`
   - No-op call after `payload = ""`; pure cleanup
   - Applied to all 3 MQTT variants

8. **Change 8 (20260403)**: Stack-allocated `char[]` payloads in `publishWeatherdata()`
   - Replaced `String payloadSensor/Extra/Combined` with `char payloadSensor[PAYLOAD_SIZE]`, `char payloadExtra[PAYLOAD_EXTRA_SIZE]`, `char payloadCombined[PAYLOAD_SIZE]`
   - Added `PAYLOAD_EXTRA_SIZE 160` constant (with worst-case size estimate comment) to `mqtt_comm.h`
   - Added overflow detection via `serializeJson()` return value comparison
   - Replaced `payloadExtra != "null"` with `strcmp(payloadExtra, "null") != 0`
   - Benefit: Eliminates per-publish heap alloc/free for the three largest runtime buffers; reduces long-term heap fragmentation
   - Applied to all 3 MQTT variants

9. **Change 9 (20260403)**: Eliminated remaining `String` heap allocations in `haAutoDiscovery()`, `publishAutoDiscovery()`, `publishStatusDiscovery()`, `publishControlDiscovery()`
   - **Why**: The Arduino `String` class heap-allocates memory on every construction and concatenation. In embedded systems without memory management, repeated alloc/free cycles from the discovery and status publish paths cause heap fragmentation. Over time, fragmentation can exhaust the allocator's ability to satisfy new requests, causing silent truncation or hard crashes — particularly dangerous in a long-running system. Replacing `String` temporaries with stack-allocated `char[]` buffers eliminates this fragmentation entirely; stack memory is reclaimed automatically when the function returns with no allocator involvement.
   - `sensor_info` struct members changed from `String` to `const char*`
   - `publishStatusDiscovery()`/`publishControlDiscovery()` signatures changed from `String` to `const char*`
   - `mqtt_connect()` forward declaration uncommented in all 3 headers
   - `haAutoDiscovery()`: Replaced `String topic`/`String rssi` + 2 shadowing inner `String topic` with `char topicData[128]`, `topicRssi[128]`, `topicExtra[128]` + `snprintf`
   - `publishAutoDiscovery()`: Replaced 5 `String` temporaries (`uniqueId`, `availTopic`, `valTmpl`, `deviceName`, `discTopic`) with `char[]` + `snprintf`; cast `sensor_id` to `(unsigned)` to fix `-Wformat=` warning; replaced `info.model != ""` with `info.model[0] != '\0'`
   - `publishStatusDiscovery()`/`publishControlDiscovery()`: Replaced `String discoveryPayload` with `char discoveryPayload[512]`; replaced 3–5 `String` temporaries per function with `char[]` + `snprintf`
   - WifiMgr: Removed stale `#define PAYLOAD_SIZE 300` from `.ino` (obsolete since header was updated to 400); `MQTTClient client(PAYLOAD_SIZE)` now correctly uses the header value
   - Benefit: Eliminates all heap-allocated `String` objects from discovery and status publish paths
   - Applied to all 3 MQTT variants

10. **Change 10 (20260404)**: Eliminated hot-path `String` allocations in `publishWeatherdata()` and `haAutoDiscovery()`
    - **Why**: `sensorName()` returning a `String` value-constructs and heap-allocates a temporary on every call, which happens once per sensor per publish cycle and once per sensor during HA discovery. `String(rssi, 1)` heap-allocates a formatted float string on every sensor publish. `String("0x") + String(unknown, HEX)` causes two heap alloc/frees per lightning sensor per cycle. All three are avoidable in tight embedded loops.
    - `sensorName(uint32_t)` → `void sensorName(uint32_t, char* buf, size_t buf_size)`: function now writes into a caller-supplied stack buffer using `snprintf`; `sensor_map[n].name.c_str()` used directly
    - All call sites in `publishWeatherdata()` and `haAutoDiscovery()`: `String sensor_str = sensorName(...)` → `char sensor_str[32]; sensorName(..., sensor_str, sizeof(sensor_str))`; `.c_str()` calls removed
    - RSSI publish: `String(weatherSensor.sensor[i].rssi, 1)` → `char rssiStr[12]; snprintf(rssiStr, ..., "%.1f", ...)`
    - Lightning unknowns: `String("0x") + String(unknown1, HEX)` → `char lgtUnknown1[12]; snprintf(lgtUnknown1, ..., "0x%x", unknown1)` (same for unknown2)
    - Benefit: Eliminates 3–5 heap alloc/free cycles per sensor per publish cycle
    - Applied to all 3 MQTT variants

11. **Change 11 (20260404)**: Eliminated `String` allocation in `publishRadio()`
    - **Why**: `String mqtt_payload` heap-allocates a buffer for a tiny JSON object (`{"rssi":-XXX.X}`, ~20 bytes). Even though this function is called infrequently, there is no reason to use the heap for a predictably small output.
    - Replaced `String mqtt_payload` with `char mqtt_payload[32]`; `serializeJson(payload, mqtt_payload)` → `serializeJson(payload, mqtt_payload, sizeof(mqtt_payload))`; `.c_str()` removed from `log_i` call
    - Applied to all 3 MQTT variants

---

## Notes

- The struct uses `const char*` (not `String`) - these are compile-time constants
- `Hostname` remains a `String` because it's built at runtime with `String(HOSTNAME)`
- No functional changes - only reorganization and optimization
- All changes are backward compatible with MQTT broker
