#include "WiFiS3.h"  // Included in board library

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>

// DS3231 - Version: Latest
// https://github.com/NorthernWidget/DS3231
#include <DS3231.h>
#include <Wire.h>

#include "sunrise_cycle.h"
#include "secrets.h" // Defines SSID and PASS
#include "wifi_helpers.h"

# define NUM_LEDS 50
# define DATA_PIN 8

CRGB leds[NUM_LEDS];
uint8_t color_arr[4][3];

// Clock component
DS3231 rtc; 

// In secrets.h
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
  
time_t start; 
int counter = 0;

enum State {
  ASTRO_SR_STATE,
  SUNRISE_STATE,
  DAY_STATE,
  SUNSET_STATE,
  ASTRO_SS_STATE, 
  NIGHT_STATE
}; 

enum State curState; 

void setup() {  
  FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS);
  get_starting_colors(color_arr);
  load_palette();
  
  Serial.begin(9600);
  while (!Serial) { ; }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed");
    while (true)
      ;  // Exit
  }
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);
  }

  Serial.print("Connected");
  printWifiStatus();

  // Initialize clock
  Wire.begin();
  

  /* Don't thrash API while testing
  // Set time 
  time_t cur_time = (time_t) get_cur_time();
  Serial.print(cur_time);
  rtc.setEpoch(cur_time, false);

  DateTime dt = RTClib::now();
  start = dt.unixtime();

  // TODO figure out what state it is
  */
  curState = ASTRO_SR_STATE;
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
      return false;  // Day and night need special handling
  }
}

void state_change() {
  enum State newState; 
  switch (curState) {
    case ASTRO_SR_STATE:
      Serial.println("Sunrise");
      newState = SUNRISE_STATE;
      break;
    case SUNRISE_STATE:
      Serial.println("Daytime");
      newState = DAY_STATE;
      break;
    case DAY_STATE:
      Serial.println("Sunset");
      newState = SUNSET_STATE;
      break;
    case SUNSET_STATE:
      Serial.println("Astro Sunset");
      newState = ASTRO_SS_STATE;
      break;
    case ASTRO_SS_STATE:
      Serial.println("Nighttime");
      newState = NIGHT_STATE;
      break;
    case NIGHT_STATE:
      Serial.println("Astro sunrise");
      newState = ASTRO_SR_STATE; 
      break;
  }

  counter = 0;
  curState = newState; 
}

void loop() { 
  DateTime alarmDT = RTClib::now();
  time_t now = alarmDT.unixtime();

  bool need_state_change = update(counter);
  load_palette();
  counter += 1; 
  if (need_state_change) { state_change(); }

  // Test for now. Need to get new sunrise time at midnight 
  if (curState == DAY_STATE || curState == NIGHT_STATE) {
    delay(500); 
    state_change(); 
  } else {
    delay(50); 
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

