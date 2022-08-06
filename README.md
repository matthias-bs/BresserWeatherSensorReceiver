# BresserWeatherSensorReceiver
Bresser 5-in-1/6-in-1 868 MHz Weather Sensor Radio Receiver for Arduino based on CC1101 or SX1276/RFM95W

The Bresser 5-in-1 Weather Stations seem to use two different protocols. Select the appropriate decoder by (un-)commenting `#define BRESSER_5_IN_1` or `#define BRESSER_6_IN_1` in `WeatherSensor.h`.

| Model         | Decoder Function                |
| ------------- | ------------------------------- |
| 7002510..12   | decodeBresser**5In1**Payload()  |
| 7902510..12   | decodeBresser**5In1**Payload()  |
| 7002585       | decodeBresser**6In1**Payload()  |

Configure the desired radio module by (un-)commenting `USE_CC1101` or `USE_SX1276` in `WeatherSensor.h`.

## Examples
### [BresserWeatherSensorMQTT](https://github.com/matthias-bs/BresserWeatherSensorReceiver/examples/BresserWeatherSensorMQTT)

MQTT publications:

`<base_topic>/data`    sensor data as JSON string - see `publishWeatherdata()`
     
`<base_topic>/radio`   CC1101 radio transceiver info as JSON string - see `publishRadio()`
     
`<base_topic>/status`  "online"|"offline"|"dead"$

$ via LWT

`<base_topic>` is set by `#define HOSTNAME ...`

`<base_topic>/data` JSON Example:
```
{"sensor_id":12345678,"ch":0,"battery_ok":true,"humidity":44,"wind_gust":1.2,"wind_avg":1.2,"wind_dir":150,"rain":146}
```
