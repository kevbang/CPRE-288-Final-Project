 /* lab6-interrupt_template.c
 *
 * Template file for CprE 288 Lab 6
 *
 * @author Diane Rover, 2/15/2020
 *
 */

#include "Timer.h"
#include "lcd.h"
#include "cyBot_Scan.h"
#include "uart-interrupt.h"
#include "open_interface.h"
#include "movement.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

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
    double ir_distance_avg;
    double width_cm;
} object_t;

object_t detected_objects[MAX_OBJECTS];
int object_count = 0;
bool scan_active = false;
bool autonomous_mode = false;

// Converts raw IR to distance (calibrated curve)
double convert_ir(int ir_raw) {
    return 157000.0 * pow(ir_raw, -1.176);
}

double get_average_ir_cm(int angle, cyBOT_Scan_t* scan) {
    int total = 0;
    int i;
    for (i = 0; i < 4; i++) {
        cyBOT_Scan(angle, scan);
        total += scan->IR_raw_val;
    }
    return convert_ir(total / 4);
}

void drive_to_target(object_t target) {
    oi_t* sensor = oi_alloc();
    oi_init(sensor);

    // Assume CyBot starts facing 90 degrees (straight ahead).
    // Adjust this offset if your bot faces 0 degrees at rest.
    int turn_angle = target.avg_angle - 90;

    char turn_msg[64];
    sprintf(turn_msg, "Turning to %d deg\r\n", turn_angle);
    uart_sendStr(turn_msg);
    lcd_clear();
    lcd_printf("Turning: %d", turn_angle);

    // Perform turn
    turn(sensor, turn_angle);

    // Now drive forward
    lcd_clear();
    lcd_puts("Driving forward");

    double drive_dist_mm = (target.ping_distance - 10) * 10;  // Keep 10 cm safe distance

    if (drive_dist_mm > 0) {
        char drive_msg[64];
        sprintf(drive_msg, "Driving %.2f mm\r\n", drive_dist_mm);
        uart_sendStr(drive_msg);

        move_forward(sensor, drive_dist_mm);
        lcd_clear();
        lcd_puts("Arrived");
    } else {
        uart_sendStr("Too close to object, no drive.\r\n");
        lcd_clear();
        lcd_puts("Too close");
    }

    oi_free(sensor);
}


void scan_objects() {
    cyBOT_Scan_t scan;
    object_count = 0;
    bool object_found = false;
    int start = 0;

    lcd_clear();
    lcd_puts("Scanning...");
    int angle;
    for (angle = 0; angle <= 180; angle += 3) {
        if (command_flag && command_byte == 's') {
            scan_active = false;
            uart_sendStr("\r\nScan stopped.\r\n");
            lcd_clear();
            lcd_puts("Scan stopped");
            command_flag = 0;
            return;
        }

        double ir_dist = get_average_ir_cm(angle, &scan);

        char msg[64];
        sprintf(msg, "Angle: %d, IR dist: %.2f cm\r\n", angle, ir_dist);
        uart_sendStr(msg);

        if (ir_dist <= IR_THRESHOLD_CM && !object_found) {
            object_found = true;
            start = angle;
        } else if ((ir_dist > IR_THRESHOLD_CM || angle == 180) && object_found) {
            object_found = false;
            int end = angle;
            int mid = (start + end) / 2;

            cyBOT_Scan(mid, &scan);
            double ping_dist = scan.sound_dist;
            int delta_angle = end - start;
            double width = 2 * ping_dist * tan((delta_angle * M_PI / 180.0) / 2);

            if (width >= MIN_WIDTH_CM && width <= MAX_WIDTH_CM && object_count < MAX_OBJECTS) {
                object_t obj;
                obj.start_angle = start;
                obj.end_angle = end;
                obj.avg_angle = mid;
                obj.ping_distance = ping_dist;
                obj.ir_distance_avg = ir_dist;
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
    for (i = 1; i < object_count; i++) {
        if (detected_objects[i].width_cm < detected_objects[min_index].width_cm) {
            min_index = i;
        }
    }

    char summary[128];
    sprintf(summary, "\r\nSmallest object at angle %d, distance %.2f cm, width %.2f cm\r\n",
        detected_objects[min_index].avg_angle,
        detected_objects[min_index].ping_distance,
        detected_objects[min_index].width_cm);
    uart_sendStr(summary);
    lcd_clear();
    lcd_printf("Target: %d deg", detected_objects[min_index].avg_angle);

    drive_to_target(detected_objects[min_index]);
}

 int main(void) {
    timer_init();
    lcd_init();
    uart_interrupt_init();
    cyBOT_init_Scan(0b0111);

    right_calibration_value = 248500;
    left_calibration_value = 1251250;

    oi_t* sensor_data = oi_alloc();
    oi_init(sensor_data);

    lcd_puts("Lab 7 Ready");
    uart_sendStr("\r\nLab 7 Ready... Press g to scan.\r\n");

    while (1) {
        if (command_flag) {
            char cmd = command_byte;
            command_flag = 0;

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
                move_forward(sensor_data, 100);
            } else if (cmd == 'x') {
                uart_sendStr("\r\nMoving backward 10cm\r\n");
                lcd_clear();
                lcd_puts("Moving backward");
                move_backwards(sensor_data, 100);
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
            } else {
                uart_sendStr("\r\nUnknown command.\r\n");
            }
        }
    }

    oi_free(sensor_data);
    return 0;
}
