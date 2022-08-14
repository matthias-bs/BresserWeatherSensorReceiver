#include <math.h>
#include "WeatherUtils.h"

/*
 * From https://www.brunweb.de/wetterstation-berechnungen/
 */

/*
 * Source: https://myscope.net/taupunkttemperatur/
 * 
 * Die Berechnung des Taupunktes erfolgt aus den Messwerten Temperatur (°C) und Luftfeuchtigkeit (%).
 * Calculation is done from the measurement values temperature (°C) and humidity (%).
 */
float calcdewpoint(float celsius, float humidity)
{
  float a;
  float b;
  
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
