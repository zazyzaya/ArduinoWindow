#include <stdio.h>
#include <FastLED.h> 

#define N_COLORS 4
#define H 0
#define S 1
#define V 2

// HSV values 
uint8_t StartingColors[][3] = {
  {177, 255, 30}, // Blue
  {187, 255, 30}, // Purple
  {197, 255, 30}, // Pink 
  {227, 255, 30}, // Red 
};

void get_starting_colors(uint8_t arr[N_COLORS][3]) { 
  for (int i=0; i<N_COLORS; i++) {
    for (int j=0; j<3; j++) {
      arr[i][j] = StartingColors[i][j];
    }
  }
}