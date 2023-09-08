# Debug Output Configuration in Arduino IDE

## ESP32

1. Select appropriate (USB-)serial port for your board
   
    ![Arduino_IDE-Tools_Port](https://github.com/matthias-bs/BresserWeatherSensorTTN/assets/83612361/be496bf8-89ce-4db5-b1bf-c88a7f5e99cb)

**or**

   ![Arduino_IDE-Select_Other_Board_and_Port](https://github.com/matthias-bs/BresserWeatherSensorTTN/assets/83612361/ac847f23-4fe6-4111-929f-ac6d36cb8a53)
  
2. Select desired debug level

    ![Arduino_IDE-Tools_CoreDebugLevel](https://github.com/matthias-bs/BresserWeatherSensorTTN/assets/83612361/72a8b1d9-8d39-41fc-9658-78b432b73d56)

  This passes the define `CORE_DEBUG_LEVEL`to the compiler accordingly.

Refer to the following for some background information
* https://thingpulse.com/esp32-logging/
* https://www.mischianti.org/2020/09/20/esp32-manage-multiple-serial-and-logging-for-debugging-3/
* https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h


## ESP8266

1. Select appropriate (USB-)serial port for your board

    ![Arduino_IDE-Tools_Port_ESP8266](https://github.com/matthias-bs/BresserWeatherSensorReceiver/assets/83612361/dafbdd33-244f-44b3-b3f4-da854633f634)

2. Select the appropriate Debug Port (MCU serial interface) of your board
  
    ![Arduino_IDE-Tools_Debug_Port_ESP8266](https://github.com/matthias-bs/BresserWeatherSensorReceiver/assets/83612361/7d7fdc96-2abd-4f55-9203-595f08eb7f06)

3. If needed, change the debug level in `WeatherSensorCfg.h`
```
 #define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE
```

    (Options: `ARDUHAL_LOG_LEVEL_<NONE|ERROR|WARN|INFO|DEBUG|VERBOSE>`)
