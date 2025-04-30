#include "final_scan.h"
#include "servo.h"
#include "ping.h"
#include "adc.h"
#include "Timer.h"
#include <math.h>
#include "open_interface.h"

#define TAPE_THRESHOLD 2700
#define CLIFF_THRESHOLD 500

int right_calibration_value = 285250;
int left_calibration_value = 1256500;

void scan_init(void) {
    servo_init();
    ping_init();
    adc_init();
}

void scan(int angle, cyBot_Scan* getScan) {
    servo_move(angle);
    timer_waitMillis(20);

    getScan->sound_dist = ping_getDistance();  // PING sensor
    getScan->IR_raw_val = adc_read();          // IR sensor raw value
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
