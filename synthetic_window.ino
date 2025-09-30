#include <stddef.h>

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>
#include <RTClib.h>
#include <UnixTime.h>

#include "lights_improved.h"
#include "sunrise_timing.h"
#include "secrets.h"

#define NUM_LEDS 25
#define DATA_PIN 8
#define SNOOZE_PIN 10
#define TZ_OFFSET -4
#define ERR 10
#define BAIL 100

CRGB leds[NUM_LEDS];
uint8_t color_arr[4][3];

// Clock component
RTC_DS3231 rtc; 

int sunrise_times[N_SUNTIMES]; 
int cur_day = -1; 

double displaying_time; 
int last_update; 
bool snoozed = false; 
int last_debounce = 0; 

int get_last_tick() {
  DateTime dt = rtc.now(); 
  int last_tick = dt.hour()*(60*60) + dt.minute()*60 + dt.second(); 
  return last_tick; 
}

void display_curtime() {
  char buff[120];

  DateTime dt = rtc.now();
  int last_tick = get_last_tick(); 

  // Init sunrise times 
  int day = dt.day(); 
  int month = dt.month();
  int hr = dt.hour();
  int min = dt.minute();
  int year = dt.year();

  sprintf(buff, "Current time: %d/%d  %d:%02d, (%d)", month, day, hr, min, last_tick);
  Serial.println(buff);
}

void display_suntimes() {
  char buff[120];
  int hr,min; 
  String names[6] = {"Astro SR", "Naut SR", "Civ SR", "Civ SS", "Naut SS", "Astro SS"}; 
  
  for (int i=0; i<6; i++) {
    hr = sunrise_times[i] / (60*60); 
    min = (sunrise_times[i] / 60) % 60; 
    sprintf(buff, "%s: %d:%02d (%d)", names[i].c_str(), hr, min, sunrise_times[i]);
    Serial.println(buff);
  }
}

void load_palette() {
  FastLED.clear();

  uint8_t *c; 
  for (int i=0; i<NUM_LEDS; i++) {
    c = color_arr[i % N_COLORS];
    leds[i] = CHSV(c[0], c[1], c[2]);
  }

  FastLED.show();
}

double binary_search_sun_angles(int tgt, int rising) {
  double st = CIV_ZENITH; 
  double en = ASTRO_ZENITH;  
  double mid = st + (en-st)/2; 

  DateTime dt = rtc.now();
  int day = dt.day(); 
  int month = dt.month();
  int year = dt.year();

  int time = suntime(day, month, year, LAT, LON, mid, rising, TZ_OFFSET); 

  int cnt=0; 
  while (abs(time-tgt) > ERR) { 
    if ((time > tgt && rising) || (time < tgt && !rising)) {
      st = mid; 
    } else {
      en = mid; 
    }
    mid = st + (en-st)/2; 
    time = suntime(day, month, year, LAT, LON, mid, rising, TZ_OFFSET); 

    if (cnt++ > BAIL) { return mid; }
  }

  return mid; 
}

double find_nearest_angle(int t) {  
  // Day 
  if (t > sunrise_times[SUNRISE] && t < sunrise_times[SUNSET]) {
    return CIV_ZENITH; 
  }
  // Night
  else if (t > sunrise_times[ASTRO_SUNSET] && t < sunrise_times[ASTRO_SUNRISE]) {
    return ASTRO_ZENITH; 
  }
  // Mid-sunrise 
  else if (t < sunrise_times[SUNRISE]) {
    return binary_search_sun_angles(t, true); 
  }
  // Mid-sunset
  else {
    return binary_search_sun_angles(t, false); 
  }
}

double loop_update() {
  int now = get_last_tick(); 
  double zenith = find_nearest_angle(now); 
  double percent = (ASTRO_ZENITH-zenith) / (ASTRO_ZENITH-CIV_ZENITH); 
  
  update_leds(color_arr, percent);
  load_palette(); 
  last_update = now; 
  
  return percent; 
}

void update_suntimes() {
  DateTime dt = rtc.now();
  int day = dt.day(); 
  int month = dt.month();
  int year = dt.year();

  if (day != cur_day) {
    get_suntimes(sunrise_times, day, month, year, TZ_OFFSET); 
    cur_day = day; 
  }
}

void setup() {  
  // Setup LEDs 
  FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS);
  update_leds(color_arr, 0);
  load_palette();
  
  pinMode(SNOOZE_PIN, INPUT_PULLUP); 

  // Turn on serial 
  Serial.begin(9600);
  while (!Serial) { ; }

  // Initialize clock
  Wire.begin(); 
  delay(200);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    
    while (1) {
      Wire.end();
      red(color_arr); 
      load_palette(); 
      delay(500); 
      white(color_arr); 
      load_palette(); 
      delay(500); 
    
      Wire.begin();
      if (rtc.begin()) { break; }
    }
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Check if RTC lost power
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");

    // Set to compile time
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("RTC already running, keeping time");
  }
  

  DateTime dt = rtc.now();

  update_suntimes(); 
  display_suntimes(); 
  display_curtime(); 
  loop_update(); 
}

void log_colors(int i) {
  char buff[120]; 
  sprintf(
    buff, "H: %03d, S: %03d, V: %03d", 
    color_arr[i][H],
    color_arr[i][S],
    color_arr[i][V]  
  ); 
  Serial.println(buff); 
}

void loop() {
  int now = get_last_tick(); 
  int elapsed = now - last_update;
  
  if (elapsed < 0) {
    // We rolled past midnight, so wrap
    elapsed += 24 * 60 * 60;
  }

  if ((digitalRead(SNOOZE_PIN) == HIGH) && 
      (millis() - last_debounce > 500)) {

    last_debounce = millis(); 
    if (!snoozed) {
      FastLED.clear();
      FastLED.show(); 
      snoozed = true; 
      Serial.println("Snoozed"); 
    } 
    else {
      snoozed = false; 
      elapsed = 100; 
      Serial.println("Woke up");
    }
  }

  DateTime dt = rtc.now();
  if (dt.year() < 2020) {
    Serial.println("RTC read error!");
    Wire.end();

    delay(50);
    Wire.begin();
    if (!rtc.begin()) {
      Serial.println("RTC not found after reset attempt");
    } 
  }

  // Unsnooze at midnight 
  if (now == 0) {
    snoozed = false; 
  }

  if (snoozed) { return; }

  // Check sun position every 60s
  if (elapsed > 10) {
    // If new day, update the sunrise/set times, otherwise, noop 
    update_suntimes(); 
   
    double percent = loop_update(); 
    display_curtime(); 
    char buff[120];
    sprintf(buff, "The sun is %0.2f%% up", percent*100);
    Serial.println(buff); 
  }
}

int pct_to_time(double p, int isRising) {
  DateTime dt = rtc.now();
  int day = dt.day(); 
  int month = dt.month();
  int year = dt.year();
  
  double theta = (-p) * (ASTRO_ZENITH - CIV_ZENITH) + ASTRO_ZENITH; 
  return suntime(day,month,year, LAT, LON, theta, isRising, TZ_OFFSET); 
}

void loop_() { 
  /*  Test code for light cycle. Ignores time
  */
  int t; 
  double p; 
  char buff[120]; 

  for (int i=0; i<=100; i++) {
    p = ((float)(i))/100; 

    update_leds(color_arr, p); 
    log_colors(0);
    load_palette(); 

    t = pct_to_time(p, true); 
    sprintf(buff, "Time: %02d:%02d (%0.2f%%)", t/(60*60), (t/60) % 60, p); 
    Serial.println(buff); 

    delay(100);  
  }
  delay(500); 
  for (int i=100; i>=0; i--) {
    p = ((float)(i))/100; 

    update_leds(color_arr, p); 
    log_colors(0);
    load_palette(); 

    t = pct_to_time(p, false); 
    sprintf(buff, "Time: %02d:%02d (%0.2f%%)", t/(60*60), (t/60) % 60, p); 
    Serial.println(buff); 

    delay(100);  
  }
}

