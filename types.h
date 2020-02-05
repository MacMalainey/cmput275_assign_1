#include <Arduino.h> // Included for type definitions

typedef struct {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
} restaurant;

typedef struct {
  int16_t joyX;
  int16_t joyY;
  bool joyButton;
  int16_t touchX;
  int16_t touchY;
  bool isTouch;
} controlInput;

typedef struct {
  uint16_t index ; // index of restaurant from 0 to NUM_RESTAURANTS -1
  uint16_t dist ; // Manhatten distance to cursor position
} restDist;

typedef struct {
  int x;
  int y;
} cursor;

typedef struct {
  int x;
  int y;
} mapCord;

enum mapState {
  MODE0,
  MODE1
};