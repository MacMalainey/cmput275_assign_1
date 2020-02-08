/**
 * Mackenzie Malainey, 1570494
 * Michael Kwok, 1548454
 * CMPUT 275 Winter 2020
 * Assignment #1
 */

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <MCUFRIEND_kbv.h>
#include <SD.h>
#include <SPI.h>
#include <TouchScreen.h>

// Project includes
#include "config.h"
#include "types.h"
#include "lcd_image.h"

// More defines (DO NOT SET THESE LIKE THOSE IN THE CONFIG HEADER)

// Maximum cursor positions on screen
#define MAX_CURSOR_X (DISPLAY_WIDTH - 60) - (CURSOR_SIZE/2 + 1)
#define MAX_CURSOR_Y DISPLAY_HEIGHT - (CURSOR_SIZE/2 + 1)

// Dimensions of the part allocated to the map display
#define MAP_DISP_WIDTH (DISPLAY_WIDTH - 60)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

MCUFRIEND_kbv tft;

const lcd_image_t yegImage = {"yeg-big.lcd", YEG_SIZE, YEG_SIZE};

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Sd2Card card;

// forward declaration for redrawing the cursor
void redrawCursor(uint16_t colour);

void setup() {
  init();

  Serial.begin(9600);

  pinMode(JOY_SEL, INPUT_PULLUP);

  //    tft.reset();             // hardware reset
  uint16_t ID = tft.readID();  // read ID from display
  // Serial.print("ID = 0x");
  // Serial.println(ID, HEX);
  // if (ID == 0xD3D3) ID = 0x9481; // write-only shield

  // must come before SD.begin() ...
  tft.begin(ID);  // LCD gets ready to work

  // SD card initialization for raw reads
  Serial.print("Initializing SPI communication for raw reads...");
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.println("failed! Is the card inserted properly?");
    while (true) {}
  }
  else {
    Serial.println("OK!");
  }

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed! Is it inserted properly?");
    while (true) {
    }
  }
  Serial.println("OK!");

  tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);
}

controlInput recordInput() {
  controlInput input;

  // Read joystick input
  input.joyX      = analogRead(JOY_HORIZ);
  input.joyY      = analogRead(JOY_VERT);
  input.joyButton = digitalRead(JOY_SEL);

  TSPoint touch = ts.getPoint();

  // Reset pins from getPoint()
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);

  if (touch.z > MINPRESSURE && touch.z < MAXPRESSURE) {
    input.isTouch = true;

    // We only care about doing the work to properly init the touch vars
    // if we have a confirmed touch
    input.touchX = map(touch.y, TS_MINX, TS_MAXX, 480, 0);
    input.touchY = map(touch.x, TS_MINY, TS_MAXY, 320, 0);
  } else
    input.isTouch = false;

  return input;
}

void drawCursor(int x, int y, uint16_t colour) {
  tft.fillRect(x - CURSOR_SIZE / 2, y - CURSOR_SIZE / 2, CURSOR_SIZE,
               CURSOR_SIZE, colour);
}

/**
 * Summary:
 * Redraws a portion of the background image (in a square)
 *
 * Paramters:
 * x (int): horizontal cordinate to start drawing at
 * y (int): vertcursoral cordinate to start drawing at
 * size (int): length and width of image segment to draw
 */
void redrawImage(int mapX, int mapY, int x, int y, int size) {
  lcd_image_draw(&yegImage, &tft, mapX + x, mapY + y, x, y, size, size);
}

cord processJoystick(int x, int y, cord last) {

  cord mapped;
  mapped.x = last.x;
  mapped.y = last.y;

  // Check if joystick is getting pushed
  if (abs(y - JOY_CENTER) > JOY_DEADZONE) {
    mapped.y +=
        map(y, 0, 1024, -MAX_CURSOR_SPEED,
            MAX_CURSOR_SPEED);  // decrease the y coordinate of the cursor
  }

  // remember the x-reading increases as we push left
  if (abs(x - JOY_CENTER) > JOY_DEADZONE) {
    mapped.x +=
        map(x, 0, 1024, MAX_CURSOR_SPEED,
            -MAX_CURSOR_SPEED);  // decrease the y coordinate of the cursor
  }

  return mapped;
}

int calculateManhattan(restaurant* restaurantInfo, cord center) {
  return 0;
  // TODO: Implement.
}

int thresholdSign(int x, int min, int max) { return (x > max) - (x < min); }
// https://stackoverflow.com/questions/14579920/fast-sign-of-integer-in-c

bool moveMap(int deltaX, int deltaY, cord &cords) {
  int nx = constrain(cords.x + (deltaX) * MAP_DISP_WIDTH,
                     0, YEG_SIZE - MAP_DISP_WIDTH);
  int ny = constrain(cords.y + (deltaY) * MAP_DISP_HEIGHT,
                     0, YEG_SIZE - MAP_DISP_HEIGHT);

  if (nx != cords.x || ny != cords.y) {
    cords.x = nx;
    cords.y = ny;
    return true;
  } else {
    return false;
  }
}

/**
 * Description:
 * Fast implementation of getRestaurant code (with caching)
 *
 * Arguments:
 * restIndex (int): Restaurant index to grab
 * restPtr (restaurant*): Pointer to store restaurant data at
 */
void getRestaurant(int restIndex, restaurant* restPtr) {
  uint32_t blockNum = REST_START_BLOCK + restIndex / 8;

  // Cache values
  static restaurant restBlock[8];
  static uint32_t prevBlockNum = 0;

  if (prevBlockNum != blockNum) {
    prevBlockNum = blockNum;
    while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
      Serial.print("Read block failed, trying again. blockNum = ");
      Serial.println(blockNum);
    }
  }

  *restPtr = restBlock[restIndex % 8];
}

void getRestaurantIndices(restaurant* restaurantArray, cord centre) {
  for (auto i = 0; i < NUM_RESTAURANTS; i++) {
  }
}

void drawRestaurantList(restaurant* restaurantArray, uint8_t selectedIndex) {
  const uint8_t listSize = 21;
  const uint8_t fontSize = 2;
}

int16_t lon_to_x (int32_t lon) {
  return map(lon, LON_WEST, LON_EAST, 0, MAP_WIDTH);
}

int16_t lat_to_y (int32_t lat) {
  return map(lat, LAT_NORTH, LAT_SOUTH, 0, MAP_HEIGHT);
}

void drawRestaurantPoints(int xLower, int yLower, int xUpper, int yUpper) {

  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    restaurant rest;
    getRestaurant(i, &rest);

    int x = lon_to_x(rest.lon);
    int y = lat_to_y(rest.lat);

    if ((x >= xLower && x <= xUpper) &&
         y >= yLower && y <= yUpper) {
      tft.fillCircle(x - xLower, y - yLower, 4, TFT_PURPLE);
    }
  }
}

int main() {
  setup();

  // Init variables
  controlInput input;
  cord curs;
  cord map;
  mapState state = MODE0;

  // initial cursor position is the middle of the screen
  curs.x = MAP_DISP_WIDTH/2;
  curs.y = MAP_DISP_HEIGHT/2;

  // Draws the centre of the Edmonton map, leaving the rightmost 60 columns black
  map.x = YEG_SIZE/2 - MAP_DISP_WIDTH/2;
	map.y = YEG_SIZE/2 - MAP_DISP_HEIGHT/2;
	lcd_image_draw(&yegImage, &tft, map.x, map.y,
                 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);

  drawCursor(curs.x, curs.y, TFT_RED);

  while (true) {
    input = recordInput();
    switch (state) {
      case MODE0:

      if (input.isTouch && (
        (input.touchX > 0 && input.touchX < MAP_DISP_WIDTH) &&
        (input.touchY > 0 && input.touchY < MAP_DISP_HEIGHT))) {
          drawRestaurantPoints(map.x, map.y, map.x + MAP_DISP_WIDTH, map.y + MAP_DISP_HEIGHT);
      }

      cord nCurs = processJoystick(input.joyX, input.joyY, curs);

      if (curs.x != nCurs.x || curs.y != nCurs.y) {
        bool didMove = moveMap(thresholdSign(nCurs.x, CURSOR_SIZE/2, MAX_CURSOR_X),
                               thresholdSign(nCurs.y, CURSOR_SIZE/2, MAX_CURSOR_Y),
                               map);
        if (didMove) {
          // The redraw helper function isn't set up for non-rectangular re-draws
          lcd_image_draw(&yegImage, &tft, map.x, map.y,
                 0, 0, DISPLAY_WIDTH - 60, MAP_DISP_HEIGHT);
          curs.x = MAP_DISP_WIDTH/2;
          curs.y = MAP_DISP_HEIGHT/2;
        } else {
          // TODO: we might want to just set this up as an lcd_image_draw function (Code size)
          redrawImage(map.x, map.y, curs.x - CURSOR_SIZE/2, curs.y - CURSOR_SIZE/2, CURSOR_SIZE);
          curs = nCurs;
          curs.y = constrain(curs.y, CURSOR_SIZE/2, MAX_CURSOR_Y);
          curs.x = constrain(curs.x, CURSOR_SIZE/2, MAX_CURSOR_X);
        }
        drawCursor(curs.x, curs.y, TFT_RED);
      }
      break;
      case MODE1:
        // drawRestaurantList(curs.x, curs.y);
        break;
    }
  }

  Serial.end();
  return 0;
}