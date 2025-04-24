#ifndef MOVEMENT_H_
#define MOVEMENT_H_

#include "open_interface.h"

// Moves the robot forward for a set distance in mm (stops on bump)
double move_forward(oi_t *sensor, double distance_mm);

// Moves the robot backward for a set distance in mm (stops on bump)
double move_backwards(oi_t *sensor, double distance_mm);

// Turns the robot by the specified degrees (positive = left, negative = right)
double turn(oi_t *sensor, double degrees);

#endif /* MOVEMENT_H_ */
