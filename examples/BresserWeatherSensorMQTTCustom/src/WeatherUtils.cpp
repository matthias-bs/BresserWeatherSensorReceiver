#include <math.h>
#include <string>
#include <string.h>
#include "WeatherUtils.h"

// Mapping of wind direction in degrees to text
// Note: add your localization as desired
const std::string COMPASS_POINTS[17] = {
    "N", "NNE", "NE", "ENE", 
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW",
    "N"
};

/*
 * Convert wind direction from Degrees to text (N, NNE, NE, ...)
 */
char * winddir_flt_to_str(float dir, char * buf)
{
    std::string point = COMPASS_POINTS[(int)((dir + 11.25)/22.5)];
    strncpy(buf, point.c_str(), sizeof(buf-1));
    buf[point.length()] = '\0';
    
    return buf;
};

//
// Convert wind speed from meters per second to Beaufort
// [https://en.wikipedia.org/wiki/Beaufort_scale]
//
uint8_t windspeed_ms_to_bft(float ms)
{
  if (ms < 5.5) {
    // 0..3 Bft
    if (ms < 0.9) {
      return 0;
    } else if (ms < 1.6) {
      return 1;
    } else if (ms < 3.4) {
      return 2;
    } else {
      return 3;
    }
  } else if (ms < 17.2) { 
    // 4..7 Bft
    if (ms < 8) {
      return 4;
    } else if (ms < 10.8) {
      return 5;
    } else if (ms < 13.9) {
      return 6;
    } else {
      return 7;
    }
  } else {
    // 8..12 Bft
    if (ms < 20.8) {
      return 8;
    } else if (ms < 24.5) {
      return 9;
    } else if (ms < 28.5) {
      return 10;
    } else if (ms < 32.7) {
      return 11;
    } else {
      return 12;
    }
  }
}

/*
 * ------------------------------------------------------------------------------------------------
 * From https://www.brunweb.de/wetterstation-berechnungen/
 * ------------------------------------------------------------------------------------------------
 */

/*
 * Source: https://myscope.net/taupunkttemperatur/
 * 
 * Die Berechnung des Taupunktes erfolgt aus den Messwerten Temperatur (°C) und Luftfeuchtigkeit (%).
 * Calculation is done from the measurement values temperature (°C) and humidity (%).
 */
float calcdewpoint(float celsius, float humidity)
{
  float a = 0;
  float b = 0;
  
  if (celsius >= 0) {
    a = 7.5;
    b = 237.3;
  } else if (celsius < 0) {
    a = 7.6;
    b = 240.7;
  }

  // Sättigungsdampfdruck/saturation vapour pressure (hPa)
  float sdd = 6.1078 * pow(10, (a * celsius) / (b + celsius));

  // Dampfdruck/vapour pressure (hPa)
  float dd = sdd * (humidity / 100);

  // v-Parameter
  float v = log10(dd / 6.1078);

  // Taupunkttemperatur/dew point (°C)
  float td = (b * v) / (a - v);
  
  // Runden 1 Nachkommastelle / round to 1 decimal
  td =  round(td * 10) / 10;
  
  return td;
}

/*
 * Source:   https://myscope.net/windchill-gefuehlte-temperatur-berechnen/
 *           https://de.wikipedia.org/wiki/Windchill
 * 
 *           Vadid for temperatures <= 10°C and windspeeds >4.8 km/h 
 */
float calcwindchill(float celsius, float windspeed_ms) 
{
  float windspeed_kmh = windspeed_ms * 3.6;
  float windchill = 13.12 + 0.6215 * celsius - 11.37 * pow(windspeed_kmh, 0.16) + 0.3965 * celsius * pow(windspeed_kmh, 0.16);
  
  return windchill;
}

/*
 * Source:  https://myscope.net/hitzeindex-gefuehle-temperatur/
 *          https://de.wikipedia.org/wiki/Hitzeindex
 * 
 *          Valid for temperatures >= 16,7°C and humidity >40%
 */
float calcheatindex(float celsius, float humidity) {
  return (-8.784695 + 1.61139411 * celsius + 2.338549 * humidity - 0.14611605 * celsius * humidity - 0.012308094 * celsius * celsius - 0.016424828 * humidity * humidity + 0.002211732 * celsius * celsius * humidity + 0.00072546 * celsius * humidity * humidity - 0.000003582 * celsius * celsius * humidity * humidity);
}

/*
 * Source:  https://myscope.net/hitzeindex-gefuehle-temperatur/
 *
 *          Valid for Valid for temperatures >= 27°C and humidity >=40%
 */
float calchumidex(float temperature, float humidity) {
  float e = (6.112 * pow(10,(7.5 * temperature/(237.7 + temperature))) * humidity/100); //vapor pressure
  float humidex = temperature + 0.55555555 * (e - 10.0); //humidex
  return humidex;
}

float perceived_temperature(float celsius, float windspeed, float humidity)
{
    if ((celsius <= 10) && (windspeed * 3.6 > 4.8)) {
        return calcwindchill(celsius, windspeed);
    }
    else if ((celsius >= 16.7) && (humidity > 40)) {
        return calcheatindex(celsius, humidity);
    }
    else {
        return celsius;
    }
}
