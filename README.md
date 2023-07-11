# BresserWeatherSensorReceiver
[![CI](https://github.com/matthias-bs/BresserWeatherSensorReceiver/actions/workflows/CI.yml/badge.svg)](https://github.com/matthias-bs/BresserWeatherSensorReceiver/actions/workflows/CI.yml)<!--[![Build Status](https://app.travis-ci.com/matthias-bs/BresserWeatherSensorReceiver.svg?branch=main)](https://app.travis-ci.com/matthias-bs/BresserWeatherSensorReceiver)-->
[![CppUTest](https://github.com/matthias-bs/BresserWeatherSensorReceiver/actions/workflows/CppUTest.yml/badge.svg)](https://github.com/matthias-bs/BresserWeatherSensorReceiver/actions/workflows/CppUTest.yml)
[![GitHub release](https://img.shields.io/github/release/matthias-bs/BresserWeatherSensorReceiver?maxAge=3600)](https://github.com/matthias-bs/BresserWeatherSensorReceiver/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/LICENSE)
[![arduino-library-badge](https://www.ardu-badge.com/badge/BresserWeatherSensorReceiver.svg?)](https://www.ardu-badge.com/BresserWeatherSensorReceiver)

Bresser 5-in-1/6-in-1/7-in-1 868 MHz Weather Sensor Radio Receiver for Arduino based on CC1101 or SX1276/RFM95W

To allow automatic handling of all Bresser weather station variants, the decoders are tried in the following order until decoding succeeded:
1. 7-in-1-Decoder 
2. 6-in-1-Decoder
3. 5-in-1 Decoder
4. Lightning Sensor Decoder (experimental)

(The Bresser 5-in-1 Weather Stations seem to use two different protocols - 5-in-1 and 6-in-1.)

| Model         | Type | Decoder Function                |
| ------------- | ---- | ------------------------------- |
| 7002510..12, 9602510   | Weather Station | decodeBresser**5In1**Payload()  |
| 7902510..12   | Weather Station (Base) | decodeBresser**5In1**Payload()  |
| *7002531*       | *3-in-1 Professional Wind Gauge / Anemometer* | *decodeBresser**6In1**Payload()* **1)** |
| 7002585       | Weather Station | decodeBresser**6In1**Payload()  |
| 7009999       | Thermo-/Hygrometer Sensor | decodeBresser**6in1**Payload() |
| 7009972       | Soil Moisture/Temperature Sensor | decodeBresser**6In1**Payload() |
| 7009976       | Lightning Sensor | decodeBresser**Lightning**Payload() |
| 7003600 and WSX3001 | Weather Station | decodeBresser**7In1**Payload() **2)** |
| 7803200       | Weather Sensor  | decodeBresser**7In1**Payload()  |
| 7003300       | Weather Station | decodeBresser**7In1**Payload()  |
| 7803300       | Weather Sensor  | decodeBresser**7In1**Payload()  |

Some guesswork:

| Numbering Scheme | Type |
| ---------------- | ---- |
| 700[25\|32\|33\|36]*   | Weather Station, Base + Sensor |
| 780[25\|32\|33]*   | Weather Station Sensor (Replacement) |
| 790*             | Weather Station Base (Replacement) |
| 700[99]*         | Accessory Sensor |

**1)** Manual configuration required, UV flag is set erroneously; see https://github.com/matthias-bs/BresserWeatherSensorReceiver/issues/42

**2)** The part number is specific to the actual variant, i.e. some more characters are appended

## Configuration

### Configuration by selecting a supported Board in the Arduino IDE

By selecting a Board and a Board Revision in the Arduino IDE, a define is passed to the preprocessor/compiler. For the boards in the table below, the default configuration is assumed based on this define. I.e. you could could use an Adafruit Feather ESP32-S2 with a CC1101 connected to the pins of your choice of course, but the code assumes you are using it with a LoRa Radio Featherwing with the wiring given below.

If you are not using the Arduino IDE, you can use the defines in the table below with your specific tool chain to get the same result.

If this is not what you need, you have to switch to **Manual Configuration**

   | Setup                                                          | Board              | Board Revision               | Define                 | Radio Module | Notes    |
   | -------------------------------------------------------------- | ------------------ | ---------------------------- | ---------------------- | -------- | ------- |
   | [LILYGO®TTGO-LORA32 V1](https://github.com/Xinyuan-LilyGo/TTGO-LoRa-Series) | "TTGO LoRa32-OLED" | "TTGO LoRa32 V1 (No TFCard)" | ARDUINO_TTGO_LORA32_V1 | SX1276 (HPD13A) | -   |
   | [LILYGO®TTGO-LORA32 V2](https://github.com/LilyGO/TTGO-LORA32) | "TTGO LoRa32-OLED" | "TTGO LoRa32 V2"             | ARDUINO_TTGO_LoRa32_V2 | SX1276 (HPD13A) | Wire DIO1 to GPIO33 |
   | [LILYGO®TTGO-LORA32 V2.1](http://www.lilygo.cn/prod_view.aspx?TypeId=50060&Id=1271&FId=t3:50060:3) | "TTGO LoRa32-OLED" | "TTGO LoRa32 V2.1 (1.6.1)" | ARDUINO_TTGO_LoRa32_v21new |  SX1276 (HPD13A) | - |
   | [Heltec Wireless Stick](https://heltec.org/project/wireless-stick/) | "Heltec Wireless Stick"   | n.a.             | ARDUINO_heltec_wireless_stick  | SX1276 | - |
   | [Heltec WiFi LoRa 32 V2](https://heltec.org/project/wifi-lora-32/)  | "Heltec WiFi LoRa 32(V2)" | n.a.             | ARDUINO_heltec_wifi_lora_32_V2 | SX1276 | - |
   | [Adafruit Feather ESP32S2 with Adafruit LoRa Radio FeatherWing](https://github.com/matthias-bs/BresserWeatherSensorReceiver#adafruit-feather-esp32s2-with-adafruit-lora-radio-featherwing)                                | "Adafruit Feather ESP32-S2" | n.a.               | ARDUINO_ADAFRUIT_FEATHER_ESP32S2   | SX1276 (RFM95W) | Wiring on the Featherwing:<br>E to IRQ<br>D to CS<br>C to RST<br>A to DI01 |
   | [Adafruit Feather ESP32 or ThingPulse ePulse Feather with Adafruit LoRa Radio FeatherWing](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/README.md#adafruit-feather-esp32-or-thingpulse-epulse-feather-with-adafruit-lora-radio-featherwing)                                | "Adafruit ESP32 Feather" | n.a.               | ARDUINO_FEATHER_ESP32   | SX1276 (RFM95W) | Wiring on the Featherwing:<br>A to RST<br>B to DIO1<br>D to IRQ<br>E to CS |
 | [DFRobot FireBeetle with FireBeetle Cover LoRa Radio 868MHz](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/README.md#dfrobot-firebeetle-esp32-with-firebeetle-cover-lora-radio-868mhz)                               | "FireBeetle-ESP32" | n.a.               | ARDUINO_ESP32_DEV & **FIREBEETLE_ESP32_COVER_LORA**<sup>1</sup>   | SX1276 (LoRa1276) | Wiring on the cover: <br>D2 to RESET<br>D3 to DIO0<br>D4 to CS<br>D5 to DIO1 |
 | [Adafruit Feather 32u4 RFM95 LoRa Radio](https://www.adafruit.com/product/3078)      | "Adafruit Feather 32u4" | n.a.                       | ARDUINO_AVR_FEATHER32U4     | SX1276 (RFM95W) | see [notes](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/README.md#adafruit-feather-32u4-rfm95-lora-radio)       |

<sup>1</sup> Must be set manually in `WeatherSensorCfg.h

The preprocessor will provide some output regarding the selected configuration if enabled in the Arduino IDE's Preferences ("Verbose Output"), e.g. 
```
ARDUINO_ADAFRUIT_FEATHER_ESP32S2 defined; assuming RFM95W FeatherWing will be used
[...]
Receiver chip: [SX1276]
Pin config: RST->0 , CS->6 , GD0/G0/IRQ->5 , GDO2/G1/GPIO->11
```

### Manual Configuration

See `WeatherSensorCfg.h` for configuration options.

* Set the desired radio module by (un-)commenting `USE_CC1101` or `USE_SX1276`.

  SX1276 is compatible with RFM95W and HPD13A.

* Set the I/O pinning according to your hardware

   | Define                     | Radio Module    | Configuration                                                    |
   | -------------------------- | --------------- | ---------------------------------------------------------------- |
   | ESP32                      | user-defined    | generic, used for ESP32 boards if none of the above is defined   |
   | ESP8266                    | user-defined    | generic, used for ESP8266 boards if none of the above is defined |

* Data from multiple sensors can be received by setting `NUM_SENSORS` to an appropriate value in `WeatherSensorCfg.h`.

   e.g. `#define NUM_SENSORS 1`

* The sensors to be handled can be configured by two ways:
   * Add any unwanted sensor IDs to the exclude list `SENSOR_IDS_EXC`

     e.g. `#define SENSOR_IDS_EXC { 0x39582376 }`
  
   * Specify the wanted sensors explicitly in the include list `SENSOR_IDS_EXC` - if empty, all sensors will be used

     e.g. `#define SENSOR_IDS_INC { 0x83750871 }`

## SW Examples

### [BresserWeatherSensorBasic](https://github.com/matthias-bs/BresserWeatherSensorReceiver/tree/main/examples/BresserWeatherSensorBasic)

Uses default configuration [src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/src/WeatherSensorCfg.h)

Really a very basic example. Good for testing the SW build, wiring and sensor reception/decoding. Output is printed to the serial console ([example](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorBasic/example.log)).
Data is provided by the `getMessage()`-method, which returns almost immediately (i.e. after a small multiple of expected time-on-air), even if no data has been received.

### [BresserWeatherSensorWaiting](https://github.com/matthias-bs/BresserWeatherSensorReceiver/tree/main/examples/BresserWeatherSensorWaiting)

Uses default configuration [src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/src/WeatherSensorCfg.h)

Very similar to [BresserWeatherSensorBasic](https://github.com/matthias-bs/BresserWeatherSensorReceiver/tree/main/examples/BresserWeatherSensorBasic), but data is provided by the `getData()`-method, which waits until a complete set of data has been received or a timeout occurred. Output is printed to the serial console ([example](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorWaiting/example.log)).

### [BresserWeatherSensorCallback](https://github.com/matthias-bs/BresserWeatherSensorReceiver/tree/main/examples/BresserWeatherSensorCallback)

Uses default configuration [src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/src/WeatherSensorCfg.h)

Based on [BresserWeatherSensorWaiting](https://github.com/matthias-bs/BresserWeatherSensorReceiver/tree/main/examples/BresserWeatherSensorWaiting), but repeatedly invokes a callback function while waiting for data. In this example, in each iteration of the wait-loop, a dot is printed. Output is printed to the serial console ([example](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorCallback/example.log)).

### [BresserWeatherSensorOptions](https://github.com/matthias-bs/BresserWeatherSensorReceiver/tree/main/examples/BresserWeatherSensorOptions)

Uses default configuration [src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/src/WeatherSensorCfg.h)

Based on [BresserWeatherSensorWaiting](https://github.com/matthias-bs/BresserWeatherSensorReceiver/tree/main/examples/BresserWeatherSensorWaiting), but demonstrates the different options of the `getData()`-method which defined if enough sensor data has been received before returning. Output is printed to the serial console ([example](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorOptions/example.log)).


### [BresserWeatherSensorMQTT](https://github.com/matthias-bs/BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTT)

Uses default configuration [src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/src/WeatherSensorCfg.h)

This is finally a useful application.

At startup, first a WiFi connection and then a connection to the MQTT broker is established. (Edit `secrets.h` accordingly!) Then receiving data of all sensors (as defined in NUM_SENSORS, see WeatherSensorCfg.h) is tried periodically. If successful, sensor data is published as MQTT messages, one message per sensor.
If the sensor ID can be mapped to a name (edit `sensor_map[]`), this name is used as the MQTT topic, otherwise the ID is used. From the sensor data, some additional data is calculated and published with the _extra_ topic.

The data topics are published at an interval of >`DATA_INTERVAL`. The _status_ and the _radio_ topics are published at an interval of `STATUS_INTERVAL`.

If sleep mode is enabled (`SLEEP_EN`), the device goes into deep sleep mode after data has been published. If `AWAKE_TIMEOUT` is reached before data has been published, deep sleep is entered, too. After `SLEEP_INTERVAL`, the controller is restarted. 

MQTT publications:

`<base_topic>/data/<ID|name>`    sensor data as JSON string - see `publishWeatherdata()`
     
`<base_topic>/radio`             CC1101 radio transceiver info as JSON string - see `publishRadio()`
     
`<base_topic>/status`            "online"|"offline"|"dead"$

$ via LWT

`<base_topic>` is set by `#define HOSTNAME ...`

`<base_topic>/data` JSON Example:
```
{"sensor_id":12345678,"ch":0,"battery_ok":true,"humidity":44,"wind_gust":1.2,"wind_avg":1.2,"wind_dir":150,"rain":146}
```

**Dashboard with [IoT MQTT Panel](https://snrlab.in/iot/iot-mqtt-panel-user-guide) (Example)**

<img src="https://user-images.githubusercontent.com/83612361/158457786-516467f9-2eec-4726-a9bd-36e9dc9eec5c.png" alt="IoTMQTTPanel_Bresser_5-in-1" width="400">

### [BresserWeatherSensorMQTTCustom](https://github.com/matthias-bs/BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTTCustom)

Customized version of the example [BresserWeatherSensorMQTT](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorMQTT/BresserWeatherSensorMQTT.ino)

The file [BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTTCustom/src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorMQTTCustom/src/WeatherSensorCfg.h) has been customized 
(from [BresserWeatherSensorReceiver/src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/src/WeatherSensorCfg.h)).

See [examples/BresserWeatherSensorMQTTCustom/Readme.md](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorMQTTCustom/Readme.md) for details.

### [BresserWeatherSensorMQTTWiFiMgr](https://github.com/matthias-bs/BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTTWiFiMgr)

Same core functionality as [BresserWeatherSensorMQTT](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorMQTT/BresserWeatherSensorMQTT.ino), but instead of using static WiFi- and MQTT-connection data, [WiFiManager](https://github.com/tzapu/WiFiManager) is used instead.

**Note:**

When using the sketch on a device for the first time, you must format the flash file system (SPIFFS) first, otherwise the configuration cannot be saved.

**Configuration:**

* Access Point SSID: ESPWeather-<chip_id>
* Access Point Password: password
* Configuration URL: http://192.168.4.1/ (The browser must be connected to the access point above!)

Please refer to the [WiFiManager](https://github.com/tzapu/WiFiManager) documentation for details!

After a successful setup, you can perform two consecutive resets (within 10 seconds) to enable WiFiManager for changing the configuration. This is achieved by using [ESP_DoubleResetDetector](https://github.com/khoih-prog/ESP_DoubleResetDetector).

<img src="https://github.com/matthias-bs/BresserWeatherSensorReceiver/assets/83612361/86a3f629-276d-48ac-8eff-acda051e7a2b" alt="WiFiManager Start Screen" width="300">
<br>
<img src="https://github.com/matthias-bs/BresserWeatherSensorReceiver/assets/83612361/a1055ec5-dcc0-44ac-89fc-6a18497cce6e" alt="WiFiManager Configuration Screen" width="300"> 


### [BresserWeatherSensorDomoticz](https://github.com/matthias-bs/BresserWeatherSensorReceiver/examples/BresserWeatherSensorDomoticz)

Based on [BresserWeatherSensorMQTT](https://github.com/matthias-bs/BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTT). Provides sensor data as MQTT messages via WiFi to Domoticz (https://domoticz.com/) (MQTT plugin for Domoticz required). The MQTT topics are designed for using with Domoticz virtual sensors (see https://www.domoticz.com/wiki/Managing_Devices#Temperature and https://www.domoticz.com/wiki/Managing_Devices#Weather).

# Debug Output Configuration

See [Debug Output Configuration in Arduino IDE](DEBUG_OUTPUT.md)

# HW Examples

**Note:**
The SX1276/RFM95W also supports FSK modulation and thus can be used to receive the weather sensor data.

## ESP8266 D1 Mini with CC1101

<img src="https://user-images.githubusercontent.com/83612361/158458191-b5cabad3-3515-45d0-98e3-94b0fa13b8ef.jpg" alt="Bresser5in1_CC1101_D1-Mini" width="400">

[Pinout ESP8266 WeMos D1-Mini with cc1101](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/ESP8266_WeMos_D1-Mini_pinout_cc1101_v2.pdf)

### CC1101

[Texas Instruments CC1101 Product Page](https://www.ti.com/product/CC1101)

**Note: CC1101 Module Connector Pitch is 2.0mm!!!**

Unlike most modules/breakout boards, most (if not all) CC1101 modules sold on common e-commerce platforms have a pitch (distance between pins) of 2.0mm. To connect it to breadboards or jumper wires with 2.54mm/100mil pitch (standard), the following options exist:

* solder wires directly to the module
* use a 2.0mm pin header and make/buy jumper wires with 2.54mm at one end and 2.0mm at the other (e.g. [Adafruit Female-Female 2.54 to 2.0mm Jumper Wires](https://www.adafruit.com/product/1919))
* use a [2.0mm to 2.54 adapter PCB](https://www.amazon.de/Lazmin-1-27MM-2-54MM-Adapter-Platten-Brett-drahtlose-default/dp/B07V873N52)

**Note 2: Make sure to use the 868MHz version!**

## Adafruit Feather ESP32S2 with Adafruit LoRa Radio FeatherWing

**Note: Make sure to use the 868MHz version!**
* [ADA3231](https://www.adafruit.com/product/3231) - Adafruit LoRa Radio FeatherWing - RFM95W 900 MHz - RadioFruit
* [ADA3232](https://www.adafruit.com/product/3232) - Adafruit LoRa Radio FeatherWing - RFM95W 433 MHz - RadioFruit
* [ADA5303](https://www.adafruit.com/product/5303) - Adafruit ESP32-S2 Feather with BME280 Sensor - STEMMA QT - 4MB Flash + 2 MB PSRAM

Solder-Bridges on the Module/Wing:
* E to IRQ
* D to CS
* C to RST
* A to DI01

## Adafruit Feather ESP32 or ThingPulse ePulse Feather with Adafruit LoRa Radio FeatherWing
![ePulse_Feather+FeatherWing](https://user-images.githubusercontent.com/83612361/228173195-f13fe4c4-7f86-4e73-b485-d5d553034554.jpg)

**Note: Make sure to use the 868MHz version!**
* [ADA3231](https://www.adafruit.com/product/3231) - Adafruit LoRa Radio FeatherWing - RFM95W 900 MHz - RadioFruit
* [ADA3232](https://www.adafruit.com/product/3232) - Adafruit LoRa Radio FeatherWing - RFM95W 433 MHz - RadioFruit
* [ADA3405](https://www.adafruit.com/product/3405) - Adafruit HUZZAH32 – ESP32 Feather Board
* [B0BSC1PVL4](https://thingpulse.com/product/epulse-feather-low-power-esp32-development-board/) - ThingPulse ePulse Feather

Solder-Bridges on the Module/Wing:
* A to RST
* B to DIO1 
* D to IRQ
* E to CS

## Adafruit RFM95W LoRa Radio Transceiver Breakout

**Note: Make sure to use the 868MHz version!**
* [ADA3072](https://www.adafruit.com/product/3072) - 868/915 MHz version
* [ADA3073](https://www.adafruit.com/product/3073) - 433 MHz version
* RF connector
* Antenna

See [Adafruit RFM69HCW and RFM9X LoRa Packet Radio Breakouts - Pinouts](https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/pinouts).


## DFRobot FireBeetle ESP32 with FireBeetle Cover LoRa Radio 868MHz
![firebeetle_esp32+cover_lora](https://user-images.githubusercontent.com/83612361/233463592-e99a9d1c-5100-4ac2-9b33-bcfc974406f0.jpg)
Note: Stacking headers were included with TEL0125.

**Note: Make sure to use the 868MHz version!**
* [DFR0478](https://www.dfrobot.com/product-1590.html) - FireBeetle ESP32 IoT Microcontroller
* [TEL0125](https://www.dfrobot.com/product-1831.html) - LoRa Radio 868MHz - FireBeetle Covers


Solder-Bridges on the Cover:
* D2 to RESET
* D3 to DIO0
* D4 to CS
* D5 to DIO1

## Adafruit Feather 32u4 RFM95 LoRa Radio

**Note: Make sure to use the 868MHz version!**

**Note: Not recommended for new projects!**

* [ADA3078](https://www.adafruit.com/product/3078) - 868/915 MHz version
* Antenna

Some restrictions apply:
* Based on AVR ATmega32u4 clocked at 8 MHz
* No WiFi
* Basic SW example ([examples/BresserWeatherSensorFeather32u4](https://github.com/matthias-bs/BresserWeatherSensorReceiver/tree/main/examples/BresserWeatherSensorFeather32u4)) already uses 92% of available program memory (see [BUILD_AVR.md](BUILD_AVR.md) for additional hints)
* Only a single pad for antenna
* `WeatherUtils.h/.cpp`: `winddir_flt_to_str()` not implemented

## Antennas and RF Connectors

The required antenna depends on the signal path between weather sensor and CC1101 or RFM95W receiver. 

Some options are:
* wire antenna
* spring antenna (helical wire coil)
* rubber antenna

See [Adafruit Tutorial - Antenna Options](https://learn.adafruit.com/adafruit-feather-32u4-radio-with-lora-radio-module/antenna-options) for wire antenna lengths and uFL connector soldering.

The [Data Alliance](https://www.data-alliance.net/mhf-series-mhf1-mhf2-mhf3-mhf4/) website helped to sort out my RF connector confusion:

> Applications of MHF Connectors & Cables
>
> The MHF series of RF micro-connectors (mating heights listed below are the maximum):
> * MHF1 (also known as MHF) has a Mating Height of 2.5mm
> * MHF2 has a Mating Height of 2.1mm
> * MHF3 has a Mating Height of 1.6mm
> * MHF4 has a Mating Height of 1.2mm
>
> MHF3 connector is compatible with a W.FL connector while MHF2 connector is equivalent of U.FL connector. The MHF4 cable connector is the smallest while MHF1 connector is the largest which is comparable to a U.FL connector.

Personally I prefer the SMA connector over the uFL connector -  but be aware of the (usual) male/female connector types and the normal/reverse polarity types. See [SMA vs RP-SMA what is the difference?](https://forum.digikey.com/t/sma-vs-rp-sma-what-is-the-difference/550) by Digikey.

## Software Build Tutorial

See [BUILD](BUILD.md)


## Source Documentation

https://matthias-bs.github.io/BresserWeatherSensorReceiver/

# Legal

> This project is in no way affiliated with, authorized, maintained, sponsored or endorsed by Bresser GmbH or any of its affiliates or subsidiaries.
