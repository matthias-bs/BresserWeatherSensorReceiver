# Debug Output Configuration in Arduino IDE

## ESP32

1. Select appropriate (USB-)serial port for your board

  <img src="https://user-images.githubusercontent.com/83612361/210953111-3381fcb4-0d39-43f5-bcfe-5d381bc04c00.png" alt="ESP32 Port Selection" width="500">

2. Select desired debug level
   
  <img src="https://user-images.githubusercontent.com/83612361/210953437-b35a0bb3-df71-4292-ad0b-9e50846d6cc8.png" alt="ESP32 Core Debug Level" width="500">

  This passes the define `CORE_DEBUG_LEVEL`to the compiler accordingly.

Refer to the following for some background information
* https://thingpulse.com/esp32-logging/
* https://www.mischianti.org/2020/09/20/esp32-manage-multiple-serial-and-logging-for-debugging-3/
* https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h

## ESP8266

1. Select appropriate (USB-)serial port for your board

  <img src="https://user-images.githubusercontent.com/83612361/210953111-3381fcb4-0d39-43f5-bcfe-5d381bc04c00.png" alt="ESP8266 Port Selection" width="500">
  
2. Select the appropriate Debug Port (MCU serial interface) of your board
  
  <img src="https://user-images.githubusercontent.com/83612361/210956029-e185a233-ad06-4457-b5a0-655db2177986.png" alt="ESP8266 Debug Port Selection" width="500">

3. If needed, change the debug level in `WeatherSensorCfg.h`
```
 #define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE
```

(Options: `ARDUHAL_LOG_LEVEL_<NONE|ERROR|WARN|INFO|DEBUG|VERBOSE>`)
