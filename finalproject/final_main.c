/**
 * Main file for CPRE 2880 Final Project
 * @author Kevin Tran
 *
 */

#include "lcd.h"
#include "servo.h"
#include "Timer.h"
#include "lcd.h"
#include "servo.h"
#include "ping.h"
#include "final_uart.h"
#include "open_interface.h"
#include "movement.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h> // Needed for isDigit()

#define MAX_OBJECTS 10
#define IR_THRESHOLD_CM 45
#define MIN_WIDTH_CM 3
#define MAX_WIDTH_CM 30

// Object structure
typedef struct {
    int start_angle;
    int end_angle;
    int avg_angle;
    double ping_distance;
    double cm_distance_avg;
    double width_cm;
} object_t;

object_t detected_objects[MAX_OBJECTS];
int object_count = 0;
bool scan_active = false;
bool autonomous_mode = false;

// Changed to do 2 scans
double get_average_ping_cm(int angle) {
    double total = 0;
    int i;

    // Need a longer wait time when moving to 0
    if (angle == 0) {
        servo_move(0); //Move scanner to 0 degrees
        timer_waitMillis(1000); // Wait for scanner to move
    } else {
        servo_move(angle); //Move scanner to angle
        timer_waitMillis(100); // Wait for scanner to move
    }
    for (i = 0; i < 2; i++) {
        total += ping_getDistance();
    }
    return total/2.0;
}

void scan_objects() {
    object_count = 0; // Keep track of num objects
    bool object_found = false; // Flag for determining if we are in an object
    int start = 0; // Holds starting angle of an object

    lcd_clear();
    lcd_puts("Scanning...");
    int angle;
    for (angle = 0; angle <= 180; angle += 3) {
        if (command_flag && command_byte == 's') { // Press s to stop scanning
            scan_active = false;
            uart_sendStr("\r\nScan stopped.\r\n");
            lcd_clear();
            lcd_puts("Scan stopped");
            command_flag = 0;
            return;
        }

        double dist_cm = get_average_ping_cm(angle);

        char msg[64];
        sprintf(msg, "Angle: %d, IR dist: %.2f cm\r\n", angle, dist_cm);
        uart_sendStr(msg);

        if (dist_cm <= IR_THRESHOLD_CM && !object_found) {
            object_found = true;
            start = angle;
        } else if ((dist_cm > IR_THRESHOLD_CM || angle == 180) && object_found) { // Object end, need to check if we can add object to list
            object_found = false;
            int end = angle;
            int mid = (start + end) / 2;

            servo_move(mid);        // Move servo to mid angle of objs
            timer_waitMillis(1000); // Wait for scanner to move
            double ping_dist = ping_getDistance(); // Ping and get dist
            
            // Compute width
            int delta_angle = end - start;
            double width = 2 * ping_dist * tan((delta_angle * M_PI / 180.0) / 2); 

            if (width >= MIN_WIDTH_CM && width <= MAX_WIDTH_CM && object_count < MAX_OBJECTS) {
                object_t obj;
                obj.start_angle = start;
                obj.end_angle = end;
                obj.avg_angle = mid;
                obj.ping_distance = ping_dist;
                obj.cm_distance_avg = dist_cm;
                obj.width_cm = width;
                detected_objects[object_count++] = obj;
            }
        }
    }

    uart_sendStr("\r\nScan complete.\r\n");
    lcd_clear();
    lcd_puts("Scan complete");

    if (object_count == 0) {
        uart_sendStr("\r\nNo valid object detected.\r\n");
        lcd_clear();
        lcd_puts("No object");
        return;
    }

    int min_index = 0;
    int i;
    char summary[128];
    for (i = 1; i < object_count; i++) {
        sprintf(summary, "\r\nObject #%d at angle %d, distance %.2f cm, width %.2f cm\r\n",
            i,
            detected_objects[i].avg_angle,
            detected_objects[i].ping_distance,
            detected_objects[i].width_cm);
        uart_sendStr(summary);
    }
}

#define BUF_SIZE  3 // Set up to only take 2 digits

// Get double digit input in PuTTy from user
int cyBot_readInt(void) {
    char buf[BUF_SIZE];
    int idx = 0;
    char c;
    long value;
    char *endptr;

    // keep reading until CR or LF
    while (1) {
        while (!command_flag) {} // Wait for next byte
        c = command_byte; // Get the user input
        command_flag = 0; // Reset command flag
        uart_sendByte(c); // Display bit back to user

        // if newline or carriage-return, stop
        if (c == '\r' || c == '\n') {
            uart_sendStr("\r\n");              
            break;
        }

        // Only accepting ints
        if (isdigit((unsigned char)c)) { 
            if (idx < BUF_SIZE - 1) { // Check space
                buf[idx++] = c;
            } 
            continue;
        }      
    }

    buf[idx] = '\0';    // Terminate string
    
    // Validate we have an int
    value = strtol(buf, &endptr, 10); // buf = string to convert, pointer that is used as output param, base 10. returns long
    if (endptr != buf && *endptr == '\0') {
        // Successful conversion
        return (int)value;
    }

    uart_sendStr("\r\nInvalid input, no commands sent :(\r\n");
    return 0;   // Return 0 as default
}

 void display_commands() {
    uart_sendStr("\r\nList of commands:\r\n
        g: initiate scan\r\n
        s: interrupt scan\r\n
        \r\n
        Set Movement:\r\n
        w: forward 10 cm\r\n
        x: backward 10 cm\r\n
        a: left 30 deg\r\n
        d: right 30 deg\r\n
        \r\n
        Custom Movement:\r\n
        i: forward x cm\r\n
        k: backward x cm\r\n
        j: left x deg\r\n
        l: right x deg\r\n
    ");
 }

 int main(void) {
    timer_init();
    lcd_init();
    uart_interrupt_init();
    servo_init();
    scan_init();

    oi_t* sensor_data = oi_alloc();
    oi_init(sensor_data);

    lcd_puts("Ready to drive");
    uart_sendStr("\r\nReady... Press g to scan. Press h for help\r\n");

    while (1) {
        if (command_flag) {
            char cmd = command_byte;
            command_flag = 0;
            int custom_input;
            double dist_result;
            if (cmd == 'g') {
                scan_active = true;
                autonomous_mode = false;
                uart_sendStr("\r\nStarting Scan...\r\n");
                lcd_clear();
                lcd_puts("Scanning...");
                scan_objects();
            } else if (cmd == 's') {
                scan_active = false;
                uart_sendStr("\r\nManual Stop Triggered.\r\n");
                lcd_clear();
                lcd_puts("Scan stopped");
            } else if (cmd == 'w') {
                uart_sendStr("\r\nMoving forward 10cm\r\n");
                lcd_clear();
                lcd_puts("Moving forward");
                dist_result = move_forward(sensor_data, 100)/10.0;
                uart_sendStr("\r\nMoved forward %.2fcm\r\n", dist_result); // Print resulting forward movement
            } else if (cmd == 'x') {
                uart_sendStr("\r\nMoving backward 10cm\r\n");
                lcd_clear();
                lcd_puts("Moving backward");
                dist_result = move_backwards(sensor_data, 100)/10.0;
                uart_sendStr("\r\nMoved backward %.2fcm\r\n", dist_result); // Print resulting backward movement
            } else if (cmd == 'a') {
                uart_sendStr("\r\nTurning left 30 deg\r\n");
                lcd_clear();
                lcd_puts("Turning left");
                turn(sensor_data, 30);
            } else if (cmd == 'd') {
                uart_sendStr("\r\nTurning right 30 deg\r\n");
                lcd_clear();
                lcd_puts("Turning right");
                turn(sensor_data, -30);
            } else if (cmd == 't') {
                uart_sendStr("\r\nTest command triggered\r\n");
                lcd_clear();
                lcd_puts("Test Triggered");
            } else if (cmd == 'i') { // Move custom amount forward
                uart_sendStr("\r\nHow far forward in cm?\r\n");
                custom_input = cyBot_readInt() * 10; // Get double digit int
                if (custom_input == 0) { // Invalid input, continue
                    continue;
                }
                uart_sendStr("\r\nMoving forward %dcm\r\n", custom_input/10);
                lcd_clear();
                lcd_puts("Moving forward");
                dist_result = move_forward(sensor_data, custom_input)/10.0;
                uart_sendStr("\r\nMoved forward %.2fcm\r\n", dist_result); // Print resulting forward movement
            } else if (cmd == 'j') { // Turn left cutom amount
                uart_sendStr("\r\nTurn how much left in cm?\r\n");
                custom_input = cyBot_readInt(); // Get double digit int
                if (custom_input == 0) { // Invalid input, continue
                    continue;
                }
                uart_sendStr("\r\nRotating %d deg left\r\n", custom_input);
                lcd_clear();
                lcd_puts("Turning left");
                turn(sensor_data, custom_input);
            } else if (cmd == 'k') { // Move custom amount backwards
                uart_sendStr("\r\nHow far backwards in cm?\r\n");
                custom_input = cyBot_readInt() * 10; // Get double digit int
                if (custom_input == 0) { // Invalid input, continue
                    continue;
                }
                uart_sendStr("\r\nMoving backward %dcm\r\n", custom_input/10);
                lcd_clear();
                lcd_puts("Moving backward");
                dist_result = move_backwards(sensor_data, custom_input)/10.0;
                uart_sendStr("\r\nMoved backward %.2fcm\r\n", dist_result); // Print resulting backward movement
            } else if (cmd == 'l') { // Turn right custom amount
                uart_sendStr("\r\nTurn how much right in cm?\r\n");
                custom_input = cyBot_readInt(); // Get double digit int
                if (custom_input == 0) { // Invalid input, continue
                    continue;
                }
                uart_sendStr("\r\nRotating %d deg right\r\n", custom_input);
                lcd_clear();
                lcd_puts("Turning right");
                turn(sensor_data, -custom_input);
            } else if (cms == 'h') { // Display list of commands
                display_commands(); 
            } else {
                uart_sendStr("\r\nUnknown command.\r\n");
            }
            uart_sendStr("\r\nPlease enter your next command\r\n"); // Default message that tells user the Cybot is ready for next command
        }
    }

    oi_free(sensor_data);
    return 0;
}

