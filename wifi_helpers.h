#include "Arduino.h"

#ifndef WIFI_HELPERS_
#define WIFI_HELPERS_

#define CIVIL_SUNRISE 0
#define SUNRISE 1
#define SUNSET 2
#define CIVIL_SUNSET 3

void get_sunrise_times(int* sunrise_times);
int get_cur_time();

#endif 