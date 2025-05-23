#include "movement.h"
#include "open_interface.h"
#include "uart-interrupt.h"
#include "lcd.h"
#include <math.h>

// Move forward and stop if bump is detected
double move_forward(oi_t *sensor, double distance_mm) {
    double sum = 0;
    oi_setWheels(200, 200); // Set wheel speed forward

    while (sum < distance_mm) {
        oi_update(sensor);
        sum += sensor->distance;

        if (sensor->bumpLeft) {
            oi_setWheels(0, 0);
            lcd_clear();
            lcd_puts("Object bumped left");
            uart_sendStr("\r\nObject bumped left side\r\n");
            return sum;
        } else if (sensor->bumpRight) {
            oi_setWheels(0, 0);
            lcd_clear();
            lcd_puts("Object bumped right");
            uart_sendStr("\r\nObject bumped right side\r\n");
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

        if (sensor->bumpLeft) {
            oi_setWheels(0, 0);
            lcd_clear();
            lcd_puts("Object bumped left");
            uart_sendStr("\r\nObject bumped back left side\r\n");
            return sum;
        } else if (sensor->bumpRight) {
            oi_setWheels(0, 0);
            lcd_clear();
            lcd_puts("Object bumped right");
            uart_sendStr("\r\nObject bumped back right side\r\n");
            return sum;
        }
    }

    oi_setWheels(0, 0);
    return sum;
}

// General turn function (positive = left, negative = right)
void turn(oi_t *sensor, double degrees) {
    double sum = 0;

    // Apply a correction factor based on testing (adjust as needed)
    double correction = 0.86;  // You might tune this between 0.80 - 0.95
    double target_degrees = degrees * correction;

    int left = target_degrees > 0 ? 200 : -200;
    int right = -left;

    oi_setWheels(left, right);

    while (fabs(sum) < fabs(target_degrees)) {
        oi_update(sensor);
        sum += sensor->angle;
    }

    oi_setWheels(0, 0);
}



