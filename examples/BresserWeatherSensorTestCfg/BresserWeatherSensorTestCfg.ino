///////////////////////////////////////////////////////////////////////////////////////////////////
// BresserWeatherSensorTestCfg.ino
//
// Test for sensor configuration setup and retrieval from 
// BresserWeatherSensorCfg.h and Preferences.
// (quick and dirty)
//
//
// https://github.com/matthias-bs/BresserWeatherSensorReceiver
//
//
// created: 04/2024
//
//
// MIT License
//
// Copyright (c) 2024 Matthias Prinke
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
// 20240417 Created
// 20240504 Added board initialization
//
// ToDo: 
// - 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "InitBoard.h"


WeatherSensor ws;

void printBuf(uint8_t *buf, uint8_t size) {
        for (size_t i=0; i < size; i+=4) {
        Serial.printf("0x%08X\n", 
            (buf[i] << 24) |
            (buf[i+1] << 16) |
            (buf[i+2] << 8) |
            buf[i+3]
        );
    }
}


void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    initBoard();

    Serial.printf("Starting execution...\n");

    ws.begin();
    
    Preferences cfgPrefs;
    cfgPrefs.begin("BWS-CFG", false);
    cfgPrefs.clear();
    cfgPrefs.end();
    cfgPrefs.begin("BWS-CFG", false);
    uint8_t buf[48];
    uint8_t size;
    size = ws.getSensorsInc(buf);
    printBuf(buf, size);
    
    uint8_t id1[] = {0xDE, 0xAD, 0xBE, 0xEF};
    ws.setSensorsInc(id1, 4);
    size = ws.getSensorsInc(buf);
    printBuf(buf, size);

    size = ws.getSensorsExc(buf);
    printBuf(buf, size);
    
    uint8_t id2[] = {0xC0, 0xFF, 0xEE, 0x11};
    ws.setSensorsExc(id2, 4);
    size = ws.getSensorsExc(buf);
    printBuf(buf, size);
    cfgPrefs.clear();
    cfgPrefs.end();

    ws.begin();
}


void loop() 
{   
    delay(100);
} // loop()
