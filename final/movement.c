#include "movement.h"
#include "lcd.h"
#include "Timer.h"
#include "uart-interrupt.h"
#include <math.h>

#define FORWARD_TIME_PER_CM 40
#define BACKWARD_TIME_PER_CM 40
#define TURN_TIME_PER_DEGREE 20
#define CLIFF_THRESHOLD 2500

bool check_hazards(oi_t* sensor_data) {
    oi_update(sensor_data);

    // Check for bumps
    if (sensor_data->bumpLeft || sensor_data->bumpRight) {
        oi_setWheels(0, 0);
        uart_sendStr("\r\nObject bumped! Backing up 5 cm...\r\n");

        // Backup without calling move_backwards (avoid recursion)
        oi_setWheels(-200, -200);
        timer_waitMillis(200);  // ~5cm at -200 mm/s (adjust as needed)
        oi_setWheels(0, 0);

        return true;
    }

    // Check for cliffs or black tape
    if (sensor_data->cliffLeftSignal > CLIFF_THRESHOLD ||
        sensor_data->cliffFrontLeftSignal > CLIFF_THRESHOLD ||
        sensor_data->cliffFrontRightSignal > CLIFF_THRESHOLD ||
        sensor_data->cliffRightSignal > CLIFF_THRESHOLD) {

        oi_setWheels(0, 0);
        uart_sendStr("\r\nCliff or black tape detected! Emergency Stop.\r\n");
        return true;
    }

    return false; // No hazard
}

void move_forward(oi_t* sensor_data, double distance_mm) {
    if (check_hazards(sensor_data)) return;

    oi_setWheels(200, 200);
    int time_ms = (int)(distance_mm / 10.0 * FORWARD_TIME_PER_CM);
    timer_waitMillis(time_ms);
    oi_setWheels(0, 0);
}

void move_backwards(oi_t* sensor_data, double distance_mm) {
    if (check_hazards(sensor_data)) return;

    oi_setWheels(-200, -200);
    int time_ms = (int)(distance_mm / 10.0 * BACKWARD_TIME_PER_CM);
    timer_waitMillis(time_ms);
    oi_setWheels(0, 0);
}

void turn(oi_t* sensor_data, double degrees) {
    if (check_hazards(sensor_data)) return;

    if (degrees > 0) {
        oi_setWheels(200, -200);
    } else {
        oi_setWheels(-200, 200);
    }

    int time_ms = (int)(fabs(degrees) * TURN_TIME_PER_DEGREE);
    timer_waitMillis(time_ms);
    oi_setWheels(0, 0);
}
