// Select appropriate sensor message format
//#define BRESSER_5_IN_1
#define BRESSER_6_IN_1

// Select type of receiver module
// (RFM95W: USE_SX1276)
//#define USE_CC1101
#define USE_SX1276

//#define _DEBUG_MODE_

// Arduino default SPI pins
//
// Board   SCK   MOSI  MISO
// ESP8266 D5    D7    D6
// ESP32   D18   D23   D19

#if defined(ESP32)
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
