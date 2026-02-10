///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorFreqTest.ino
//
// Example for BresserWeatherSensorReceiver - 
// Frequency calibration utility for finding the optimal carrier frequency offset.
//
// This sketch helps to determine the optimal frequency setting for your CC1101/SX1276/SX1262/LR1121
// transceiver module in relation to your specific Bresser weather sensor transmitter.
//
// The method is inspired by the AskSinPP FreqTest example:
// https://github.com/pa-pa/AskSinPP/blob/master/examples/FreqTest/FreqTest.ino
// https://asksinpp.de/Grundlagen/FAQ/Fehlerhafte_CC1101.html#ermittlung-der-cc1101-frequenz
//
// How it works:
// 1. The sketch starts at the default frequency (868.3 MHz) and scans up and down in steps
// 2. For each frequency offset tested, it waits for messages from Bresser weather sensors
// 3. Messages received and their RSSI values are recorded
// 4. After scanning, the optimal frequency offset is calculated and displayed
//
// Usage:
// 1. Upload this sketch to your ESP32/ESP8266
// 2. Open the Serial Monitor (115200 baud)
// 3. Wait for the scan to complete (this may take several minutes)
// 4. Note the optimal frequency offset displayed at the end
// 5. Use this offset value in your main sketch by passing it to ws.begin(1, true, offset)
//
// Note: Make sure your weather sensor is actively transmitting during the test.
// Most Bresser sensors transmit every 30-60 seconds.
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
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

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "InitBoard.h"

// Configuration for frequency scanning
#define SCAN_START_OFFSET_KHZ  -250    // Start scanning at -250 kHz from base frequency
#define SCAN_END_OFFSET_KHZ     250    // End scanning at +250 kHz from base frequency
#define SCAN_STEP_KHZ           25     // Scan in 25 kHz steps
#define SCAN_TIME_PER_FREQ_MS   60000  // Wait 60 seconds at each frequency for messages

// Structure to store results for each frequency tested
struct FreqTestResult {
    double offset_mhz;      // Frequency offset in MHz
    int message_count;      // Number of messages received
    float max_rssi;         // Maximum RSSI value observed
    float avg_rssi;         // Average RSSI value
};

WeatherSensor ws;

// Array to store scan results
#define MAX_SCAN_POINTS ((SCAN_END_OFFSET_KHZ - SCAN_START_OFFSET_KHZ) / SCAN_STEP_KHZ + 1)
FreqTestResult scanResults[MAX_SCAN_POINTS];
int scanResultCount = 0;

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(false);  // Reduce debug output for cleaner results
    delay(1000);
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Bresser Weather Sensor Frequency Test");
    Serial.println("========================================");
    Serial.println();
    Serial.println("This utility will scan different frequency offsets");
    Serial.println("to find the optimal setting for your weather sensor.");
    Serial.println();
    Serial.printf("Base frequency: 868.300 MHz\n");
    Serial.printf("Scan range: %d to %d kHz (%.3f to %.3f MHz)\n", 
                  SCAN_START_OFFSET_KHZ, SCAN_END_OFFSET_KHZ,
                  868.3 + (SCAN_START_OFFSET_KHZ/1000.0), 
                  868.3 + (SCAN_END_OFFSET_KHZ/1000.0));
    Serial.printf("Scan step: %d kHz\n", SCAN_STEP_KHZ);
    Serial.printf("Time per frequency: %d seconds\n", SCAN_TIME_PER_FREQ_MS / 1000);
    Serial.println();
    Serial.println("Make sure your weather sensor is transmitting!");
    Serial.println("Most sensors transmit every 30-60 seconds.");
    Serial.println();
    Serial.println("Starting scan in 5 seconds...");
    delay(5000);
    Serial.println();
    
    initBoard();
    
    // Perform frequency scan
    performFrequencyScan();
    
    // Analyze and display results
    analyzeResults();
}

void performFrequencyScan() {
    Serial.println("========================================");
    Serial.println("Scanning Frequencies");
    Serial.println("========================================");
    Serial.println();
    
    int currentResult = 0;
    
    for (int offset_khz = SCAN_START_OFFSET_KHZ; 
         offset_khz <= SCAN_END_OFFSET_KHZ; 
         offset_khz += SCAN_STEP_KHZ) {
        
        double offset_mhz = offset_khz / 1000.0;
        
        Serial.printf("Testing frequency offset: %+7d kHz (%.3f MHz) ... ",
                      offset_khz, 868.3 + offset_mhz);
        
        // Initialize receiver with current frequency offset
        // Use MAX_SENSORS_DEFAULT to support multiple sensors during calibration
        ws.begin(MAX_SENSORS_DEFAULT, true, offset_mhz);
        
        // Clear any previous sensor data
        ws.clearSlots();
        
        // Statistics for this frequency
        int msg_count = 0;
        float rssi_sum = 0.0;
        float max_rssi = -200.0;
        
        // Wait and collect messages at this frequency
        unsigned long start_time = millis();
        while (millis() - start_time < SCAN_TIME_PER_FREQ_MS) {
            int decode_status = ws.getMessage();
            
            if (decode_status == DECODE_OK) {
                msg_count++;
                float rssi = ws.rssi;  // Use last packet RSSI instead of assuming sensor[0]
                rssi_sum += rssi;
                if (rssi > max_rssi) {
                    max_rssi = rssi;
                }
                
                // Show progress indicator
                Serial.print(".");
            }
            
            delay(10);  // Small delay to prevent tight loop
        }
        
        // Store results
        scanResults[currentResult].offset_mhz = offset_mhz;
        scanResults[currentResult].message_count = msg_count;
        scanResults[currentResult].max_rssi = max_rssi;
        scanResults[currentResult].avg_rssi = (msg_count > 0) ? (rssi_sum / msg_count) : -200.0;
        
        // Display results for this frequency
        if (msg_count > 0) {
            Serial.printf(" Received: %2d messages, RSSI: max=%.1f dBm, avg=%.1f dBm\n",
                          msg_count, max_rssi, scanResults[currentResult].avg_rssi);
        } else {
            Serial.println(" No messages received");
        }
        
        currentResult++;
        scanResultCount = currentResult;
        
        // Put radio to sleep before reinitializing
        ws.sleep();
        delay(100);
    }
    
    Serial.println();
}

void analyzeResults() {
    Serial.println("========================================");
    Serial.println("Scan Results Summary");
    Serial.println("========================================");
    Serial.println();
    
    // Find best frequency based on message count and RSSI
    int best_idx = -1;
    int max_messages = 0;
    float best_rssi = -200.0;
    
    Serial.println("Offset (kHz) | Frequency (MHz) | Messages | Max RSSI (dBm) | Avg RSSI (dBm)");
    Serial.println("-------------|-----------------|----------|----------------|----------------");
    
    for (int i = 0; i < scanResultCount; i++) {
        Serial.printf("%+7d      | %11.3f     | %8d | %14.1f | %14.1f\n",
                      (int)(scanResults[i].offset_mhz * 1000),
                      868.3 + scanResults[i].offset_mhz,
                      scanResults[i].message_count,
                      scanResults[i].max_rssi,
                      scanResults[i].avg_rssi);
        
        // Determine best frequency
        // Prioritize message count, then RSSI
        if (scanResults[i].message_count > max_messages) {
            max_messages = scanResults[i].message_count;
            best_rssi = scanResults[i].avg_rssi;
            best_idx = i;
        } else if (scanResults[i].message_count == max_messages && 
                   scanResults[i].avg_rssi > best_rssi) {
            best_rssi = scanResults[i].avg_rssi;
            best_idx = i;
        }
    }
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Recommendation");
    Serial.println("========================================");
    Serial.println();
    
    if (best_idx >= 0 && max_messages > 0) {
        double best_offset = scanResults[best_idx].offset_mhz;
        Serial.println("Based on the scan results:");
        Serial.printf("  Optimal frequency offset: %.3f MHz\n", best_offset);
        Serial.printf("  Optimal frequency: %.3f MHz\n", 868.3 + best_offset);
        Serial.printf("  Messages received: %d\n", scanResults[best_idx].message_count);
        Serial.printf("  Average RSSI: %.1f dBm\n", scanResults[best_idx].avg_rssi);
        Serial.println();
        Serial.println("To use this frequency offset in your sketch:");
        Serial.printf("  ws.begin(1, true, %.3f);\n", best_offset);
        Serial.println();
        
        // Check if offset is significant
        if (abs(best_offset) < 0.025) {  // Less than 25 kHz
            Serial.println("Note: The optimal offset is very close to zero.");
            Serial.println("Your transceiver is already well calibrated.");
        } else if (abs(best_offset) > 0.150) {  // More than 150 kHz
            Serial.println("Warning: The optimal offset is quite large.");
            Serial.println("This may indicate a hardware issue with your transceiver module.");
        }
    } else {
        Serial.println("WARNING: No messages were received during the scan!");
        Serial.println();
        Serial.println("Possible issues:");
        Serial.println("  1. Weather sensor is not transmitting");
        Serial.println("  2. Weather sensor is out of range");
        Serial.println("  3. Wrong pin configuration for the radio module");
        Serial.println("  4. Hardware issue with the radio module");
        Serial.println();
        Serial.println("Please verify your setup and try again.");
    }
    
    Serial.println();
}

void loop() {
    // Scan is complete, nothing to do in loop
    delay(10000);
    Serial.println("Scan complete. Reset device to run another scan.");
}
