#include "movement.h"
#include "open_interface.h"
#include "lcd.h"
#include "final_uart.h"
#include <math.h>

bool check_hazards(oi_t *sensor) {
    // For cliff sensors, (0 = no cliff, 1 = cliff).
    if (sensor->cliffLeft || sensor->cliffFrontLeft) { // Check for cliff left side, stop and display message if cliff
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Cliff detected");
        uart_sendStr("\r\nCliff left side, back up\r\n");
        return true;
    } 
    if (sensor->cliffRight || sensor->cliffFrontRight) { // Check for cliff right side, stop and display message if cliff
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Cliff detected");
        uart_sendStr("\r\nCliff right side, back up\r\n");
        return true;
    }
    if (sensor->bumpLeft) { // Check for bump on left side, stop and display message if bump
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Object bumped left");
        uart_sendStr("\r\nObject bumped left side, back up\r\n");
        return true;
    }
    if (sensor->bumpRight) { // Check for bump on right side, stop and display message if bump
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Object bumped right");
        uart_sendStr("\r\nObject bumped right side, back up\r\n");
        return true;
    }
    return false;
}

// Move forward and stop if bump is detected
double move_forward(oi_t *sensor, double distance_mm) {
    double sum = 0;
    oi_update(sensor); // Clear old data
    oi_setWheels(200, 200); // Set wheel speed forward

    while (sum < distance_mm) {
        oi_update(sensor);
        sum += sensor->distance;
        if (check_hazards(sensor)) return sum; // If we run into something stop      
    }

    oi_setWheels(0, 0); // Stop the robot
    return sum;
}

// Move backward
double move_backwards(oi_t *sensor, double distance_mm) {
    double sum = 0;
    oi_update(sensor); // Clear old data
    oi_setWheels(-200, -200); // Set wheel speed backward

    while (sum < distance_mm) {
        oi_update(sensor);
        sum += fabs(sensor->distance);
        if (check_hazards(sensor)) return sum; // If we run into something stop      
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

