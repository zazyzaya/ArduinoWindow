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

// Interrupt signaling byte
volatile byte clock_alert = 0;

// Alarm parameters
byte alarmDay = 11;
byte alarmHour = 13;
byte alarmMinute = 53;
byte alarmSecond = 30;
byte alarmBits = 0b00001000;  // Alarm once a day (see: https://github.com/NorthernWidget/DS3231/blob/master/Documentation/Alarms.md#alarm-bits-quick-reference)
bool alarmIsDay = false;      // True if day represents day of week
bool alarmH12 = false;        // Uses 24hr clock
bool alarmPM = false;         // Ditto 

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
  Serial.print(cur_time);
  rtc.setEpoch(cur_time, false);

  DateTime dt = RTClib::now();
  start = dt.unixtime();
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


bool sunrise() {
  uint8_t *hsv; 
  bool changed = 0;

  for (int row=0; row < N_COLORS; row++) {
      hsv = color_arr[row]; 

      if (hsv[H] < MAX_H || hsv[H] > MIN_H) { hsv[H] += 1; changed = 1;}
      if (hsv[S] > MIN_S) { hsv[S] -= 1; changed = 1;} 
      if (hsv[V] < MAX_V) {hsv[V] += 1; changed = 1;}
  }

  return changed; 
}

void loop() { 
  bool changed = 1; 
  int steps = 0; 
  DateTime alarmDT = RTClib::now();
  time_t now = alarmDT.unixtime();

  if ((now - start) > 30) {
    clock_alert = 0; 
    Serial.write("Astro twilight\n"); 
    for (int i=0; i<AT_LEN; i++) {
      load_palette(); 
      delay(200);
      civil_twilight(color_arr);
    }

    Serial.write("Sunrise\n");
    while (changed) {
      changed = sunrise(); 
      load_palette(); 
      delay(200);
      steps += 1; 
    }

    get_starting_colors(color_arr);
  }
  else {
    Serial.print("Will begin in "); 
    Serial.print(now-start); 
    Serial.print("seconds\n");
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

