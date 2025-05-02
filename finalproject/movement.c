#include "movement.h"
#include "open_interface.h"
#include "lcd.h"
#include "final_uart.h"
#include <math.h>

#define TAPE_THRESHOLD 2600
#define CLIFF_THRESHOLD 500

bool check_hazards(oi_t *sensor) {
    uint16_t tape_sens_left = sensor->cliffFrontLeftSignal;
    uint16_t tape_sens_right = sensor->cliffFrontRightSignal;
    uint16_t tape_left = sensor->cliffLeftSignal;
    uint16_t tape_right = sensor->cliffRightSignal;

    char buffer[25];

    sprintf(buffer, "\r\nFront Left: %d\r\n", tape_sens_left);
    uart_sendStr(buffer);
    sprintf(buffer, "Left: %d\r\n", tape_left);
    uart_sendStr(buffer);
    sprintf(buffer, "\r\nFront Right: %d\r\n", tape_sens_right);
    uart_sendStr(buffer);
    sprintf(buffer, "Left: %d\r\n", tape_right);
    uart_sendStr(buffer);

    // Test what threshold tape sets the sensors too and check for that in order to determine when we hit the tape
    // If we are between the tape and cliff threshold then we are at the border and should stop. This checks left side of the vehicle 
    if ((tape_sens_left > TAPE_THRESHOLD || tape_left > TAPE_THRESHOLD) ) {
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Tape detected");
        uart_sendStr("\r\nTape left side\r\n");
        return true;
    }
    // If we are between the tape and cliff threshold then we are at the border and should stop. This checks right side of the vehicle
    if ((tape_sens_right > TAPE_THRESHOLD) || (tape_right > TAPE_THRESHOLD )) {
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Tape detected");
        uart_sendStr("\r\nTape right side\r\n");
        return true;
    }

    // Landing zone stuff, we could stop at the landing zone and then go forawrd a certain distance to a "safe spot"

    if(tape_sens_left < CLIFF_THRESHOLD || tape_left < CLIFF_THRESHOLD) {
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Landing zone detected");
        uart_sendStr("\r\nLanding zone left side\r\n");
        return true;
    }

    if(tape_sens_right < CLIFF_THRESHOLD || tape_right < CLIFF_THRESHOLD) {
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Landing zone detected");
        uart_sendStr("\r\nLanding zone right side\r\n");
        return true;
    }

    // bumper

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

bool check_back_hazards(oi_t *sensor) {
//    uint16_t tape_sens_left = sensor->cliffFrontLeftSignal;
//    uint16_t tape_sens_right = sensor->cliffFrontRightSignal;
    uint16_t tape_left = sensor->cliffLeftSignal;
    uint16_t tape_right = sensor->cliffRightSignal;

    char buffer[25];

//    sprintf(buffer, "\r\nFront Left: %d\r\n", tape_sens_left);
//    uart_sendStr(buffer);
    sprintf(buffer, "Left: %d\r\n", tape_left);
    uart_sendStr(buffer);
//    sprintf(buffer, "\r\nFront Right: %d\r\n", tape_sens_right);
//    uart_sendStr(buffer);
    sprintf(buffer, "Left: %d\r\n", tape_right);
    uart_sendStr(buffer);

    // Test what threshold tape sets the sensors too and check for that in order to determine when we hit the tape
    // If we are between the tape and cliff threshold then we are at the border and should stop. This checks left side of the vehicle
    if (tape_left > TAPE_THRESHOLD)  {
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Tape detected");
        uart_sendStr("\r\nTape left side\r\n");
        return true;
    }
    // If we are between the tape and cliff threshold then we are at the border and should stop. This checks right side of the vehicle
    if (tape_right > TAPE_THRESHOLD ) {
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Tape detected");
        uart_sendStr("\r\nTape right side\r\n");
        return true;
    }

    // cliff stuff

    if(tape_left < CLIFF_THRESHOLD) {
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Cliff detected");
        uart_sendStr("\r\nCliff left side, back up\r\n");
        return true;
    }

    if(tape_right < CLIFF_THRESHOLD) {
        oi_setWheels(0, 0);
        lcd_clear();
        lcd_puts("Cliff detected");
        uart_sendStr("\r\nCliff right side, back up\r\n");
        return true;
    }

    // bumper

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
        if (check_back_hazards(sensor)) return sum; // If we run into something stop
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

