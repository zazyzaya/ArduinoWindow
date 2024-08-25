#include "WiFiS3.h"  // Included in board library

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>

// DS3231 - Version: Latest
// https://github.com/NorthernWidget/DS3231
#include <DS3231.h>
#include <Wire.h>
#include <UnixTime.h>

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

// Timing variables 
time_t start; 
int counter = 0;
int sunrise_times[N_SUNTIMES]; 
int tick_delta; 
int last_tick; 

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
  DateTime dt = RTClib::now();
  int now = dt.unixtime(); 
  int tomorrow = now + (86400); // Seconds in a day 

  UnixTime ut(0); 
  ut.getDateTime(tomorrow); 
  dmy[0] = ut.day; 
  dmy[1] = ut.month; 
  dmy[2] = ut.year;
}  

void nightly_update() {
  int tomorrow[3]; 
  get_tomorrow(tomorrow); 
  get_sunrise_times(sunrise_times, tomorrow[0], tomorrow[1], tomorrow[2]); 
}

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
  DateTime dt = RTClib::now();

  // Init sunrise times 
  int day = dt.day(); 
  int month = dt.month();
  int hr = dt.hour();
  int min = dt.minute();
  int year = dt.year();

  // Need to roll back clock 1hr to account for daylight savings being 
  // inserted by the compiler 
  if ((month > 3 && month < 11) || (month == 3 && day >=10) || (month == 1 && day <= 3)) {
    rtc.setHour((24+hr-1) % 24); 
    dt = RTClib::now();

    // Almost certainly unnecessary, but what if the Arduino is 
    // reset at 23:59:59.99999 on new year's eve? 
    day = dt.day(); 
    month = dt.month();
    hr = dt.hour();
    min = dt.minute();
    year = dt.year();
  }

  // Get sunrise/set times 
  get_sunrise_times(sunrise_times, day, month, year); 

  char buff[80]; 
  dt = RTClib::now(); // Refresh since hitting API takes more than a ms 
  last_tick = dt.unixtime();
  sprintf(buff, "\nCur time: %d/%d  %d:%d, (%d)", month, day, hr, min, last_tick);
  Serial.println(buff);

  // Figure out current state 
  if (last_tick > sunrise_times[ASTRO_SUNSET]) { curState = NIGHT_STATE; }
  else if (last_tick > sunrise_times[NAUT_SUNSET]) { curState = ASTRO_SS_STATE; }
  else if (last_tick > sunrise_times[SUNSET]) { curState = SUNSET_STATE; }
  else if (last_tick > sunrise_times[SUNRISE]) { curState = DAY_STATE; }
  else if (last_tick > sunrise_times[NAUT_SUNRISE]) { curState = SUNRISE_STATE; }
  else if (last_tick > sunrise_times[ASTRO_SUNRISE]) { curState = ASTRO_SS_STATE; }
  else { curState = NIGHT_STATE; }

  // Set tick_delta
  curState -= 1; 
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
      return false;  // Day and night need special handling
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

void loop_() {
  DateTime dt = RTClib::now();
  int now = dt.unixtime();

  if (curState == DAY_STATE) {
    // Can be pretty non-precise while waiting 
    if (sunrise_times[SUNSET] - now > 60) {
      delay(59*1000); // Sleep for a minute and check back later 
      return;
    }

    // Otherwise, just count to 60, then state transition
    while (sunrise_times[SUNSET] > now) {
      delay(1000); 
      dt = RTClib::now(); 
      now = dt.unixtime(); 
    }
  }


  // Update 
  bool need_state_change = update(counter);
  load_palette();
  counter += 1; 

  // State change (if needed)
  if (need_state_change) { state_change(); }
}

void loop() { 
  /*  Old code used to verify that cycles worked correctly
      Ignores time 
  */
  DateTime dt = RTClib::now();
  int now = dt.unixtime();

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

