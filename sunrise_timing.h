#include "Arduino.h"

#ifndef SUNTIME_HELPERS_
#define SUNTIME_HELPERS_

#define ASTRO_SUNRISE 0
#define SUNRISE 1
#define SUNSET 2
#define ASTRO_SUNSET 3 
#define N_SUNTIMES 4

#define CIV_ZENITH 70.0
#define ASTRO_ZENITH 102.0 // Using nautical instead

#define SUN_HOUR 0
#define SUN_MIN 1
#define SUN_SEC 2

void get_suntimes(long sunrise_times[N_SUNTIMES], int day, int month, int year, int tz_offset);
long suntime(double day, double month, double year, double lat, 
              double lon, double zenith, int risingTime, int tz_offset); 
#endif 