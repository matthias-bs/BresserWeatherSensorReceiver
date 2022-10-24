# BresserWeatherSensorMQTTCustom
Customized version of the example [BresserWeatherSensorMQTT](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorMQTT/BresserWeatherSensorMQTT.ino)

The file [BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTTCustom/src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorMQTTCustom/src/WeatherSensorCfg.h) has been customized 
(from [BresserWeatherSensorReceiver/src/WeatherSensorCfg.h](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/src/WeatherSensorCfg.h).
   - board definition (`//#define LORAWAN_NODE`)
   - type of receiver module (changed from `USE_SX1276` to `USE_CC1101`)
 
All other files in `BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTTCustom/src/` have been copied from `BresserWeatherSensorReceiver/src`.

Finally, including the header files from<br> 
`BresserWeatherSensorReceiver/src/` has been changed to<br>
`BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTTCustom/src/` <br>
in [BresserWeatherSensorMQTTCustom.ino](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorMQTTCustom/BresserWeatherSensorMQTTCustom.ino):

```
#include "src/WeatherSensorCfg.h"
#include "src/WeatherSensor.h"
#include "src/WeatherUtils.h"
#include "src/RainGauge.h"
```
