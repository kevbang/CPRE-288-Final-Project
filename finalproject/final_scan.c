#include "final_scan.h"
#include "servo.h"
#include "ping.h"
#include "adc.h"
#include "Timer.h"
#include <math.h>

int right_calibration_value = 285250;
int left_calibration_value = 1256500;

void scan_init(void) {
    servo_init();
    ping_init();
    adc_init();
}

void scan(int angle, cyBot_Scan* getScan) {
    servo_move(angle);
    timer_waitMillis(300);

    getScan->sound_dist = ping_getDistance();  // PING sensor
    getScan->IR_raw_val = adc_read();          // IR sensor raw value
}
