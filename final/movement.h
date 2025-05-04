#ifndef MOVEMENT_H_
#define MOVEMENT_H_

#include "open_interface.h"
#include <stdbool.h>

// Moves the robot forward for a set distance in mm (handles bump and hazards)
double move_forward(oi_t *sensor, double distance_mm);

// Moves the robot forward into a parking spot (no hazard checks)
double park_forward(oi_t *sensor, double distance_mm);

// Moves the robot backward into a parking spot (no hazard checks)
double park_backwards(oi_t *sensor, double distance_mm);

// Moves the robot backward for a set distance in mm (handles hazards)
double move_backwards(oi_t *sensor, double distance_mm);

// Checks for bumpers while parking (returns true if bumped)
bool check_bumpers(oi_t *sensor);

// Turns the robot by specified degrees (positive = left, negative = right)
double turn(oi_t *sensor, double degrees);

// Checks for tape, cliffs, or bump (returns 0 = none, 1 = tape/cliff, 2 = bump)
int check_hazards(oi_t *sensor);

// Checks for hazards while reversing (returns true if hazard detected)
bool check_back_hazards(oi_t *sensor);

#endif /* MOVEMENT_H_ */
