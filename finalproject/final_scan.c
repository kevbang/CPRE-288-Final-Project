#include "final_scan.h"
#include "servo.h"
#include "ping.h"
#include "adc.h"
#include "Timer.h"
#include <math.h>
#include "open_interface.h"
#include "final_uart.h"

#define TAPE_THRESHOLD 2600
#define CLIFF_THRESHOLD 500

// Might need redefining for more busy area, but doubting it
#define MAX_OBJECTS 15
#define IR_THRESHOLD_CM 45
#define MIN_WIDTH_CM 3
#define MAX_WIDTH_CM 30

object_t detected_objects[MAX_OBJECTS];

int right_calibration_value = 285250;
int left_calibration_value = 1256500;

void scan_init(void) {
    ping_init();
    adc_init();
    servo_init();
}

void scan(int angle, cyBot_Scan* getScan) {
    servo_move(angle);
    timer_waitMillis(20);

    unsigned long time_diff;
    float ms;
    ping_trigger();
    timer_waitMillis(2);//take a measurement every 500 ms
    if(update_flag >= 2) {

        time_diff = last_time - current_time;

        float distance1 = ping_getDistance(time_diff, &ms);
        float distance2 = ping_getDistance(time_diff, &ms);
        getScan->sound_dist = (distance1 + distance2) / 2.0;  // PING sensor
        timer_waitMillis(500);
        update_flag = 0;
    }
     getScan->IR_raw_val = adc_read();  // IR sensor raw value

}

void scanFullForObjects(void) {
    cyBot_Scan scanner;
    int object_count = 0; // Keep track of object number
    bool object_found = false;
    int start = 0;

    uart_sendStr("Starting full sweep...\r\n");
    timer_waitMillis(20);

    int angle;
    for (angle = 0; angle <= 180; angle += 2) {
        scan(angle, &scanner);
        double ir_cm = 157000.0 * pow(scanner.IR_raw_val, -1.176);

        char msg[100];
        snprintf(msg, sizeof(msg), "Angle: %d deg, IR: %.2f cm, Ping: %.2f cm\r\n",
                 angle, ir_cm, scanner.sound_dist);
        uart_sendStr(msg);
        
        /**
         * Object detection logic that uses IR sensor thresholds and max/min width thresholds to determine objects
         */
        if (ir_cm <= IR_THRESHOLD_CM && !object_found) {
            object_found = true;
            start = angle;
        } else if ((ir_cm > IR_THRESHOLD_CM || angle == 180) && object_found) {
            object_found = false;
            int end = angle;
            int mid = (start + end) / 2;

            scan(mid, &scanner);
            double ping_dist = scanner.sound_dist;
            ir_cm = 157000.0 * pow(scanner.IR_raw_val, -1.176);
            int delta_angle = end - start;
            double width = 2 * ping_dist * tan((delta_angle * M_PI / 180.0) / 2);

            if (width >= MIN_WIDTH_CM && width <= MAX_WIDTH_CM && object_count < MAX_OBJECTS) {
                object_t obj;
                obj.start_angle = start;
                obj.end_angle = end;
                obj.avg_angle = mid;
                obj.ping_distance = ping_dist;
                obj.ir_distance_avg = ir_cm;
                obj.width_cm = width;
                detected_objects[object_count++] = obj;
            }
        }

        timer_waitMillis(20);
    }

    uart_sendStr("\r\nFull sweep complete.\r\n");

    // If we have no objects, display no objects
    if (object_count == 0) {
        uart_sendStr("\r\nNo valid object detected.\r\n");
        lcd_clear();
        lcd_puts("No object");
        return;
    } else { // Let the user know how many objects we got
        uart_sendStr("\r\n%d valid object detected.\r\n",
        object_count);
    }

    int i;
    char summary[128];
    for (i = 0; i < object_count; i++) { // Changed to 0 because otherwise it would skip the first object
        sprintf(summary, "\r\nObject #%d Angle: %d Ping: %.2f cm IR: %.2f cm Width: %.2f cm\r\n",
            i+1,
            detected_objects[i].avg_angle,
            detected_objects[i].ping_distance,
            detected_objects[i].ir_distance_avg,
            detected_objects[i].width_cm);
        uart_sendStr(summary);
    }
}

bool checkBumpOrCliffDuringMove(oi_t* sensor_data) {
    oi_update(sensor_data);

    uint8_t cliffLeft, cliffFrontLeft, cliffFrontRight, cliffRight;
    oi_readCliffSensors(&cliffLeft, &cliffFrontLeft, &cliffFrontRight, &cliffRight);

    if (cliffLeft > TAPE_THRESHOLD || cliffFrontLeft > TAPE_THRESHOLD ||
        cliffFrontRight > TAPE_THRESHOLD || cliffRight > TAPE_THRESHOLD) {
        uart_sendStr("\r\nTape detected! Stopping...\r\n");
        oi_setWheels(0, 0);
        return true;
    }

    if (cliffLeft < CLIFF_THRESHOLD || cliffFrontLeft < CLIFF_THRESHOLD ||
        cliffFrontRight < CLIFF_THRESHOLD || cliffRight < CLIFF_THRESHOLD) {
        uart_sendStr("\r\nDestination Zone detected! Stopping...\r\n");
        oi_setWheels(0, 0);
        return true;
    }

    return false;
}

/**
 * This function is used to scan while moving.
 */
void idleScan(void) {
    int start_angle = 45;
    int end_angle = 135;
    int step = 5;
    char buffer[90];
    cyBot_Scan scanner;
    int i = 0;
    float distances[19]; //  (Up to (135-45) / 5 + 1 = 19)
    int angles[19];
    int min_angle = 90;
    int angle;
    uart_sendStr("This is idle scan\r\n");

    /*
     * SCAN FOR DATA
     */
    for(angle = start_angle; angle <= end_angle; angle+=step, i++) {
        scan(angle, &scanner);
        distances[i] = scanner.sound_dist;
        angles[i] = angle;
        sprintf(buffer, "Angle:%3d, Distance %.2f cm\r\n", angle, scanner.sound_dist);
        uart_sendStr(buffer);
    }

    /*
     * FIND THE BIGGEST GAP
     */

    float largest_gap = 0.0f;
    int gap_index = 0;
    int j = 0;
    int num_points = (end_angle - start_angle) / step + 1;

    for(j = 0; j < num_points - 1; j++) {
        float gap = distances[j + 1] - distances[j];
        if (gap > largest_gap) {
            largest_gap = gap;
            gap_index = j;
        }
    }

    int gap_angle = (angles[gap_index] + angles[gap_index + 1]) / 2;

    servo_move(gap_angle);
    sprintf(buffer, "turning to midpoint angle: %d\r\n", gap_angle);
    uart_sendStr(buffer);

}



