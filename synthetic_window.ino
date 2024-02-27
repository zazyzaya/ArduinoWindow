#include "WiFiS3.h"  // Included in board library
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>

#include "sunrise_cycle.h"
#include "secrets.h" // Defines SSID, PASS, LAT and LONG
#include "wifi_helpers.ino" // Defines find_date(String s)

# define NUM_LEDS 50
# define DATA_PIN 8

CRGB leds[NUM_LEDS];
uint8_t color_arr[4][3];

ArduinoLEDMatrix matrix; 

// In secrets.h
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

WiFiClient client;
char user_agent[] = "User-Agent: ArduinoWifi/1.1";
char server[] = "new.earthtools.org";

char curtime_http[30]; 
char suntimes_http[30];

void setup() {  
  FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS);
  get_starting_colors(color_arr);
  load_palette();
  
  Serial.begin(9600);
  while (!Serial) { ; }

  matrix.begin();

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

  sprintf(curtime_http, "/timezone/%s/%s", LAT, LONG);
  sprintf(suntimes_http, "/sun/%s/%s/", LAT, LONG);
}

void get_cur_time() {
  char line[80]; 
  if (client.connect(server, 80)) {
    Serial.println("connected successfully");
    Serial.println("~~~~~~~~~~");

    sprintf(line, "GET %s HTTP/1.1", curtime_http);
    client.println(line); 
    Serial.println(line);

    sprintf(line, "Host: %s", server);
    client.println(line);
    Serial.println(line);

    client.println(user_agent);
    Serial.println(user_agent);

    client.println("Connection: close");
    Serial.println("Connection: close");
    
    client.println();
    Serial.println();

    delay(1000);
    read_response();
  }
  else {
    Serial.println("Couldn't connect");
    delay(1000);
    Serial.println("Trying again"); 
    get_cur_time();
  }
}

String read_response() {
  Serial.println("Reading response..."); 
  String resp = "";

  uint32_t received_data_num = 0;
  while (client.available()) {
    String result = client.readString(); 
    resp += result; 
    Serial.println(result);
  }

  return resp;
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

bool sunset() {
  uint8_t *hsv; 
  bool changed = 0;

  for (int row=0; row < N_COLORS; row++) {
    hsv = color_arr[row]; 
    
    // How many times has it been decrimented
    int t = MAX_V - hsv[V]; 

    // V decriments no matter what until 
    // night time 
    if (hsv[V] > MIN_V) {
      hsv[V] -= 1;
    }

    if (t >= CYCLE_LEN - CycleLens[row][H]) {
      hsv[H] -= 1;
    }
    if (t >= CYCLE_LEN - CycleLens[row][S]) {
      hsv[S] += 1;
    }

    changed += hsv[V] > MIN_V;
  }

  return changed;
}

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
  get_cur_time();

  Serial.write("Sunrise\n");
  while (changed) {
    changed = sunrise(); 
    load_palette(); 
    delay(200);
  }

  changed = 1; 
  Serial.write("Sunset\n");
  while (changed) {
    changed = sunset(); 
    load_palette(); 
    delay(200); 
  }

  get_starting_colors(color_arr);
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