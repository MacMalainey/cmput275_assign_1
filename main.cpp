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

void redrawCursor(uint16_t colour) {
  int cursorY;
  int cursorX;
  tft.fillRect(cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2,
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
void redrawImage(int x, int y, int size) {
  int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
	int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;

  lcd_image_draw(&yegImage, &tft, yegMiddleX + x, yegMiddleY + y,
                 x, y, size, size);

}

void processJoystick(int x, int y) {

  int cursorY;
  int cursorX;

  // Check if joystick is getting pushed
  if (abs(y - JOY_CENTER) > JOY_DEADZONE) {
    cursorY += map(y, 0, 1024, -MAX_CURSOR_SPEED, MAX_CURSOR_SPEED); // decrease the y coordinate of the cursor
    cursorY = constrain(cursorY, CURSOR_SIZE/2, DISPLAY_HEIGHT - (CURSOR_SIZE/2 + 1));
  }

  // remember the x-reading increases as we push left
  if (abs(x - JOY_CENTER) > JOY_DEADZONE) {
    cursorX += map(x, 0, 1024, MAX_CURSOR_SPEED, -MAX_CURSOR_SPEED); // decrease the y coordinate of the cursor
    cursorX = constrain(cursorX, CURSOR_SIZE/2, DISPLAY_WIDTH - 60 - (CURSOR_SIZE/2 + 1));
  }

  // Draw a small patch of the Edmonton map at the old cursor position before
  // drawing a red rectangle at the new cursor position
  // if (cursorX != preX || cursorY != preY) redrawImage(preX - CURSOR_SIZE/2, preY - CURSOR_SIZE/2, CURSOR_SIZE);
  redrawCursor(TFT_RED);

  delay(20);
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
  cursor cur;

  // initial cursor position is the middle of the screen
  cur.x = (DISPLAY_WIDTH - 60)/2;
  cur.y = DISPLAY_HEIGHT/2;

  // Draws the centre of the Edmonton map, leaving the rightmost 60 columns black
  int yegX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
	int yegY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
	lcd_image_draw(&yegImage, &tft, yegX, yegY,
                 0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

  input = recordInput();

  while (true) {

  }

	Serial.end();
	return 0;
}