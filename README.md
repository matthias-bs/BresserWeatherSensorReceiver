# BresserWeatherSensorLib
Bresser 5-in-1/6-in-1 868 MHz Weather Sensor Radio Receiver for Arduino based on CC1101 or SX1276/RFM95W

The Bresser 5-in-1 Weather Stations seem to use two different protocols. Select the appropriate decoder by (un-)commenting `#define BRESSER_5_IN_1` or `#define BRESSER_6_IN_1` in the source code.

| Model         | Decoder Function                |
| ------------- | ------------------------------- |
| 7002510..12   | decodeBresser**5In1**Payload()  |
| 7902510..12   | decodeBresser**5In1**Payload()  |
| 7002585       | decodeBresser**6In1**Payload()  |
