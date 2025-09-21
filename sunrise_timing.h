#include "Arduino.h"

#ifndef SUNTIME_HELPERS_
#define SUNTIME_HELPERS_

#define ASTRO_SUNRISE 0
#define NAUT_SUNRISE 1
#define SUNRISE 2
#define SUNSET 3
#define NAUT_SUNSET 4
#define ASTRO_SUNSET 5 
#define N_SUNTIMES 6

#define CIV_ZENITH 90
#define NAUT_ZENITH 96
#define ASTRO_ZENITH 102

#define SUN_HOUR 0
#define SUN_MIN 1
#define SUN_SEC 2

void get_suntimes(int sunrise_times[N_SUNTIMES], int day, int month, int year, int tz_offset);

#endif 