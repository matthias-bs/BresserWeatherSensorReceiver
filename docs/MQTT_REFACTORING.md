# MQTT Communication Refactoring Guide

**Date**: February 21, 2026  
**Applied to**: BresserWeatherSensorMQTT example  
**Status**: Completed - Ready to apply to other sketches

## Overview

This guide documents the memory optimization and code organization refactoring applied to MQTT communication in the BresserWeatherSensorReceiver project. These changes reduce heap fragmentation, persistent memory usage, and improve code maintainability.

## Changes Applied

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

This refactoring builds on previous optimizations:

1. **Issue 1 (20260221)**: Refactored `publishWeatherdata()` - replaced String concatenation with snprintf
   - Reduced per-sensor temporaries from 5+ to 0
   - Benefit: Reduced heap fragmentation in sensor loop

2. **Issue 2 (20260221)**: Refactored discovery functions - replaced raw string concatenation with JsonDocument
   - Reduced temporaries from 12-15 per call to 1-2
   - Benefit: Significantly reduced fragmentation in discovery

3. **Issue 3 (20260221)**: Converted global String topics to const char*
   - Eliminated 14 String objects (~40-50 bytes each)
   - Benefit: ~560-700 bytes persistent heap savings

4. **Latest (20260221)**: Struct organization
   - Improved code clarity and maintainability
   - No memory impact (same const char* pointers, just organized)
   - Benefit: Easier future maintenance

---

## Notes

- The struct uses `const char*` (not `String`) - these are compile-time constants
- `Hostname` remains a `String` because it's built at runtime with `String(HOSTNAME)`
- No functional changes - only reorganization and optimization
- All changes are backward compatible with MQTT broker
