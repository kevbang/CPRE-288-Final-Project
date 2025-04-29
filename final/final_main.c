#include "Timer.h"
#include "lcd.h"
#include "uart-interrupt.h"
#include "open_interface.h"
#include "movement.h"
#include "adc.h"
#include "servo.h"
#include "cyBot_Scan.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#define MOVE_DIST_CM 5
#define TURN_ANGLE_DEG 15
#define IR_THRESHOLD 45
#define SCAN_INTERVAL_MS 500
#define CLIFF_THRESHOLD 2700

extern volatile char command_byte;
extern volatile int command_flag;

oi_t* sensor_data = NULL;

void sendSensorData(void);
int getAngleInput(void);
void scanAndReportIdle(void);
void scanFullForObjects(void);
bool checkBumpOrCliffDuringMove(void);

int main(void) {
    timer_init();
    lcd_init();
    uart_interrupt_init();
    adc_init();
    servo_init();
    cyBOT_init_Scan();


    right_calibration_value = 285250;
    left_calibration_value = 1256500;

    sensor_data = oi_alloc();
    oi_init(sensor_data);  

    lcd_clear();
    lcd_puts("Ready!");

    servo_move(90);
    timer_waitMillis(300);

    unsigned int lastScanTime = timer_getMillis();

    while (1) {
        if (command_flag) {
            command_flag = 0;
            char c = command_byte;

            if (c == 'w') {
                oi_update(sensor_data);
                if (!checkBumpOrCliffDuringMove()) {
                    move_forward(sensor_data, MOVE_DIST_CM * 10);
                }
            } else if (c == 's') {
                oi_update(sensor_data);
                if (!checkBumpOrCliffDuringMove()) {
                    move_backwards(sensor_data, MOVE_DIST_CM * 10);
                }
            } else if (c == 'a') {
                oi_update(sensor_data);
                if (!checkBumpOrCliffDuringMove()) {
                    turn(sensor_data, TURN_ANGLE_DEG);
                }
            } else if (c == 'd') {
                oi_update(sensor_data);
                if (!checkBumpOrCliffDuringMove()) {
                    turn(sensor_data, -TURN_ANGLE_DEG);
                }
            } else if (c == 'm') {
                sendSensorData();
            } else if (c == 'o') {
                uart_sendStr("Enter scan angle (0-180): ");
                int angle = getAngleInput();
                if (angle >= 0 && angle <= 180) {
                    servo_move(angle);
                    timer_waitMillis(500);
                    int ir_raw = adc_read();
                    char buffer[50];
                    snprintf(buffer, sizeof(buffer), "IR at %d degrees = %d\r\n", angle, ir_raw);
                    uart_sendStr(buffer);
                } else {
                    uart_sendStr("Invalid angle entered.\r\n");
                }
            } else if (c == 'g') {
                uart_sendStr("\r\nStarting full scan...\r\n");
                scanFullForObjects();
            }
        }

        if (timer_getMillis() - lastScanTime >= SCAN_INTERVAL_MS) {
            scanAndReportIdle();
            lastScanTime = timer_getMillis();
        }
    }

    oi_free(sensor_data);  // <-- Free memory
    oi_close();            // <-- Then safely stop
    return 0;
}

void sendSensorData(void) {
    char buffer[150];
    int ir_raw = adc_read();

    uint8_t cliffLeft, cliffFrontLeft, cliffFrontRight, cliffRight;
    oi_readCliffSensors(&cliffLeft, &cliffFrontLeft, &cliffFrontRight, &cliffRight);

    snprintf(buffer, sizeof(buffer),
             "IR Distance (raw): %d\r\nCliff Left: %d, Front Left: %d, Front Right: %d, Right: %d\r\n",
             ir_raw, cliffLeft, cliffFrontLeft, cliffFrontRight, cliffRight);

    uart_sendStr(buffer);

    if (cliffLeft < CLIFF_THRESHOLD || cliffFrontLeft < CLIFF_THRESHOLD ||
        cliffFrontRight < CLIFF_THRESHOLD || cliffRight < CLIFF_THRESHOLD) {
        oi_setWheels(0, 0);
        uart_sendStr("ALERT: Cliff/Tape Detected! Emergency Stop!\r\n");
    }
}

int getAngleInput(void) {
    char buf[10] = {0};
    int idx = 0;
    char ch = 0;

    while (1) {
        if ((UART1_FR_R & UART_FR_RXFE) == 0) {
            ch = UART1_DR_R & 0xFF;
            uart_sendChar(ch);
            if (ch == '\r') {
                break;
            }
            if (idx < 9) {
                buf[idx++] = ch;
            }
        }
    }
    buf[idx] = '\0';
    uart_sendStr("\r\n");

    return atoi(buf);
}

void scanAndReportIdle(void) {
    int angle = 90;
    servo_move(angle);
    timer_waitMillis(300);

    int ir_raw = adc_read();
    double distance_cm = 9462.0 * pow(ir_raw, -0.99);

    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Idle Scan -> Angle: %d deg, Distance: %.2f cm\r\n", angle, distance_cm);
    uart_sendStr(buffer);
}

void scanFullForObjects(void) {
    cyBOT_Scan_t scan;

    uart_sendStr("Starting full sweep...\r\n");
    timer_waitMillis(300);

    int angle;
    for (angle = 0; angle <= 180; angle += 2) {
        cyBOT_Scan(angle, &scan);
        double ir_cm = 157000.0 * pow(scan.IR_raw_val, -1.176);

        char msg[100];
        snprintf(msg, sizeof(msg), "Angle: %d deg, IR: %.2f cm, Ping: %.2f cm\r\n",
                 angle, ir_cm, scan.sound_dist);
        uart_sendStr(msg);

        timer_waitMillis(50);
    }

    uart_sendStr("\r\nFull sweep complete.\r\n");
}

bool checkBumpOrCliffDuringMove(void) {
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
