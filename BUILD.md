# SW Build

Tested with Arduino 1.8.19 and an empty Arduino library directory.

1. Check that the required Board Manager URL is in the Preferences (e.g. the one for ESP8266):

![00-board-manager](https://user-images.githubusercontent.com/83612361/196784057-5d0a4494-f6d2-4723-881b-0e1de74a7fd9.png)

2. Select  **BresserWeatherSensorReceiver** in the Library Manager - **please use the latest version**:
 
![01-library-manager](https://user-images.githubusercontent.com/83612361/196784440-13c60b00-e336-4259-876c-a1025bee7ce8.png)

3. The Library Manager will ask if to install the **library dependencies** - select **"Install all"**:

![2-dependencies](https://user-images.githubusercontent.com/83612361/196784660-dbf38a22-d156-4444-a3bf-affdb2d06ac9.png)

4. From the **File** Menu, select **Examples->BresserWeatherSensorReceiver->BresserWeatherSensorMQTT**:

![3-Examples-BresserWeatherSensorReceiver-BresserWeatherSensorMQTT](https://user-images.githubusercontent.com/83612361/196785302-3c46eed5-3656-46cc-a426-017197f67089.png)

5. Select your board:

![04-board](https://user-images.githubusercontent.com/83612361/196785701-72692d15-167a-41f4-a586-59c38fa6e049.png)

6. Modify `BresserWeatherSensorCfg.h` as required, typically:
```
// Number of sensors to be received
#define NUM_SENSORS 1
```
```
// Select type of receiver module
#define USE_CC1101
//#define USE_SX1276
```
Please use **either** one of the pre-defined pinnings:
```
// Use pinning for LoRaWAN Node 
#define LORAWAN_NODE

// Use pinning for TTGO ESP32 boards with integrated RF tranceiver (SX1276)
// https://github.com/espressif/arduino-esp32/tree/master/variants/ttgo-lora32-*
//#define TTGO_LORA32

// Use pinning for Adafruit Feather ESP32S2 with RFM95W "FeatherWing" ADA3232
//#define ADAFRUIT_FEATHER_ESP32S2
```
**or** check that your pinning matches the default for the selected MCU:
```
[...]
#elif defined(ESP32)
    #define PIN_RECEIVER_CS   27
    
    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  21 
    
    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 33
    
    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  32

#elif defined(ESP8266)
    #define PIN_RECEIVER_CS   15
    
    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  4 
    
    // CC1101: GDO2 / RFM95W/SX127x: 
    #define PIN_RECEIVER_GPIO 5 
    
    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  2
#endif
``` 
7. Edit the mandatory settings in `BresserWeatherSensorMQTT.ino`:
```
#define USE_SECUREWIFI          // use secure WIFI
//#define USE_WIFI              // use non-secure WIFI
...
// Map sensor IDs to Names - the number of entries must match NUM_SENSORS
SensorMap sensor_map[NUM_SENSORS] = {
    {0x39582376, "WeatherSensor"}
};
...
#ifndef SECRETS
    const char ssid[] = "WiFiSSID";
    const char pass[] = "WiFiPassword";
    ...

    #define    MQTT_PORT     8883
    const char MQTT_HOST[] = "xxx.yyy.zzz.com";
    const char MQTT_USER[] = ""; // leave blank if no credentials used
    const char MQTT_PASS[] = ""; // leave blank if no credentials used
...
```

- Set the WiFi settings (ssid/pass) according to your Access Point
- Set `USE_SECUREWIFI`/`USE_WIFI`, `MQTT_PORT`, `MQTT_HOST`, `MQTT_USER` and `MQTT_PASS` according to your MQTT broker's IP address and configuration
   - for a non-secure setup: `USE_WIFI` and typically `MQTT_PORT` 1883
   - for a secure-setup: `USE_SECUREWIFI` and typically `MQTT_PORT` 8883 (and other security measures as needed)
   - set `MQTT_USER` and `MQTT_PASS` as required by the MQTT broker
 - SensorMap is just for your convenience - if you do not know your sensor's ID yet, leave it as it is. Just change the number of entries to match `NUM_SENSORS`.

You can of course copy the **secrets** to `secrets.h` and make your changes there instead of modifying the template in the sketch. In this case, do not forget to add the following in `secrets.h`:
```
#define SECRETS
```
**Note:** The define `SECRET`has been renamed to `SECRETS`for consistency.

8. Now you want to save your changes. You will be asked to select a new directory, because the example resides in the Arduino/libraries folder which is treated as read-only:

![5-modified-sketch](https://user-images.githubusercontent.com/83612361/196790621-31fbb28a-2d49-4e38-be3e-75ed513332bc.png)

9. Finally you should be able to compile your sketch without any errors or warnings:

![06-compiler-result](https://user-images.githubusercontent.com/83612361/196790812-d0ab3066-d82e-495e-a15f-0dfb2334a393.png)
