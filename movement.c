#include "movement.h"
#include "open_interface.h"
#include "lcd.h"
#include <math.h>

// Move forward and stop if bump is detected
double move_forward(oi_t *sensor, double distance_mm) {
    double sum = 0;
    oi_setWheels(200, 200); // Set wheel speed forward

    while (sum < distance_mm) {
        oi_update(sensor);
        sum += sensor->distance;

        if (sensor->bumpLeft || sensor->bumpRight) {
            lcd_clear();
            lcd_puts("Object bumped");
            oi_setWheels(0, 0);
            return sum;
        }
    }

    oi_setWheels(0, 0); // Stop the robot
    return sum;
}

// Move backward
double move_backwards(oi_t *sensor, double distance_mm) {
    double sum = 0;
    oi_setWheels(-200, -200); // Set wheel speed backward

    while (sum < distance_mm) {
        oi_update(sensor);
        sum += fabs(sensor->distance);

        if (sensor->bumpLeft || sensor->bumpRight) {
            lcd_clear();
            lcd_puts("Bumped while rev");
            oi_setWheels(0, 0);
            return sum;
        }
    }

    oi_setWheels(0, 0);
    return sum;
}

// General turn function (positive = left, negative = right)
double turn(oi_t *sensor, double degrees) {
    double sum = 0;

    // Calibration factor to prevent over-turning
    double correction = 0.86; // Adjust this as needed through testing
    double corrected_degrees = degrees * correction;

    int left = corrected_degrees > 0 ? 200 : -200;
    int right = -left;

    oi_setWheels(left, right);

    while (fabs(sum) < fabs(corrected_degrees)) {
        oi_update(sensor);
        sum += sensor->angle;
    }

    oi_setWheels(0, 0);
    return sum;
}

