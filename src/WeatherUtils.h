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
 * \brief Calculate perceptive temperature (feels-like temperature)
 * 
 * Apply windchill or heat index depending on current data or
 * just return the real temperature.
 * 
 * \param celsius air temperature in °C
 * \param windspeed wind speed in km/h
 * \param humidity relative humidity in %
 * 
 * \returns perceptive temperature in °C
 */
float perceptive_temperature(float celsius, float windspeed, float humidity);
#endif
