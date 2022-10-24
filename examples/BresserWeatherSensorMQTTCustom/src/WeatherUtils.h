#if !defined(WEATHER_UTILS_H)
#define WEATHER_UTILS_H

/*!
 * \brief Calculate dew point
 * 
 * \param celsius air temperature in °C
 * \param humidity relative humidity in %
 * 
 * \returns dew point temperature in °C
 */
float calcdewpoint(float celsius, float humidity);

/*!
 * \brief Calculate windchill temperature
 * 
 * Results are valid for temperatures <= 10°C and windspeeds >4.8 km/h only!
 * 
 * \param celsius air temperature in °C
 * \param windspeed wind speed in km/h
 * 
 * \returns windchill tempoerature in °C
 */
float calcwindchill(float celsius, float windspeed);

/*!
 * \brief Calculate heat index
 * 
 * Results are valid for temperatures >= 16.7°C and humidity >40% only!
 * 
 * \param celsius air temperature in °C
 * \param humidity relative humidity in %
 * 
 * \returns heat index in °C
 */
float calcheatindex(float celsius, float humidity);

/*!
 * \brief Calculate Humidex
 * 
 * \param temperature air temperature in °C
 * \param humidity relative humidity in %
 * 
 * \returns Humidex
 */
float calchumidex(float temperature, float humidity);

/*!
 * \brief Calculate perceived temperature (feels-like temperature)
 * 
 * Apply windchill or heat index depending on current data or
 * just return the real temperature.
 * 
 * \param celsius air temperature in °C
 * \param windspeed wind speed in km/h
 * \param humidity relative humidity in %
 * 
 * \returns perceived temperature in °C
 */
float perceived_temperature(float celsius, float windspeed, float humidity);

/*!
 * \brief Convert wind direction from Degrees to text (N, NNE, NE, ...)
 *
 * \param dir Wind direction in degrees
 * \param buf buffer for result (4 characters required)
 * 
 * \returns pointer to buffer
 */
char * winddir_flt_to_str(float dir, char * buf);

/*!
 * \brief Converts wind speed from Meters per Second to Beaufort.
 * 
 * \param ms Wind speed in m/s.
 * 
 * \returns Wind speed in bft.
*/        
uint8_t windspeed_ms_to_bft(float ms);

#endif
