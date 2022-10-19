#if !defined(WEATHER_SENSOR_CFG_H)
#define WEATHER_SENSOR_CFG_H

// Number of sensors to be received
#define NUM_SENSORS 1

// Use pinning for LoRaWAN Node 
//#define LORAWAN_NODE


// List of sensor IDs to be excluded - can be empty
uint32_t const sensor_ids_exc[] = {};
//uint32_t const sensor_ids_exc[] = { 0x39582376 };


// List of sensor IDs to be included - if empty, handle all available sensors
uint32_t const sensor_ids_inc[] = {};
//uint32_t const sensor_ids_inc[] = { 0x83750871 };


// Select appropriate sensor message format(s)
#define BRESSER_5_IN_1
#define BRESSER_6_IN_1

// Select type of receiver module
#define USE_CC1101
//#define USE_SX1276

// Disable data type which will not be used to save RAM
#define WIND_DATA_FLOATINGPOINT
#define WIND_DATA_FIXEDPOINT


//#define _DEBUG_MODE_

// Arduino default SPI pins
//
// Board   SCK   MOSI  MISO
// ESP8266 D5    D7    D6
// ESP32   D18   D23   D19
#if defined(LORAWAN_NODE)
    #define PIN_RECEIVER_CS   14
    
    // CC1101: GDO0 / RFM95W/SX127x: G0
    #define PIN_RECEIVER_IRQ  4 
    
    // CC1101: GDO2 / RFM95W/SX127x: G1
    #define PIN_RECEIVER_GPIO 16
    
    // RFM95W/SX127x - GPIOxx / CC1101 - RADIOLIB_NC
    #define PIN_RECEIVER_RST  12
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

#endif
