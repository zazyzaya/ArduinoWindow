#include <stddef.h>

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>
#include <RTClib.h>
#include <UnixTime.h>

#include "sunrise_cycle.h"
#include "sunrise_timing.h"

#define NUM_LEDS 100
#define DATA_PIN 8
#define TZ_OFFSET -4

CRGB leds[NUM_LEDS];
uint8_t color_arr[4][3];

// Clock component
RTC_DS3231 rtc; 

// Timing variables 
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

struct NextState_t {
  State s; 
  int t; 
}; 
NextState_t ns, cs; 

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

  sprintf(buff, "Current time: %d/%d  %d:%02d, (%d)", month, day, hr, min, last_tick);
  Serial.println(buff);
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

void rtcDelay(int t) {
  DateTime st = rtc.now();
  unsigned long target = st.unixtime() + t;
  while (rtc.now().unixtime() < target) { 
    /* I really wish my chip had the SQW pin so I had alarms :') */
    delay(100); 
  }
}

bool update(int i, State s) {
  switch (s) {
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
      cs.s = SUNRISE_STATE; 
      cs.t = NAUT_SUNRISE; 

      tick_delta = (sunrise_times[SUNRISE] - sunrise_times[NAUT_SUNRISE]) / SUNRISE_LEN; 
      sprintf(buff, "Sunrise (%d s/update)", tick_delta); 
      Serial.println(buff);
      break;

    case SUNRISE_STATE:
      cs.s = DAY_STATE; 
      cs.t = SUNRISE; 

      tick_delta = 0;
      sprintf(buff, "Daytime"); 
      Serial.println(buff); 
      break;

    case DAY_STATE:
      cs.s = SUNSET_STATE; 
      cs.t = SUNSET; 

      tick_delta = (sunrise_times[NAUT_SUNSET] - sunrise_times[SUNSET]) / SUNRISE_LEN; 
      sprintf(buff, "Sunset (%d s/update)", tick_delta); 
      Serial.println(buff); 
      break;

    case SUNSET_STATE:
      cs.s = ASTRO_SS_STATE;
      cs.t = NAUT_SUNSET; 

      tick_delta = (sunrise_times[ASTRO_SUNSET] - sunrise_times[NAUT_SUNSET]) / AT_LEN; 
      sprintf(buff, "Astro Sunset (%d s/update)", tick_delta); 
      Serial.println(buff); 
      break;

    case ASTRO_SS_STATE:
      cs.s = NIGHT_STATE; 
      cs.t = sunrise_times[ASTRO_SUNSET];
      
      tick_delta = 0; 
      Serial.println("Nighttime");
      nightly_update();
      break;

    case NIGHT_STATE:
      cs.s = ASTRO_SR_STATE; 
      cs.t = NAUT_SUNRISE; 

      tick_delta = (sunrise_times[NAUT_SUNRISE] - sunrise_times[ASTRO_SUNRISE]) / AT_LEN; 
      sprintf(buff, "Astro Sunrise (%d s/update)", tick_delta); 
      Serial.println(buff); 
      break;
  }

  counter = 0;
  curState = cs.s; 
}

String state_to_str(State s) {
  switch (s) {
    case ASTRO_SR_STATE: 
      return "Dawn"; 
    case SUNRISE_STATE: 
      return "Sunrise"; 
    case DAY_STATE: 
      return "Daytime"; 
    case SUNSET_STATE: 
      return "Sunset"; 
    case ASTRO_SS_STATE: 
      return "Twilight"; 
    case NIGHT_STATE: 
      return "Nightime"; 
  }
}

void get_next_state(State s) {
  switch (s) {
    case ASTRO_SR_STATE: 
      ns.s = SUNRISE_STATE; 
      ns.t = NAUT_SUNRISE; 
      break; 
    case SUNRISE_STATE: 
      ns.s = DAY_STATE; 
      ns.t = SUNRISE; 
      break; 
    case DAY_STATE: 
      ns.s = SUNSET_STATE; 
      ns.t = SUNSET; 
      break; 
    case SUNSET_STATE: 
      ns.s = ASTRO_SS_STATE; 
      ns.t = NAUT_SUNSET;
      break; 
    case ASTRO_SS_STATE: 
      ns.s = NIGHT_STATE; 
      ns.t = ASTRO_SUNSET; 
      break; 
    case NIGHT_STATE: 
      ns.s = ASTRO_SR_STATE; 
      ns.t = ASTRO_SUNRISE; 
  }
}

void display_state() {
  char buff[120];
  get_next_state(curState); 
  int next_time = sunrise_times[ns.t]; 

  sprintf(
    buff, "Current state: %s. (Next: %s at %d:%d)", 
    state_to_str(curState).c_str(), state_to_str(ns.s).c_str(),
    next_time / (60*60), (next_time / 60) % 60
  ); 

  Serial.println(buff); 
}

int fastforward() {
  /* 
    Returns true if ffwd is complete, false otherwise
  */
  char buff[120]; 
  sprintf(buff, "Fastforwarding through %s", state_to_str(curState).c_str()); 
  Serial.println(buff); 

  get_last_tick(); 
  if (curState == NIGHT_STATE) {
    // To fastforward through night, just determine if the lights should 
    // continue to be off 
    return  last_tick > sunrise_times[ASTRO_SUNSET] ||
            last_tick < sunrise_times[ASTRO_SUNRISE]; 
  } 
  else if (curState == DAY_STATE) {
    // Likewise for daytime 
    return last_tick < sunrise_times[SUNSET];
  }

  // Otherwise iterate through light cycle until system time >= last_tick 
  // Or until we need to switch states 
  int cur_time = sunrise_times[cs.t];
  while (!update(counter, curState)) { 
    load_palette(); 
    counter += 1; 
    get_last_tick(); 

    if (cur_time+(tick_delta*counter) > last_tick) {
      return true; 
    }
  }

  return false;
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
    
    while (1) {
      red(color_arr); 
      load_palette(); 
      delay(500); 
      white(color_arr); 
      load_palette(); 
      delay(500); 
    }
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
  
  curState = NIGHT_STATE; 
  while (!fastforward()) {
    state_change(); 
  }
      
}

//void loop() {;}

void loop() {
  display_curtime(); 
  display_state(); 
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
  bool need_state_change = update(counter, curState);
  load_palette();
  counter += 1; 
  rtcDelay(tick_delta);

  // State change (if needed)
  if (need_state_change) { state_change(); }
}

void loop_() { 
  /*  Old code used to verify that cycles worked correctly
      Ignores time 
  */
  DateTime dt = rtc.now();

  // Update 
  bool need_state_change = update(counter, curState);
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

