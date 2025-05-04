#ifndef MOVEMENT_H_
#define MOVEMENT_H_

#include "open_interface.h"

// Moves the robot forward for a set distance in mm (stops on bump)
double move_forward(oi_t *sensor, double distance_mm);

// moves the robot forward into a parking spot.
double park_forward(oi_t *sensor, double distance_mm);

// moves the bot backward into a parking spot. used for alignment.
double park_backwards(oi_t *sensor, double distance_mm);

// Moves the robot backward for a set distance in mm (stops on bump)
double move_backwards(oi_t *sensor, double distance_mm);

// checks bumpers while parking
bool check_bumpers(oi_t *sensor);

// Turns the robot by the specified degrees (positive = left, negative = right)
double turn(oi_t *sensor, double degrees);

#endif /* MOVEMENT_H_ */
