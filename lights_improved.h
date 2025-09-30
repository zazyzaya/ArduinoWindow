#include <stdio.h>
#include <FastLED.h>
#include <math.h>
#include "Arduino.h" 

#define N_COLORS 4
#define H 0
#define S 1
#define V 2

#define NIGHT 102.
#define END_SUNRISE 85.
#define DAY 70.

#define HUE_CHANGE 100
#define MAX_HUE (255+40)
#define MAX_VAL 200
#define VAL_MIN 30
#define SAT_MIN 120

#define HUE_POLY 1.5
#define SAT_POLY 2
#define VAL_POLY 0.5
#define GREEN_START (45+255)
#define GREEN_OFFSET 85

#define MIN(a, b) ((a) < (b) ? (a) : (b))

const double STOP_HUE_CHANGE = (NIGHT - END_SUNRISE) / (NIGHT - DAY); 
int initColors[] = {180,200,225,190}; 
int finalColors[] = {135,145,255+45,255+40}; 

void white(uint8_t arr[N_COLORS][3]) {
  for (int i=0; i<N_COLORS; i++) {
    arr[i][0] = 0;
    arr[i][1] = 0; 
    arr[i][2] = 255; 
  }
}

void red(uint8_t arr[N_COLORS][3]) {
  for (int i=0; i<N_COLORS; i++) {
    arr[i][0] = 0;
    arr[i][1] = 255; 
    arr[i][2] = 255; 
  }
}

void update_leds(uint8_t arr[N_COLORS][3], double percent) {
    uint8_t s,v; 
    
    s = ((255-SAT_MIN) * (1-pow(percent, SAT_POLY))) + SAT_MIN; 
    v = (MAX_VAL-VAL_MIN) * pow(percent, VAL_POLY); 
    v = (percent != 0) ? v+VAL_MIN : v; // Imperceptable below 20
    
    // For calculating hue we stop changing it after a certain point
    percent = MIN(percent, STOP_HUE_CHANGE) / STOP_HUE_CHANGE; 
    for (int i=0; i<N_COLORS; i++) {
        int h = initColors[i]; 
        //int offset = (finalColors[i] > GREEN_START) ? GREEN_OFFSET : 0; 
        double hue_change = (finalColors[i] - initColors[i]); // - offset; 
        h += (pow(percent, HUE_POLY) * hue_change); 

        // Filter out green values 
        //if (h > GREEN_START) { h += GREEN_OFFSET; }

        // Load array 
        arr[i][H] = h; 
        arr[i][S] = s; 
        arr[i][V] = v; 
    }
}