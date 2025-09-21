#include <string>

#include "WiFiS3.h"  // Included in board library

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>

#include <RTClib.h>

#include <UnixTime.h>

#include "sunrise_cycle.h"
#include "sunrise_timing.h"

#define NUM_LEDS 50
#define DATA_PIN 8
#define TZ_OFFSET -4

CRGB leds[NUM_LEDS];
uint8_t color_arr[4][3];

// Clock component
RTC_DS3231 rtc; 

// Timing variables 
time_t start; 
int counter = 0;
int sunrise_times[N_SUNTIMES]; 
int tick_delta; 
int last_tick; 
UnixTime ut(0); 

enum State {
  ASTRO_SR_STATE,
  SUNRISE_STATE,
  DAY_STATE,
  SUNSET_STATE,
  ASTRO_SS_STATE, 
  NIGHT_STATE
}; 

enum State curState; 

void get_tomorrow(int dmy[3]) {
  DateTime dt = rtc.now();
  int now = dt.unixtime(); 
  int tomorrow = now + (86400); // Seconds in a day 

  DateTime tm = DateTime(tomorrow); 

  dmy[0] = tm.day(); 
  dmy[1] = tm.month(); 
  dmy[2] = tm.year();
}  

void nightly_update() {
  int tomorrow[3]; 
  get_tomorrow(tomorrow); 
  get_suntimes(sunrise_times, tomorrow[0], tomorrow[1], tomorrow[2], TZ_OFFSET); 
}

void get_last_tick() {
  DateTime dt = rtc.now(); 
  last_tick = dt.hour()*(60*60) + dt.minute()*60 + dt.second(); 
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

void display_curtime() {
  char buff[120];

  DateTime dt = rtc.now();
  get_last_tick(); 

  // Init sunrise times 
  int day = dt.day(); 
  int month = dt.month();
  int hr = dt.hour();
  int min = dt.minute();
  int year = dt.year();

  sprintf(buff, "\nCur time: %d/%d  %d:%02d, (%d)", month, day, hr, min, last_tick);
  Serial.println(buff);
}

void setup() {  
  // Setup LEDs 
  FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS);
  get_starting_colors(color_arr);
  load_palette();
  
  // Turn on serial 
  Serial.begin(9600);
  while (!Serial) { ; }

  // Initialize clock
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Check if RTC lost power
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");

    // Set to compile time
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("RTC already running, keeping time");
  }

  DateTime dt = rtc.now();

  // Init sunrise times 
  int day = dt.day(); 
  int month = dt.month();
  int hr = dt.hour();
  int min = dt.minute();
  int year = dt.year();

  // Get sunrise/set times 
  get_suntimes(sunrise_times, day, month, year, TZ_OFFSET); 
  display_suntimes(); 
  display_curtime(); 
  get_last_tick(); 
  

  // Figure out previous state 
  if (last_tick > sunrise_times[ASTRO_SUNSET]) { curState = ASTRO_SS_STATE; }
  else if (last_tick > sunrise_times[NAUT_SUNSET]) { curState = SUNSET_STATE; }
  else if (last_tick > sunrise_times[SUNSET]) { curState = DAY_STATE; }
  else if (last_tick > sunrise_times[SUNRISE]) { curState = SUNRISE_STATE; }
  else if (last_tick > sunrise_times[NAUT_SUNRISE]) { curState = ASTRO_SR_STATE; }
  else if (last_tick > sunrise_times[ASTRO_SUNRISE]) { curState = NIGHT_STATE; }
  else { curState = ASTRO_SS_STATE; }

  // Set tick_delta
  state_change(); 

  // Fast forward to correct number of updates 
  // TODO 
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

char buf[120];
int T=0;

bool update(int i) {
  switch (curState) {
    case ASTRO_SR_STATE: 
      return increment_civil_sr(i, color_arr); 
      break;
    case SUNRISE_STATE: 
      return increment_sunrise(i, color_arr);
      break;
    case SUNSET_STATE:
      return increment_sunset(i, color_arr);
      break;
    case ASTRO_SS_STATE: 
      return increment_civil_ss(i, color_arr);      
      break;
    default:
      return true;  // Day and night need special handling
  }
}

void state_change() {
  enum State newState; 
  char buff[80]; 

  switch (curState) {
    case -1: // When state_change(state-1) is called 
    case ASTRO_SR_STATE:
      tick_delta = (sunrise_times[SUNRISE] - sunrise_times[NAUT_SUNRISE]) / SUNRISE_LEN; 
      sprintf(buff, "Sunrise (%d s/update)", tick_delta); 
      Serial.println(buff);
      newState = SUNRISE_STATE;
      break;

    case SUNRISE_STATE:
      newState = DAY_STATE;
      tick_delta = -1;
      sprintf(buff, "Daytime"); 
      Serial.println(buff); 
      break;

    case DAY_STATE:
      newState = SUNSET_STATE;
      tick_delta = (sunrise_times[NAUT_SUNSET] - sunrise_times[SUNSET]) / SUNRISE_LEN; 
      sprintf(buff, "Sunset (%d s/update)", tick_delta); 
      Serial.println(buff); 
      break;

    case SUNSET_STATE:
      newState = ASTRO_SS_STATE;
      tick_delta = (sunrise_times[ASTRO_SUNSET] - sunrise_times[NAUT_SUNSET]) / AT_LEN; 
      sprintf(buff, "Astro Sunset (%d s/update)", tick_delta); 
      Serial.println(buff); 
      break;

    case ASTRO_SS_STATE:
      Serial.println("Nighttime");
      tick_delta = -1; 
      newState = NIGHT_STATE;
      nightly_update();
      break;

    case NIGHT_STATE:
      tick_delta = (sunrise_times[NAUT_SUNRISE] - sunrise_times[ASTRO_SUNRISE]) / AT_LEN; 
      sprintf(buff, "Astro Sunrise (%d s/update)", tick_delta); 
      Serial.println(buff); 
      newState = ASTRO_SR_STATE; 
      break;
  }

  counter = 0;
  curState = newState; 
}

void loop() {
  get_last_tick(); 

  if (curState == DAY_STATE) {
    // Can be pretty non-precise while waiting 
    if (sunrise_times[SUNSET] - last_tick > 60) {
      delay(59*1000); // Sleep for a minute and check back later 
      return;
    }

    // Otherwise, just count to 60, then state transition
    while (sunrise_times[SUNSET] > last_tick) {
      delay(1000); 
      get_last_tick();  
    }
  }

  if (curState == NIGHT_STATE) {
    // Night state will always start before midnight and end after midnight
    // First, wait until midnight
    if (last_tick > sunrise_times[ASTRO_SUNSET]) {
      delay(60*1000); 
      return;
    }

    // After midnight, do a similar thing to the sunset waiting procedure above
    if (sunrise_times[ASTRO_SUNRISE] - last_tick > 60) {
      delay(59*1000); 
      return; 
    }

    while (sunrise_times[ASTRO_SUNRISE] > last_tick) {
      delay(1000); 
      get_last_tick(); 
    }
  }


  // Update 
  bool need_state_change = update(counter);
  load_palette();
  counter += 1; 

  // State change (if needed)
  if (need_state_change) { state_change(); }
}

void loop_() { 
  /*  Old code used to verify that cycles worked correctly
      Ignores time 
  */
  DateTime dt = rtc.now();

  // Update 
  bool need_state_change = update(counter);
  load_palette();
  counter += 1; 

  // State change (if needed)
  if (need_state_change) { state_change(); }

  // Test for now. Need to get new sunrise time at midnight 
  if (curState == DAY_STATE || curState == NIGHT_STATE) {
    delay(500); 
    state_change(); 
  } else {
    delay(50); 
  }
}

