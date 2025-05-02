#include "final_scan.h"
#include "servo.h"
#include "ping.h"
#include "adc.h"
#include "Timer.h"
#include <math.h>
#include "open_interface.h"
#include "final_uart.h"

#define TAPE_THRESHOLD 2700
#define CLIFF_THRESHOLD 500

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
        getScan->sound_dist = (distance1 + distance2) / 2;  // PING sensor
        timer_waitMillis(500);
        update_flag = 0;
    }
     getScan->IR_raw_val = adc_read();  // IR sensor raw value

}

void scanFullForObjects(void) {
    cyBot_Scan scanner;

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

        timer_waitMillis(20);
    }

    uart_sendStr("\r\nFull sweep complete.\r\n");
}

bool checkBumpOrCliffDuringMove(oi_t* sensor_data) {
    oi_update(sensor_data);

    uint8_t cliffLeft, cliffFrontLeft, cliffFrontRight, cliffRight;
    oi_readCliffSensors(&cliffLeft, &cliffFrontLeft, &cliffFrontRight, &cliffRight);

    if (cliffLeft < CLIFF_THRESHOLD || cliffFrontLeft < CLIFF_THRESHOLD ||
        cliffFrontRight < CLIFF_THRESHOLD || cliffRight < CLIFF_THRESHOLD) {
        uart_sendStr("\r\nCliff or tape detected! Stopping...\r\n");
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



