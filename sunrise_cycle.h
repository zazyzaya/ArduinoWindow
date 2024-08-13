#include <stdio.h>
#include <FastLED.h>
#include "Arduino.h" 

#define N_COLORS 4
#define H 0
#define S 1
#define V 2

// Constrain sunrise cycle
#define MAX_H 42
#define MIN_H 160 // Loops around 
#define MAX_S 255
#define MIN_S 155
#define MAX_V 254
#define MIN_V 0

// Always takes this long for brightness 
// to get to max 
#define CYCLE_LEN 224

#define AT_LEN 50                       // Only brightness changes
#define SUNRISE_LEN 204                 // Num loop iters (constrained by MIN_V+AT_LEN = 50; 254-50 = 204)


// HSV values 
uint8_t StartingColors[][3] = {
  {177, 255, MIN_V}, // Blue
  {187, 255, MIN_V}, // Purple
  {197, 255, MIN_V}, // Pink 
  {227, 255, MIN_V}, // Red 
};

uint8_t sunset_delay[][3] = {
  {84,104,0},
  {94,104,0},
  {104,104,0},
  {134,104,0}
};

void get_starting_colors(uint8_t arr[N_COLORS][3]) { 
  for (int i=0; i<N_COLORS; i++) {
    for (int j=0; j<3; j++) {
      arr[i][j] = StartingColors[i][j];
    }
  }
}

// During civil twilight, just increase brightness, but keep colors desaturated
void civil_twilight(uint8_t arr[N_COLORS][3]) {
  for (int i=0; i<N_COLORS; i++) {
    arr[i][V] += 1; 
  }
}

bool sunrise(uint8_t arr[N_COLORS][3]) {
  uint8_t *hsv; 
  bool changed = 0;

  for (int row=0; row < N_COLORS; row++) {
      hsv = arr[row]; 

      if (hsv[H] < MAX_H || hsv[H] > MIN_H) { hsv[H] += 1; changed = 1;}
      if (hsv[S] > MIN_S) { hsv[S] -= 1; changed = 1;} 
      if (hsv[V] < MAX_V) {hsv[V] += 1; changed = 1;}
  }

  return changed; 
}

bool sunset(uint8_t arr[N_COLORS][3], int i) {
  uint8_t *hsv; 

  for (int row=0; row < N_COLORS; row++) {
    hsv = arr[row]; 

    if (i > sunset_delay[row][H]) { hsv[H] -= 1; }
    if (i > sunset_delay[row][S]) { hsv[S] += 1; } 
    hsv[V] -= 1; // No delay 
  }

  return i != SUNRISE_LEN;
}

void civil_sunset(uint8_t arr[N_COLORS][3]) {
  for (int i=0; i<N_COLORS; i++) {
    arr[i][V] -= 1; 
  }
}

// Incrementors 
bool increment_civil_sr(int i, uint8_t color_arr[N_COLORS][3]) {
  if (i == AT_LEN) { return true; }
  civil_twilight(color_arr);
  return false; 
}

bool increment_sunrise(int i, uint8_t color_arr[N_COLORS][3]) {
  bool changed = sunrise(color_arr);
  return !changed; 
}

bool increment_sunset(int i, uint8_t color_arr[N_COLORS][3]) {
  bool changed = sunset(color_arr, i);
  return !changed; 
}

bool increment_civil_ss(int i, uint8_t color_arr[N_COLORS][3]) {
  if (color_arr[0][V] == 0) { return true; }
  civil_sunset(color_arr);
  return false; 
}