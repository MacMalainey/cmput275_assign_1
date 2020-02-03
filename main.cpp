/*
	Mackenzie Malainey, 1570494
  CMPUT 275 Winter 2020
  Assignment #1
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <SD.h>

// Project includes
#include "lcd_image.h"
#include "config.h"
#include "types.h"

MCUFRIEND_kbv tft;

const lcd_image_t yegImage = { "yeg-big.lcd", YEG_SIZE, YEG_SIZE };

// a multimeter reading says there are 300 ohms of resistance across the plate,
// so initialize with this to get more accurate readings
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// different than SD
Sd2Card card;

// forward declaration for redrawing the cursor
void redrawCursor(uint16_t colour);

void setup() {
  init();

  Serial.begin(9600);

	pinMode(JOY_SEL, INPUT_PULLUP);

	//    tft.reset();             // hardware reset
  uint16_t ID = tft.readID();    // read ID from display
  // Serial.print("ID = 0x");
  // Serial.println(ID, HEX);
  // if (ID == 0xD3D3) ID = 0x9481; // write-only shield
  
  // must come before SD.begin() ...
  tft.begin(ID);                 // LCD gets ready to work

	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed! Is it inserted properly?");
		while (true) {}
	}
	Serial.println("OK!");

	tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);

  // TODO: Move to another screen

  // redrawCursor(TFT_RED);
}

controlInput recordInput() {

  controlInput input;

  // Read joystick input
  input.joyX = analogRead(JOY_HORIZ);
  input.joyY = analogRead(JOY_VERT);
  input.joyButton = digitalRead(JOY_SEL);

  TSPoint touch = ts.getPoint();

  // Reset pins from getPoint()
  pinMode(YP, OUTPUT); 
  pinMode(XM, OUTPUT);

  if (touch.z > MINPRESSURE && touch.z < MAXPRESSURE)
  {
    input.isTouch = true;

    // We only care about doing the work to properly init the touch vars
    // if we have a confirmed touch
    input.touchX = map(touch.y, TS_MINX, TS_MAXX, 480, 0);
    input.touchY = map(touch.x, TS_MINY, TS_MAXY, 320, 0);
  }
  else input.isTouch = false;

  return input;
}

void redrawCursor(int x, int y, uint16_t colour) {
  tft.fillRect(x - CURSOR_SIZE/2, y - CURSOR_SIZE/2,
               CURSOR_SIZE, CURSOR_SIZE, colour);
}

/**
 * Summary:
 * Redraws a portion of the background image (in a square)
 * 
 * Paramters:
 * x (int): horizontal cordinate to start drawing at
 * y (int): vertical cordinate to start drawing at
 * size (int): length and width of image segment to draw
 */
void redrawImage(int mapX, int mapY, int x, int y, int size) {

  lcd_image_draw(&yegImage, &tft, mapX + x, mapY + y,
                 x, y, size, size);

}

cursor processJoystick(int x, int y, cursor last) {

  cursor mapped;
  mapped.x = last.x;
  mapped.y = last.y;

  // Check if joystick is getting pushed
  if (abs(y - JOY_CENTER) > JOY_DEADZONE) {
    mapped.y += map(y, 0, 1024, -MAX_CURSOR_SPEED, MAX_CURSOR_SPEED); // decrease the y coordinate of the cursor
    mapped.y = constrain(mapped.y, CURSOR_SIZE/2, DISPLAY_HEIGHT - (CURSOR_SIZE/2 + 1));
  }

  // remember the x-reading increases as we push left
  if (abs(x - JOY_CENTER) > JOY_DEADZONE) {
    mapped.x += map(x, 0, 1024, MAX_CURSOR_SPEED, -MAX_CURSOR_SPEED); // decrease the y coordinate of the cursor
    mapped.x = constrain(mapped.x, CURSOR_SIZE/2, DISPLAY_WIDTH - 60 - (CURSOR_SIZE/2 + 1));
  }

  return mapped;
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
  uint32_t blockNum = REST_START_BLOCK + restIndex/8;

  // Cache values
  static restaurant restBlock[8];
  static uint32_t prevBlockNum = 0;

  if (prevBlockNum != blockNum)
  {
    prevBlockNum = blockNum;
    while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
      Serial.println("Read block failed, trying again.");
    }
  }

  *restPtr = restBlock[restIndex % 8];
}



int main() {
	setup();

  // Init variables
  controlInput input;
  cursor curs;
  mapState state = MODE0;

  // initial cursor position is the middle of the screen
  curs.x = (DISPLAY_WIDTH - 60)/2;
  curs.y = DISPLAY_HEIGHT/2;

  // Draws the centre of the Edmonton map, leaving the rightmost 60 columns black
  int yegX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
	int yegY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
	lcd_image_draw(&yegImage, &tft, yegX, yegY,
                 0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

  redrawCursor(curs.x, curs.y, TFT_RED);

  while (true) {

    switch (state) {
      case MODE0:
      input = recordInput();

      cursor nCurs = processJoystick(input.joyX, input.joyY, curs);

      if (curs.x != nCurs.x || curs.y != nCurs.y) {
        redrawImage(yegX, yegY, curs.x - CURSOR_SIZE/2, curs.y - CURSOR_SIZE/2, CURSOR_SIZE);
        curs = nCurs;
      }
      redrawCursor(curs.x, curs.y, TFT_RED);

      break;
      case MODE1:
      break;
    }

  }

	Serial.end();
	return 0;
}