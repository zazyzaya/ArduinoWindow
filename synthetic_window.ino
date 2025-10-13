#include <stddef.h>
#include <stdlib.h>

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>
#include <RTClib.h>
#include <UnixTime.h>
#include <Wire.h>

#include "color_calculations.h"
#include "sunrise_timing.h"
#include "secrets.h"

#define NUM_LEDS 100
#define DATA_PIN 2
#define SNOOZE_PIN 4
#define TZ_OFFSET -4
#define ERR 10
#define BAIL 100
#define BUFF_MAX 120

CRGB leds[NUM_LEDS];
uint8_t color_arr[N_COLORS][3];
int led_color[NUM_LEDS]; 

// Clock component
RTC_DS3231 rtc; 

long sunrise_times[N_SUNTIMES]; 
long cur_day = -1; 

double displaying_time; 
long last_update; 
bool snoozed = false; 
long last_debounce = 0; 

// Deal with input
char receivedChars[BUFF_MAX];
boolean newData = false;

long get_last_tick() {
  DateTime dt = rtc.now(); 
  long hr = (long) dt.hour(); 
  long min = (long) dt.minute(); 
  long sec = (long) dt.second(); 

  long last_tick = hr*(60*60) + min*60 + sec; 
  return last_tick; 
}

void display_curtime() {
  char buff[BUFF_MAX];

  DateTime dt = rtc.now();
  long last_tick = get_last_tick(); 

  // Init sunrise times 
  int day = dt.day(); 
  int month = dt.month();
  int hr = dt.hour();
  int min = dt.minute();
  int year = dt.year();

  sprintf(buff, "Current time: %d/%d  %d:%02d, (%ld)", month, day, hr, min, last_tick);
  Serial.println(buff);
}

void display_suntimes() {
  char buff[BUFF_MAX];
  long hr,min; 
  String names[6] = {"Astro SR", "Civ SR", "Civ SS", "Astro SS"}; 
  
  for (int i=0; i<4; i++) {
    hr = sunrise_times[i] / (60*60); 
    min = (sunrise_times[i] / 60) % 60; 
    sprintf(buff, "%s: %ld:%02ld (%ld)", names[i].c_str(), hr, min, sunrise_times[i]);
    Serial.println(buff);
  }
}

void load_palette() {
  FastLED.clear();

  uint8_t *c; 
  for (int i=0; i<NUM_LEDS; i++) {
    c = color_arr[led_color[i]];
    //c  = color_arr[0];
    leds[i] = CHSV(c[0], c[1], c[2]);
  }

  FastLED.show();
}

double binary_search_sun_angles(long tgt, long rising) {
  double st = CIV_ZENITH; 
  double en = ASTRO_ZENITH;  
  double mid = st + (en-st)/2; 

  DateTime dt = rtc.now();
  int day = dt.day(); 
  int month = dt.month();
  int year = dt.year();

  long time = suntime(day, month, year, LAT, LON, mid, rising, TZ_OFFSET); 

  long cnt=0; 
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

double find_nearest_angle(long t) {  
  // Day 
  if (t > sunrise_times[SUNRISE] && t < sunrise_times[SUNSET]) {
    Serial.println("Daytime");
    return CIV_ZENITH; 
  }
  // Night
  else if (t > sunrise_times[ASTRO_SUNSET] || t < sunrise_times[ASTRO_SUNRISE]) {
    Serial.println("Night time");
    return ASTRO_ZENITH; 
  }
  // Mid-sunrise 
  else if (t < sunrise_times[SUNRISE]) {
    Serial.println("Sunrise");
    return binary_search_sun_angles(t, true); 
  }
  // Mid-sunset
  else {
    Serial.println("Sunset");
    return binary_search_sun_angles(t, false); 
  }
}

double loop_update() {
  long now = get_last_tick(); 
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
    Serial.println("Updating sunrise times");
    get_suntimes(sunrise_times, day, month, year, TZ_OFFSET); 
    display_suntimes();
    cur_day = day; 
  }
}

void i2c_unstick() {
  const uint8_t SDA_PIN = A4; 
  const uint8_t SCL_PIN = A5;

  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);

  // If SDA held low, pulse SCL up to 9 times
  for (int i = 0; i < 9 && digitalRead(SDA_PIN) == LOW; i++) {
    pinMode(SCL_PIN, OUTPUT);
    digitalWrite(SCL_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL_PIN, HIGH);
    delayMicroseconds(5);
    pinMode(SCL_PIN, INPUT_PULLUP);
    delayMicroseconds(5);
  }
}

void setup() {  
  // Test it's alive
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW

  // Setup LEDs 
  randomSeed(analogRead(A0));
  FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS);
  for (int i=0; i<NUM_LEDS; i++) {
    led_color[i] = random(0,N_COLORS);
  }
  
  //pinMode(SNOOZE_PIN, INPUT_PULLUP); 

  // Turn on serial 
  Serial.begin(9600);
  while (!Serial) { ; }
  Serial.println("I'm alive!");

  // Initialize clock
  i2c_unstick();  
  Wire.begin();     
  delay(300);         
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    char buff[BUFF_MAX]; 

    for (int i = 0; i < 5; i++) {
        if (rtc.begin()) break;
        delay(500);
        sprintf(buff, "Couldn't find RTC (%d)", i);
        Serial.println(buff);
    }
    if (!rtc.begin()) Serial.println("RTC init failed after retries");
  }

  Serial.println("Found RTC"); 

  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  DateTime dt = rtc.now();

  update_suntimes(); 
  display_suntimes(); 
  display_curtime(); 
  loop_update(); 
}

void log_colors(long i) {
  char buff[BUFF_MAX]; 
  sprintf(
    buff, "H: %03d, S: %03d, V: %03d", 
    color_arr[i][H],
    color_arr[i][S],
    color_arr[i][V]  
  ); 
  Serial.println(buff); 
}

void recv_data() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= BUFF_MAX) {
                ndx = BUFF_MAX - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChars);

        String s = String(receivedChars); 
        uint8_t month = s.substring(0,2).toInt(); 
        uint8_t day = s.substring(3,5).toInt(); 
        uint16_t year = s.substring(6,10).toInt(); 
        uint8_t hour = s.substring(11,13).toInt(); 
        uint8_t minute = s.substring(14,16).toInt();

        char buff[BUFF_MAX]; 
        sprintf(buff, "%02d/%02d/%04d %02d:%02d", month, day, year, hour, minute); 
        Serial.println(buff); 

        if (
          day != 0 && month != 0 && year != 0 && 
          month <= 12 && day <= 31
        ) {
          DateTime dt(year, month, day, hour, minute, 0); 
          rtc.adjust(dt); 
        }

        else {
          Serial.println("Please input a time in the format DD/MM/YYYY HH:MM"); 
        }

        newData = false;
    }
}

void loop() {
  // Update clock if needed
  recv_data(); 
  showNewData(); 

  long now = get_last_tick(); 
  long elapsed = now - last_update;
  
  if (elapsed < 0) {
    // We rolled past midnight, so wrap
    elapsed += 24 * 60 * 60;
    last_update = 0; 
  }

  /* Removed in final implementation
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
  */

  DateTime dt = rtc.now();

  // Check sun position every 60s
  if (elapsed >= 10) {
    // If new day, update the sunrise/set times, otherwise, noop 
    update_suntimes(); 
   
    double percent = loop_update(); 
    display_curtime(); 
    char buff[BUFF_MAX];
    sprintf(buff, "The sun is %d%% up", (int)(percent*100));
    Serial.println(buff); 
  }
}

long pct_to_time(double p, long isRising) {
  DateTime dt = rtc.now();
  long day = dt.day(); 
  long month = dt.month();
  long year = dt.year();
  
  double theta = (-p) * (ASTRO_ZENITH - CIV_ZENITH) + ASTRO_ZENITH; 
  return suntime(day,month,year, LAT, LON, theta, isRising, TZ_OFFSET); 
}

void loop_() { 
  /*  Test code for light cycle. Ignores time
  */
  long t; 
  double p; 
  char buff[BUFF_MAX]; 

  Serial.println("In the loop"); 

  for (int i=0; i<=100; i++) {
    p = ((float)(i))/100; 

    update_leds(color_arr, p); 
    log_colors(0);
    log_colors(1);
    log_colors(2);
    log_colors(3);
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
    log_colors(1);
    log_colors(2);
    log_colors(3);
    load_palette(); 

    t = pct_to_time(p, false); 
    sprintf(buff, "Time: %02d:%02d (%d%%)", t/(60*60), (t/60) % 60, i); 
    Serial.println(buff); 

    delay(100);  
  }
}

