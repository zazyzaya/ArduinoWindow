#include <stdio.h>
#include <FastLED.h> 

#define N_COLORS 4
#define H 0
#define S 1
#define V 2

// Constrain sunrise cycle
#define MAX_H 42
#define MIN_H 160 // Loops around 
#define MIN_S 155
#define MAX_V 254
#define MIN_V 10

// Always takes this long for brightness 
// to get to max 
#define CYCLE_LEN 224

#define AT_LEN 40                       // Only brightness changes
#define SR_LEN 100                      // All colors change
#define DAY_UPDATES 255-SR_LEN-AT_LEN   // Only brightness changes


// HSV values 
uint8_t StartingColors[][3] = {
  {177, 255, MIN_V}, // Blue
  {187, 255, MIN_V}, // Purple
  {197, 255, MIN_V}, // Pink 
  {227, 255, MIN_V}, // Red 
};

void get_starting_colors(uint8_t arr[N_COLORS][3]) { 
  for (int i=0; i<N_COLORS; i++) {
    for (int j=0; j<3; j++) {
      arr[i][j] = StartingColors[i][j];
    }
  }
}

// During astro twilight, just increase brightness, but keep colors desaturated
void astronomical_twilight(uint8_t arr[N_COLORS][3]) {
  for (int i=0; i<N_COLORS; i++) {
    arr[i][V] += 1; 
  }
}