#pragma once

#define SD_CS     10
#define JOY_VERT  A9  // should connect A9 to pin VRx
#define JOY_HORIZ A8  // should connect A8 to pin VRy
#define JOY_SEL   53

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320
#define YEG_SIZE       2048

#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define CURSOR_SIZE 9

#define MAX_CURSOR_SPEED 7

// touch screen pins, obtained from the documentaion
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

// dimensions of the part allocated to the map display
#define MAP_DISP_WIDTH  (DISPLAY_WIDTH - 60)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS  1066

// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 940
#define TS_MAXY 920

// thresholds to determine if there was a touch
#define MINPRESSURE 10
#define MAXPRESSURE 1000

// lat/long conversion constants.
#define LAT_NORTH 5361858l
#define LAT_SOUTH 5340953l
#define LON_WEST  -11368652l
#define LON_EAST  -11333496l