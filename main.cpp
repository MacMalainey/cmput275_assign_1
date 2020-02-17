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

const uint8_t MENU_DISPLAY_SIZE = 21;

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
  tft.setTextSize(2);

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
  input.joyX = -1 * (analogRead(JOY_HORIZ) - JOY_CENTER);  // Input is reversed
  input.joyY = analogRead(JOY_VERT) - JOY_CENTER;
  input.joyButton = digitalRead(JOY_SEL);

  // Input flags
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
 * Calculate the acceleration of the joystick
 */

inline float calculate_acceleration(int joystick_value) {
  // constant for the curve for the joystick's acceleration function.
  const float curve_constant = 1.2;

  float normalized = 0.0;

  if (joystick_value < 0) {
    normalized = (float)(joystick_value + JOY_DEADZONE) / JOY_CENTER;
  } else {
    normalized = (float)(joystick_value - JOY_DEADZONE) / JOY_CENTER;
  }

  return 9 * sin(curve_constant * normalized);
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
 *
 * @return usable size of sorted array
 */
uint16_t generateRestaurantList(cord center, restDist* distanceArray, uint8_t minRating,
                                void (*sort_fn)(restDist*, uint16_t)) {
  uint16_t arrayIndex = 0;

  for (uint16_t i = 0; i < NUM_RESTAURANTS; i++) {
    restaurant currentRestaurant;
    getRestaurant(i, &currentRestaurant);
    auto distance = calculateManhattan(&currentRestaurant, center);
    if (max((currentRestaurant.rating + 1) / 2, 1) >= minRating) {
      distanceArray[arrayIndex] = {i, distance};
      arrayIndex++;
    }
  }

  sort_fn(distanceArray, arrayIndex);
  return arrayIndex;
}

/**
 * Draws the restaurant list menu
 *
 * @param restaurantArray: The array to draw out.
 * @param selectedIndex: Index of the selected restaurant
 * @param isUpdate: If it's an update, use a faster routine.
 */
void drawRestaurantList(restDist* restaurantArray, uint8_t selectedIndex, uint8_t menuPage, bool isUpdate,
                        uint16_t arraySize) {
  selectedIndex %= MENU_DISPLAY_SIZE;

  if (!isUpdate) {
    tft.fillScreen(TFT_BLACK);
  }

  tft.setTextWrap(false);

  for (auto i = 0; i < MENU_DISPLAY_SIZE; i++) {
    uint16_t restaurantIndex = (menuPage * MENU_DISPLAY_SIZE) + i;
    restaurant currentRestaurant;
    getRestaurant(restaurantArray[restaurantIndex].index, &currentRestaurant);

    tft.setCursor(0, i * 15);

    if (i == selectedIndex) {
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
      tft.print(currentRestaurant.name);
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      if (isUpdate) {
        if (i == selectedIndex - 1 || i == selectedIndex + 1) {
          tft.print(currentRestaurant.name);
        } else {
          tft.print("");
        }
      } else {
        tft.print(currentRestaurant.name);
      }
    }
  }
  // Clean up after yourself to be nice
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextWrap(true);
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
void drawRestaurantPoints(int xLower, int yLower, int xUpper, int yUpper, uint8_t minRate) {
  for (int i = 0; i < NUM_RESTAURANTS; i++) {
    restaurant rest;
    getRestaurant(i, &rest);

    int x = lon_to_x(rest.lon);
    int y = lat_to_y(rest.lat);

    if ((x >= xLower && x <= xUpper) && (y >= yLower && y <= yUpper) && max((rest.rating + 1) / 2, 1) >= minRate) {
      tft.fillCircle(x - xLower, y - yLower, 4, TFT_PURPLE);
    }
  }
}

void drawButton(int x, int y, const char* text) {
  const int BORDER = 2;  // BUTTON COLOUR BORDER
  const int Y_DISPLACE = 16;
  uint8_t length = strlen(text);
  tft.fillRect(x, y, DISPLAY_WIDTH - MAP_DISP_WIDTH, (DISPLAY_HEIGHT / 2), TFT_BLUE);
  tft.fillRect(x + BORDER, y + BORDER, DISPLAY_WIDTH - MAP_DISP_WIDTH - (2 * BORDER),
               (DISPLAY_HEIGHT / 2) - (2 * BORDER), TFT_WHITE);

  tft.setTextColor(TFT_BLACK);

  for (uint8_t i = 0; i < length; i++) {
    tft.setCursor(x + (DISPLAY_WIDTH - MAP_DISP_WIDTH) / 2 - 3,
                  y + (Y_DISPLACE * i) + (DISPLAY_HEIGHT / 4) - ((length * 16) / 2));
    tft.print(text[i]);
  }

  // Reset text size and Colour
  tft.setTextColor(TFT_WHITE);
}

int main() {
  setup();

  // Some constants that are only for this section
  const char QSORT_TEXT[] = "QSORT";
  const char ISORT_TEXT[] = "ISORT";
  const char BSORT_TEXT[] = "BOTH";

  // Init variables
  controlInput input;
  cord curs;
  cord map;
  mapState state = MODE0;
  restDist restaurantDistances[NUM_RESTAURANTS];
  sortMode sort = QSORT;
  uint8_t minRating = 1;
  int16_t listSelected = 0;
  uint16_t oldPage = 0;
  uint16_t ratedArraySize = 0;

  // initial cursor position is the middle of the screen
  curs.x = MAP_DISP_WIDTH / 2;
  curs.y = MAP_DISP_HEIGHT / 2;

  // Draws the centre of the Edmonton map, leaving the rightmost 60 columns black
  map.x = YEG_SIZE / 2 - MAP_DISP_WIDTH / 2;
  map.y = YEG_SIZE / 2 - MAP_DISP_HEIGHT / 2;

  // Draw initial stuff on map
  lcd_image_draw(&yegImage, &tft, map.x, map.y, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
  drawButton(MAP_DISP_WIDTH, 0, QSORT_TEXT);
  drawButton(MAP_DISP_WIDTH, DISPLAY_HEIGHT / 2, "1 STARS");
  drawCursor(curs.x, curs.y, TFT_RED);

  while (true) {
    // Process joystick input
    input = recordInput();
    cord nCurs = processJoystick(input, curs);

    switch (state) {
      case MODE0:
        // Handle all touch cases
        if (input.isTouch) {
          if ((input.touchX > 0 && input.touchX < MAP_DISP_WIDTH) &&
              (input.touchY > 0 && input.touchY < MAP_DISP_HEIGHT)) {
            // Handle if touched the map
            drawRestaurantPoints(map.x, map.y, map.x + MAP_DISP_WIDTH, map.y + MAP_DISP_HEIGHT, minRating);
          } else if ((input.touchX < DISPLAY_WIDTH && input.touchX > MAP_DISP_WIDTH) &&
                     (input.touchY > 0 && input.touchY < DISPLAY_HEIGHT / 2)) {
            // Handle if touched the sort button
            sort = (sortMode)((sort + 1) % 3);
            switch (sort) {
              case QSORT:
                drawButton(MAP_DISP_WIDTH, 0, QSORT_TEXT);
                break;
              case ISORT:
                drawButton(MAP_DISP_WIDTH, 0, ISORT_TEXT);
                break;
              case BOTH:
                drawButton(MAP_DISP_WIDTH, 0, BSORT_TEXT);
                break;
            }
          } else if ((input.touchX < DISPLAY_WIDTH && input.touchX > MAP_DISP_WIDTH) &&
                     (input.touchY > DISPLAY_HEIGHT / 2 && input.touchY < DISPLAY_HEIGHT)) {
            // Handle if touched the rating button
            minRating = (minRating % 5) + 1;
            char msg[] = "0 STARS";
            msg[0] += minRating;
            drawButton(MAP_DISP_WIDTH, DISPLAY_HEIGHT / 2, msg);
          }
        }

        if (curs.x != nCurs.x || curs.y != nCurs.y) {
          // Check for map movements
          bool didMove = moveMap(thresholdSign(nCurs.x, CURSOR_SIZE / 2, MAX_CURSOR_X),
                                 thresholdSign(nCurs.y, CURSOR_SIZE / 2, MAX_CURSOR_Y), map);
          // Redraw entire map if it has moved
          if (didMove) {
            // The redraw helper function isn't set up for non-square re-draws
            lcd_image_draw(&yegImage, &tft, map.x, map.y, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
            // Move cursor to center of screen
            curs.x = MAP_DISP_WIDTH / 2;
            curs.y = MAP_DISP_HEIGHT / 2;
          } else {
            // Redraw previous cursor placement
            lcd_image_draw(&yegImage, &tft, map.x + curs.x - CURSOR_SIZE / 2, map.y + curs.y - CURSOR_SIZE / 2,
                           curs.x - CURSOR_SIZE / 2, curs.y - CURSOR_SIZE / 2, CURSOR_SIZE, CURSOR_SIZE);
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
          auto time = millis();
          switch (sort) {
            case QSORT:
              ratedArraySize = generateRestaurantList(realLocation, restaurantDistances, minRating, quicksort);
              time = millis() - time;
              Serial.print(QSORT_TEXT);
              break;
            case ISORT:
              ratedArraySize = generateRestaurantList(realLocation, restaurantDistances, minRating, isort);
              time = millis() - time;
              Serial.print(ISORT_TEXT);
              break;
            case BOTH:
              generateRestaurantList(realLocation, restaurantDistances, minRating, quicksort);

              time = millis() - time;
              Serial.print(QSORT_TEXT);
              Serial.print(" running time: ");
              Serial.print(time);
              Serial.println(" ms");

              time = millis();
              ratedArraySize = generateRestaurantList(realLocation, restaurantDistances, minRating, isort);
              time = millis() - time;
              Serial.print(ISORT_TEXT);
              break;
          }
          Serial.print(" running time: ");
          Serial.print(time);
          Serial.println(" ms");

          drawRestaurantList(restaurantDistances, 0, 0, false, ratedArraySize);

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
          listSelected = constrain(listSelected, 0, (NUM_RESTAURANTS - 1));
          uint16_t newPage = listSelected / MENU_DISPLAY_SIZE;
          drawRestaurantList(restaurantDistances, listSelected, newPage, oldPage == newPage, ratedArraySize);
          oldPage = newPage;
        }

        if (input.joyButton == false) {
          // Switch to mode0
          restaurant targetRestaurant;
          getRestaurant(restaurantDistances[listSelected].index, &targetRestaurant);

          // Constrain lat and lon
          targetRestaurant.lon = constrain(targetRestaurant.lon, LON_WEST, LON_EAST);
          targetRestaurant.lat = constrain(targetRestaurant.lat, LAT_SOUTH, LAT_NORTH);
          // Set the right distance by converting from lat to lon and then finding the center
          cord restaurantCoord = {lon_to_x(targetRestaurant.lon), lat_to_y(targetRestaurant.lat)};

          map.x = constrain(restaurantCoord.x - (MAP_DISP_WIDTH / 2), 0, YEG_SIZE - MAP_DISP_WIDTH);
          map.y = constrain(restaurantCoord.y - (MAP_DISP_HEIGHT / 2), 0, YEG_SIZE - MAP_DISP_HEIGHT);

          if (map.x <= 0 || map.x >= (YEG_SIZE - MAP_DISP_WIDTH - 1)) {
            // L or R edge of map
            curs.x = restaurantCoord.x - map.x;
          } else {
            curs.x = MAP_DISP_WIDTH / 2;
          }

          if (map.y <= 0 || map.y >= (YEG_SIZE - MAP_DISP_HEIGHT - 1)) {
            // top or bottom edge of map
            curs.y = restaurantCoord.y - map.y;
          } else {
            curs.y = MAP_DISP_HEIGHT / 2;
          }

          // Constrain cursors to screen
          curs.x = constrain(curs.x, CURSOR_SIZE / 2, MAX_CURSOR_X);
          curs.y = constrain(curs.y, CURSOR_SIZE / 2, MAX_CURSOR_Y);

          // Reset the restaurant list.
          listSelected = 0;
          oldPage = 0;

          // Clear screen and draw map and buttons before switching to mode 0
          tft.fillScreen(TFT_BLACK);
          lcd_image_draw(&yegImage, &tft, map.x, map.y, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
          drawCursor(curs.x, curs.y, TFT_RED);
          switch (sort) {
            case QSORT:
              drawButton(MAP_DISP_WIDTH, 0, QSORT_TEXT);
              break;
            case ISORT:
              drawButton(MAP_DISP_WIDTH, 0, ISORT_TEXT);
              break;
            case BOTH:
              drawButton(MAP_DISP_WIDTH, 0, BSORT_TEXT);
              break;
          }
          char msg[] = "0 STARS";
          msg[0] += minRating;
          drawButton(MAP_DISP_WIDTH, DISPLAY_HEIGHT / 2, msg);
          state = MODE0;
        }
        break;
    }
  }

  Serial.end();
  return 0;
}