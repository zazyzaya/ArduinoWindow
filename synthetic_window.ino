// SunPosition - Version: Latest 
// https://github.com/GyverLibs/SunPosition
//#include <SunPosition.h>

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>
#include "time.h"

// Can only go to 2063 (only 5 bits for numbers)
// If someone is using this in 40 years, caveat emptor
Time cur_time = {
  0, 0, 4, 12, 23, 0
}; 

const int TIME_FIDELITY = 6; // Bits of resolution
const int TPIN_0 = 2; 
const int START = 8; 
const int BUTTON = 9; 
const int CLOCK_MODE[] = {10,11,12}; 
const int COUNT_FAST=13;

// Bitmap of months with 31 days 
// (0b101011010101 & (1 << month) is true iff month has 31 days)
const unsigned int thirty_one_days = 2773;

bool is_live = 0; 
float last_time;

# define NUM_LEDS 150
# define DATA_PIN 1
CRGB leds[NUM_LEDS];

void setup() {  
  // Pins for the clock display
  for (int i=TPIN_0; i<TPIN_0+TIME_FIDELITY; i++){
    pinMode(i, OUTPUT);
  }

  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  pinMode(START, INPUT_PULLUP);  // Set to live time
  pinMode(BUTTON, INPUT_PULLUP); // Incriment variables
  pinMode(COUNT_FAST, INPUT); // If 1 will add 10 to value instead of 1
  
  // Keep track of what we're showing/adding value to 
  for (int i=0; i<3; i++) {
    pinMode(CLOCK_MODE[i], INPUT);
  }

  Serial.begin(9600);
}

TimeUnit get_mode() {
  int mode = 0; 
  for (int i=0; i<3; i++) {
    int val = digitalRead(CLOCK_MODE[i]); 
    mode = mode + (val << i); 
  }
  return static_cast<TimeUnit>(mode);
}

void display_time(TimeUnit mode) {
  int dval; 
  switch(mode) {
    case SECONDS: dval = (int) cur_time.second; break;
    case MINUTES: dval = cur_time.minute; break;
    case HOURS: dval = cur_time.hour; break;
    case DAYS: dval = cur_time.day; break;
    case MONTHS: dval = cur_time.month; break;
    case YEARS: dval = cur_time.year; break;
  }

  for (int i=0; i<TIME_FIDELITY; i++){
    digitalWrite(i+TPIN_0, dval & 0x1); 
    dval = dval >> 1; 
  }
}


bool is_leap_year(){
  int y = cur_time.year + 2000;
  return (y % 4 == 0) && (!(y % 100) || (y % 400 == 0));
}

void update() {
  float elapsed = millis() - last_time; 
  if (elapsed < 1) {
    return;
  }

  last_time = millis();

  cur_time.second = cur_time.second + (elapsed / 1000); 
  if (cur_time.second >= 60) {
    cur_time.minute += (int)(cur_time.second / 60); 
    cur_time.second -= 60;
  }
  if (cur_time.minute >= 60) {
    cur_time.hour += cur_time.minute % 60; 
    cur_time.minute %= 60;
  }
  if (cur_time.hour >= 24) {
    cur_time.day += 1; // Like, c'mon let's be real. It's just gonna be 1 day
    cur_time.hour = 0; 
  }

  if (cur_time.day > 31 && (1 << cur_time.month) & thirty_one_days) {
    cur_time.month += 1; 
    cur_time.day = 1; // Days aren't zero-indexed
  }
  else if (cur_time.day > 30 && !((1 << cur_time.month) & thirty_one_days)) {
    // Don't worry, February has it's own block after this one 
    cur_time.month += 1; 
    cur_time.day = 1; 
  }
  else if (cur_time.month == 2) {
    if (
      (is_leap_year() && cur_time.day > 29) ||
      (cur_time.day > 28)
    ) {
      cur_time.month = 3; 
      cur_time.day = 1; 
    }
  }

  if (cur_time.month == 13) {
    cur_time.month = 1; 
    cur_time.year += 1;
  }
}

void incriment(TimeUnit mode) {
  Serial.println("incriment called"); 
  
  int inc; 
  int loopback; 
  int fast = digitalRead(COUNT_FAST);
  void *ptr; 

  switch (mode) {
    case SECONDS: 
      ptr = &cur_time.second;
      inc= (9*fast) + 1; 
      loopback = 60;
      break;
    case MINUTES:
      ptr = &cur_time.minute;
      inc= (9*fast) + 1; 
      loopback = 60;
      break;
    case HOURS: 
      ptr = &cur_time.hour;
      inc = (4*fast) + 1; 
      loopback = 24; 
      break;
    case DAYS: 
      ptr = &cur_time.day; 
      inc = (4*fast) + 1;
      loopback = 31; 
      break;
    case YEARS: 
      ptr = &cur_time.year;
      inc = (9*fast) + 1; 
      loopback = 100;
      break;
  }

  if (mode == SECONDS) {
    float *cast_ptr = (float *) ptr; 
    float val = *cast_ptr; 

    val += inc;
    while (val > loopback) {
      val -= loopback; // Float version of mod 
    }

    *cast_ptr = val;
  }
  else {
    int *cast_ptr = (int *) ptr;  
    Serial.print(*cast_ptr);
    Serial.print("->");
    *cast_ptr = (*cast_ptr + inc) % loopback;
    Serial.println(*cast_ptr);
  }
}

void debounce(int pin) {
  while (1) {
    if (digitalRead(pin)) {
      delay(100);
      return;
    }
  }
}

int r=0, g=50, b=100;
void display_lights() {
  b += 5; 
  r %= 255; g %= 255; b %=255;
  FastLED.clear();
  for (int i=0; i<NUM_LEDS; i++) {
    leds[i] = CRGB(r,g,b);
    FastLED.show();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  /*
  TimeUnit tmode = get_mode(); 
  if (!digitalRead(BUTTON) && !is_live) {
    debounce(BUTTON);
    incriment(tmode);
  }

  if (!digitalRead(START) && !is_live) {
    last_time=millis(); 
    is_live = 1; 
  }

  if (is_live) {
    update();
  }

  display_time(tmode); 
  */
  
  FastLED.clear();
  for (int dot=0; dot < NUM_LEDS; dot++) {
    leds[dot] = CRGB::Blue; 
  }
  FastLED.show();
  delay(30);
}