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
int sunrise_times[N_SUNTIMES]; 
int tick_delta; 

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
  

  // Set time 
  time_t cur_time = (time_t) get_cur_time();
  rtc.setEpoch(cur_time, false);

  DateTime dt = RTClib::now();
  start = dt.unixtime();

  // Init sunrise times 
  int day = dt.day(); 
  int month = dt.month();
  int hr = dt.hour();
  int min = dt.minute();
  get_sunrise_times(sunrise_times, day, month); 

  /* 
  Astro SR: 10:3:21
  SR: 10:31:51
  SS: 0:32:54
  Astro SS: 1:1:19

  Cur time: 8/16  2:30
  */

  char buff[80];
  sprintf(buff, "Astro SR: %d:%d:%d", sunrise_times[0][0], sunrise_times[0][1], sunrise_times[0][2]);
  Serial.println(buff);
  sprintf(buff, "SR: %d:%d:%d", sunrise_times[1][0], sunrise_times[1][1], sunrise_times[1][2]);
  Serial.println(buff);
  sprintf(buff, "SS: %d:%d:%d", sunrise_times[2][0], sunrise_times[2][1], sunrise_times[2][2]);
  Serial.println(buff);
  sprintf(buff, "Astro SS: %d:%d:%d", sunrise_times[3][0], sunrise_times[3][1], sunrise_times[3][2]);
  Serial.println(buff);

  sprintf(buff, "\nCur time: %d/%d  %d:%d", month, day, hr, min);
  Serial.println(buff);

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
  char buff[80]; 

  switch (curState) {
    case ASTRO_SR_STATE:
      tick_delta = (sunrise_times[SUNRISE] - sunrise_times[NAUT_SUNRISE]) / SUNRISE_LEN; 
      sprintf(buff, "Sunrise (%d s/update)", tick_delta); 
      Serial.println(buff);
      newState = SUNRISE_STATE;
      break;

    case SUNRISE_STATE:
      newState = DAY_STATE;
      tick_delta = -1
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
      tick_delta = (sunrise_times[NAUT_SUNSET] - sunrise_times[ASTRO_SUNSET]) / AT_LEN; 
      sprintf(buff, "Astro Sunset (%d s/update)", tick_delta); 
      Serial.println(buff); 
      break;

    case ASTRO_SS_STATE:
      Serial.println("Nighttime");
      tick_delta = -1; 
      newState = NIGHT_STATE;
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
  DateTime alarmDT = RTClib::now();
  time_t now = alarmDT.unixtime();

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

