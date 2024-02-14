// SunPosition - Version: Latest 
// https://github.com/GyverLibs/SunPosition
//#include <SunPosition.h>

// FastLED - Version: Latest 
// https://github.com/FastLED/FastLED
#include <FastLED.h>

#include "sunrise_cycle.h"

# define NUM_LEDS 50
# define DATA_PIN 8

CRGB leds[NUM_LEDS];
uint8_t color_arr[4][3];

void setup() {  
  FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS);
  get_starting_colors(color_arr);
  load_palette();
  Serial.begin(9600);
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

bool incriment_each() {
  uint8_t *hsv; 
  bool changed = 0;

  for (int row=0; row < N_COLORS; row++) {
      hsv = color_arr[row]; 

      if (hsv[H] < 42 || hsv[H] > 166) { hsv[H] += 1; changed = 1;}
      if (hsv[S] > 90) { hsv[S] -= 1; changed = 1;} 
      if (hsv[V] < 254) {hsv[V] += 1; changed = 1;}

      sprintf(buf, "%d,%d,%d\n", hsv[H], hsv[S], hsv[V]);
      Serial.write(buf);
  }

  return changed; 
}

//int brightness[] = {25,50,75,100,125,150,175,200,225,255,255,255,255};
void loop() {
  sprintf(buf, "%d\n", T);
  Serial.write(buf); 

  //FastLED.setBrightness(brightness[i]);
  bool changed = incriment_each();
  load_palette();

  if (changed) {
    delay(100);
  }
  else {
    delay(2000); 
    get_starting_colors(color_arr);
  }
}