#pragma once

#include <stdint.h>  // Included for type definitions

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating;  // from 0 to 10
  char name[55];
};

struct controlInput {
  int16_t joyX;
  int16_t joyY;
  bool joyXMoved;
  bool joyYMoved;
  bool joyButton;
  int16_t touchX;
  int16_t touchY;
  bool isTouch;
};

struct restDist {
  uint16_t index;  // index of restaurant from 0 to NUM_RESTAURANTS -1
  uint16_t dist;   // Manhattan distance to cursor position
};

struct cord {
  int x;
  int y;
};

enum mapState { MODE0, MODE1 };
enum sortMode { QSORT, ISORT, BOTH };