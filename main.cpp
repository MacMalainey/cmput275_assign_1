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
#include "lcd_image.h"
#include "utils.h"

// More defines (DO NOT SET THESE LIKE THOSE IN THE CONFIG HEADER)

// Maximum cursor positions on screen
#define MAX_CURSOR_X (DISPLAY_WIDTH - 60) - (CURSOR_SIZE / 2 + 1)
#define MAX_CURSOR_Y DISPLAY_HEIGHT - (CURSOR_SIZE / 2 + 1)

// Dimensions of the part allocated to the map display
#define MAP_DISP_WIDTH  (DISPLAY_WIDTH - 60)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

MCUFRIEND_kbv tft;

const lcd_image_t yegImage = {"yeg-big.lcd", YEG_SIZE, YEG_SIZE};

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Sd2Card card;

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
    while (true) {
    }
  } else {
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

/**
 * Description:
 * Reads from all device inputs
 *
 * Returns:
 * input (controlInput): all device inputs
 */
controlInput recordInput() {
  controlInput input;

  // Read joystick input
  input.joyX = analogRead(JOY_HORIZ) - JOY_CENTER;
  input.joyY = analogRead(JOY_VERT) - JOY_CENTER;
  input.joyButton = digitalRead(JOY_SEL);
  input.joyXMoved = true;
  input.joyYMoved = true;

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

  if (input.joyY > -JOY_DEADZONE && input.joyY < JOY_DEADZONE) {
    input.joyY = 0;
    input.joyYMoved = false;
  }

  if (input.joyX > -JOY_DEADZONE && input.joyX < JOY_DEADZONE) {
    input.joyX = 0;
    input.joyXMoved = false;
  }

  // Process joystick input. x and y will be 0 when inside deadzone, value outside.
  return input;
}

/**
 * Description:
 * Draws the cursor on screen at the given cordinates and color
 */
void drawCursor(int x, int y, uint16_t colour) {
  tft.fillRect(x - CURSOR_SIZE / 2, y - CURSOR_SIZE / 2, CURSOR_SIZE, CURSOR_SIZE, colour);
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

inline float calculate_acceleration(int joystick_value) {
  // constant for the curve for the joystick's acceleration function.
  const float curve_constant = 1.2;

  float normalized = 0.0;

  if (joystick_value < 0) {
    normalized = (float)(joystick_value + JOY_DEADZONE) / JOY_CENTER;
  } else {
    normalized = (float)(joystick_value - JOY_DEADZONE) / JOY_CENTER;
  }

  return 8 * sin(curve_constant * normalized);
}

/**
 * Description:
 * Creates a set of cordinates based off of another set of cordinates and the given input.
 * Assumes the given input are raw joystick reads.
 *
 * Arguments:
 * x (int): Raw horizontal joystick input
 * y (int): Raw vertical joystick input
 * last (cord): Cordinate to base translation off of
 *
 * Returns:
 * newCord (cord): Translated cordinate measurments based off of "last"
 */
cord processJoystick(const controlInput& joystick, const cord& last) {
  cord mapped;

  mapped.y = last.y;
  mapped.x = last.x;

  if (joystick.joyYMoved) {
    mapped.y += calculate_acceleration(joystick.joyY);
  }
  if (joystick.joyXMoved) {
    mapped.x += calculate_acceleration(joystick.joyX);
  }

  return mapped;
}

/**
 * Description:
 * Moves a set of cords based on the direction of the signs of the input
 * Assumes the cord represents the placement of the map (translates them one map segment length)
 *
 * Arguments:
 * deltaX (int): change in x value on map
 * deltaY (int): change in y value on map
 * &cords (cord): cords object to change
 *
 * Returns:
 * mapHasMoved (bool): variable telling if the map has changed location or not
 */
bool moveMap(int deltaX, int deltaY, cord& cords) {
  int nx = constrain(cords.x + (deltaX)*MAP_DISP_WIDTH, 0, YEG_SIZE - MAP_DISP_WIDTH);
  int ny = constrain(cords.y + (deltaY)*MAP_DISP_HEIGHT, 0, YEG_SIZE - MAP_DISP_HEIGHT);

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
  static restaurant restBlock[8];
  static uint32_t previousBlockNum = 0;

  if (blockNum != previousBlockNum) {
    while (!card.readBlock(blockNum, (uint8_t*)restBlock)) {
      Serial.println("Read block failed, trying again.");
    }
    previousBlockNum = blockNum;
  }

  *restPtr = restBlock[restIndex % 8];
}

/**
 * Generates and sorts the list of restaurants
 *
 * @param center: The center of the screen at the moment
 * @param distanceArray: the array to store the results at.
 */
void generateRestaurantList(cord center, restDist* distanceArray) {
  for (auto i = 0; i < NUM_RESTAURANTS; i++) {
    restaurant currentRestaurant;
    getRestaurant(i, &currentRestaurant);
    auto distance = calculateManhattan(&currentRestaurant, center);
    distanceArray[i] = {i, distance};
  }

  isort(distanceArray, NUM_RESTAURANTS);
}

/**
 * Draws the restaurant list menu
 *
 * @param restaurantArray: The array to draw out.
 * @param selectedIndex: Index of the selected restaurant
 * @param isUpdate: If it's an update, use a faster routine.
 */
void drawRestaurantList(restDist* restaurantArray, int8_t selectedIndex, bool isUpdate) {
  const uint8_t fontSize = 2;

  tft.setTextSize(fontSize);
  tft.setCursor(0, 0);
  tft.setTextWrap(false);

  for (auto i = 0; i < MENU_LIST_SIZE; i++) {
    restaurant currentRestaurant;
    getRestaurant(restaurantArray[i].index, &currentRestaurant);

    if (isUpdate) {
      if (i == selectedIndex) {
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.println(currentRestaurant.name);
      } else if (i == selectedIndex - 1 || i == selectedIndex + 1) {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.println(currentRestaurant.name);
      } else {
        tft.println();
      }
    } else {
      if (i == selectedIndex) {
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
      } else {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }
      tft.println(currentRestaurant.name);
    }
  }
}

/**
 * Description:
 * Draw any restaurants that are in the given bounds on the TFT display
 *
 * Arguments:
 * xLower (int): Horizontal map cord lower bound
 * yLower (int): Vertical map cord lower bound
 * xUpper (int): Horizontal map cord upper bound
 * yUpper (int): Vertical map cord upper bound
 */
void drawRestaurantPoints(int xLower, int yLower, int xUpper, int yUpper) {
  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    restaurant rest;
    getRestaurant(i, &rest);

    int x = lon_to_x(rest.lon);
    int y = lat_to_y(rest.lat);

    if ((x >= xLower && x <= xUpper) && y >= yLower && y <= yUpper) {
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
  restDist restaurantDistances[NUM_RESTAURANTS];
  int8_t listSelected = 0;

  // initial cursor position is the middle of the screen
  curs.x = MAP_DISP_WIDTH / 2;
  curs.y = MAP_DISP_HEIGHT / 2;

  // Draws the centre of the Edmonton map, leaving the rightmost 60 columns black
  map.x = YEG_SIZE / 2 - MAP_DISP_WIDTH / 2;
  map.y = YEG_SIZE / 2 - MAP_DISP_HEIGHT / 2;

  lcd_image_draw(&yegImage, &tft, map.x, map.y, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
  drawCursor(curs.x, curs.y, TFT_RED);

  while (true) {
    // Process joystick input
    input = recordInput();
    cord nCurs = processJoystick(input, curs);

    switch (state) {
      case MODE0:
        // Process touch input
        if (input.isTouch && ((input.touchX > 0 && input.touchX < MAP_DISP_WIDTH) &&
                              (input.touchY > 0 && input.touchY < MAP_DISP_HEIGHT))) {
          drawRestaurantPoints(map.x, map.y, map.x + MAP_DISP_WIDTH, map.y + MAP_DISP_HEIGHT);
        }

        if (curs.x != nCurs.x || curs.y != nCurs.y) {
          // Check for map movements
          bool didMove = moveMap(thresholdSign(nCurs.x, CURSOR_SIZE / 2, MAX_CURSOR_X),
                                 thresholdSign(nCurs.y, CURSOR_SIZE / 2, MAX_CURSOR_Y), map);
          // Redraw entire map if it has moved
          if (didMove) {
            // The redraw helper function isn't set up for non-square re-draws
            lcd_image_draw(&yegImage, &tft, map.x, map.y, 0, 0, DISPLAY_WIDTH - 60, MAP_DISP_HEIGHT);
            // Move cursor to center of screen
            curs.x = MAP_DISP_WIDTH / 2;
            curs.y = MAP_DISP_HEIGHT / 2;
          } else {
            // TODO: we might want to just set this up as an lcd_image_draw function (Code size)
            // Redraw previous cursor placement
            redrawImage(map.x, map.y, curs.x - CURSOR_SIZE / 2, curs.y - CURSOR_SIZE / 2, CURSOR_SIZE);
            curs = nCurs;
            // Make certain the cursor doesn't move off screen
            curs.y = constrain(curs.y, CURSOR_SIZE / 2, MAX_CURSOR_Y);
            curs.x = constrain(curs.x, CURSOR_SIZE / 2, MAX_CURSOR_X);
          }
          drawCursor(curs.x, curs.y, TFT_RED);
        }
        if (input.joyButton == false) {
          // clear screen
          tft.fillScreen(TFT_BLACK);

          // Real location = map + local coord
          cord realLocation = {curs.x + map.x, curs.y + map.y};
          generateRestaurantList(realLocation, restaurantDistances);
          drawRestaurantList(restaurantDistances, 0, false);
          state = MODE1;
        }
        break;
      case MODE1:
        if (input.joyYMoved) {
          // Update only when joyY used
          if (input.joyY < 0) {
            // Go up list
            listSelected--;
          } else if (input.joyY > 0) {
            // Go down list
            listSelected++;
          }
          listSelected = constrain(listSelected, 0, MENU_LIST_SIZE);
          drawRestaurantList(restaurantDistances, listSelected, true);
        }
        if (input.joyButton == false) {
          // Switch to mode0

          restaurant targetRestaurant;

          getRestaurant(restaurantDistances[listSelected].index, &targetRestaurant);

          // Set the right distance by converting from lat to lon and then finding the center
          map.y = lat_to_y(targetRestaurant.lat) - (MAP_DISP_HEIGHT / 2);
          map.x = lon_to_x(targetRestaurant.lon) - (MAP_DISP_WIDTH / 2);

          // Set cursor to center
          curs.x = MAP_DISP_WIDTH / 2;
          curs.y = MAP_DISP_HEIGHT / 2;

          // Clear screen and draw map before switching to mode 0
          tft.fillScreen(TFT_BLACK);
          lcd_image_draw(&yegImage, &tft, map.x, map.y, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
          drawCursor(curs.x, curs.y, TFT_RED);
          state = MODE0;
        }
        break;
    }
  }

  Serial.end();
  return 0;
}