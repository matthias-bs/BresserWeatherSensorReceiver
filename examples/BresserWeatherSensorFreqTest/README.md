# Bresser Weather Sensor Frequency Test

This example helps you find the optimal carrier frequency offset for your CC1101/SX1276/SX1262/LR1121 transceiver module. Different modules can have slight frequency deviations from the nominal 868.3 MHz, and finding the optimal offset can improve reception reliability.

## Background

The method is inspired by the AskSinPP FreqTest example:
- [FreqTest.ino](https://github.com/pa-pa/AskSinPP/blob/master/examples/FreqTest/FreqTest.ino)
- [Determining CC1101 Frequency](https://asksinpp.de/Grundlagen/FAQ/Fehlerhafte_CC1101.html#ermittlung-der-cc1101-frequenz)

## How It Works

1. The sketch scans through a range of frequency offsets around 868.3 MHz
2. At each frequency, it waits for messages from your Bresser weather sensors
3. Messages received and their signal strength (RSSI) are recorded
4. After scanning, the optimal frequency offset is calculated and displayed

## Usage

1. Make sure your weather sensor is actively transmitting (most Bresser sensors transmit every 30-60 seconds)
2. Upload this sketch to your ESP32/ESP8266
3. Open the Serial Monitor (115200 baud)
4. Wait for the scan to complete (approximately 5-10 minutes depending on configuration)
5. Note the recommended frequency offset displayed at the end
6. Use this offset in your main sketch by passing it to the `begin()` method:

```cpp
WeatherSensor ws;
void setup() {
    // Use the frequency offset determined by the FreqTest
    ws.begin(1, true, 0.050);  // Example: +50 kHz offset
}
```

## Configuration

You can adjust the scanning parameters at the top of the sketch:

```cpp
#define SCAN_START_OFFSET_KHZ  -250    // Start scanning at -250 kHz
#define SCAN_END_OFFSET_KHZ     250    // End scanning at +250 kHz
#define SCAN_STEP_KHZ           25     // Scan in 25 kHz steps
#define SCAN_TIME_PER_FREQ_MS   30000  // Wait 30 seconds at each frequency
```

### Quick Scan (Faster but less accurate)
```cpp
#define SCAN_START_OFFSET_KHZ  -100
#define SCAN_END_OFFSET_KHZ     100
#define SCAN_STEP_KHZ           50
#define SCAN_TIME_PER_FREQ_MS   20000
```

### Detailed Scan (Slower but more accurate)
```cpp
#define SCAN_START_OFFSET_KHZ  -300
#define SCAN_END_OFFSET_KHZ     300
#define SCAN_STEP_KHZ           10
#define SCAN_TIME_PER_FREQ_MS   45000
```

## Example Output

```
========================================
Bresser Weather Sensor Frequency Test
========================================

Base frequency: 868.300 MHz
Scan range: -250 to 250 kHz (868.050 to 868.550 MHz)
Scan step: 25 kHz
Time per frequency: 30 seconds

========================================
Scanning Frequencies
========================================

Testing frequency offset:   -250 kHz (868.050 MHz) ... No messages received
Testing frequency offset:   -225 kHz (868.075 MHz) ... No messages received
Testing frequency offset:   -200 kHz (868.100 MHz) .... Received:  4 messages, RSSI: max=-45.2 dBm, avg=-48.3 dBm
...
Testing frequency offset:    +50 kHz (868.350 MHz) ........ Received:  8 messages, RSSI: max=-42.1 dBm, avg=-44.5 dBm
...

========================================
Scan Results Summary
========================================

Offset (kHz) | Frequency (MHz) | Messages | Max RSSI (dBm) | Avg RSSI (dBm)
-------------|-----------------|----------|----------------|----------------
...
    +50      |     868.350     |        8 |          -42.1 |          -44.5
...

========================================
Recommendation
========================================

Based on the scan results:
  Optimal frequency offset: 0.050 MHz
  Optimal frequency: 868.350 MHz
  Messages received: 8
  Average RSSI: -44.5 dBm

To use this frequency offset in your sketch:
  ws.begin(1, true, 0.050);
```

## Troubleshooting

### No messages received during scan

If you see "WARNING: No messages were received during the scan!", check:

1. **Weather sensor is transmitting**: Most Bresser sensors transmit every 30-60 seconds. Make sure the sensor has fresh batteries and is within range.

2. **Radio module connections**: Verify that your CC1101/SX1276/SX1262/LR1121 module is properly connected according to the pin definitions in `WeatherSensorCfg.h`.

3. **Scan time**: Try increasing `SCAN_TIME_PER_FREQ_MS` to give more time to receive messages at each frequency.

4. **Hardware issue**: Try a different radio module if possible.

### Unreliable results

- Run the test multiple times and average the results
- Increase `SCAN_TIME_PER_FREQ_MS` for more stable results
- Reduce `SCAN_STEP_KHZ` for finer resolution

## Notes

- The optimal offset may vary with temperature. Run the test in typical operating conditions.
- Different sensors from the same manufacturer may have slightly different frequencies.
- If you have multiple sensors, the test will find the best compromise frequency.
- A well-calibrated module should have an optimal offset close to 0 kHz.
- Large offsets (>150 kHz) may indicate a defective module.

## See Also

- [BresserWeatherSensorBasic](../BresserWeatherSensorBasic/) - Basic example for receiving weather sensor data
- [Main Repository](https://github.com/matthias-bs/BresserWeatherSensorReceiver)
