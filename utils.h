#pragma once

#include <Arduino.h>

#include "config.h"
#include "types.h"

/* Provided lattitude and longitude conversion functions */
int32_t x_to_lon(int16_t x) { return map(x, 0, YEG_SIZE, LON_WEST, LON_EAST); }
int32_t y_to_lon(int16_t y) { return map(y, 0, YEG_SIZE, LAT_NORTH, LAT_SOUTH); }
int16_t lon_to_x(int32_t lon) { return map(lon, LON_WEST, LON_EAST, 0, YEG_SIZE); }
int16_t lat_to_y(int32_t lat) { return map(lat, LAT_NORTH, LAT_SOUTH, 0, YEG_SIZE); }

/**
 * Calculates the manhattan distance between two points
 *
 * @param restaurantInfo: The restaurant to calculate the distance to.
 * @param center; The center of the screen right now.
 * @return manhattan distance calculated.
 */
uint16_t calculateManhattan(restaurant* restaurantInfo, cord center) {
  return abs(center.x - lon_to_x(restaurantInfo->lon)) + abs(center.y - lat_to_y(restaurantInfo->lat));
}

/**
 * Description:
 * Returns the direction (sign) of an integer that is outside of a given boundary
 * EX: The args 3, 1, 2 would return +1 because (3) > 2; 1, 0, 2, would return 0 because 0 < 1 < 2
 * https://stackoverflow.com/questions/14579920/fast-sign-of-integer-in-c
 *
 * Arguments:
 * x (int): Number to check if within boundaries
 * min (int): Minimum boundary
 * max (int): Maximum boundary
 *
 * Returns:
 * sign (int): Direction of x if x is outside of bounds (otherwise returns 0)
 * 
 */
int thresholdSign(int x, int min, int max) { return (x > max) - (x < min); }

/**
 * Swaps 2 restDist pointers
 */
void swap(restDist& a, restDist& b) {
  auto temp = a;
  a = b;
  b = temp;
}

/**
 * Sorts a restDist array of length `length` using insertion sort.
 */
void isort(restDist* array, uint16_t length) {
  uint16_t i = 1;
  uint16_t j;
  while (i < length) {
    j = i;
    while (j > 0 && array[j - 1].dist > array[j].dist) {
      swap(array[j], array[j - 1]);
      j--;
    }
    i++;
  }
}