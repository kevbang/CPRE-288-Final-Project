#ifndef MOVEMENT_H_
#define MOVEMENT_H_

#include <stdbool.h>
#include "open_interface.h"

bool check_hazards(oi_t* sensor_data);

void move_forward(oi_t* sensor_data, double distance_mm);
void move_backwards(oi_t* sensor_data, double distance_mm);
void turn(oi_t* sensor_data, double degrees);

#endif /* MOVEMENT_H_ */
