#include "lcd.h"
#include "servo.h"
#include "Timer.h"
#include "ping.h"
#include "final_uart.h"
#include "open_interface.h"
#include "movement.h"
#include "final_scan.h"
#include "adc.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_OBJECTS 10
#define BUF_SIZE  5
#define PI 3.14159265359
#define CLIFF_DETECTION_THRESHOLD 300

object_t detected_objects[MAX_OBJECTS];
int object_count = 0;
bool scan_active = false;
bool autonomous_mode = false;

void load_cybot_songs(oi_t *sensor_data) {
    unsigned char notes0[16] = {
        72, 72, 72, 72, 79, 79, 76, 76,
        79, 0, 79, 72, 79, 76, 72, 79
    };
    unsigned char durations0[16] = {
        12, 12, 12, 20, 20, 20, 20, 20,
        32, 10, 20, 20, 20, 20, 20, 32
    };
    oi_loadSong(0, 16, notes0, durations0);

    unsigned char notes1[4] = { 76, 79, 83, 88 };
    unsigned char durations1[4] = { 16, 16, 16, 24 };
    oi_loadSong(1, 4, notes1, durations1);
}

int cyBot_readInt(void) {
    char buf[BUF_SIZE];
    int idx = 0;
    char c;
    long value;
    char *endptr;

    while (1) {
        while (!command_flag) {}
        c = command_byte;
        command_flag = 0;

        if (c == '\r' || c == '\n') {
            uart_sendStr("\r\n");
            break;
        }

        if (isdigit((unsigned char)c)) {
            if (idx < BUF_SIZE - 1) {
                buf[idx++] = c;
            }
        }
    }

    buf[idx] = '\0';
    value = strtol(buf, &endptr, 10);
    if (endptr != buf && *endptr == '\0') {
        return (int)value;
    }

    uart_sendStr("\r\nInvalid input.\r\n");
    return 0;
}

void display_commands() {
    uart_sendStr("\r\nList of commands:\r\n"
                 "g: scan for objects\r\n"
                 "s: stop scan\r\n"
                 "y: scan at 90 deg\r\n"
                 "u: user angle scan\r\n"
                 "m: play victory song\r\n"
                 "b: play arrival sound\r\n"
                 "\r\nMovement:\r\n"
                 "w/x: forward/backward 10cm\r\n"
                 "a/d: turn left/right 30 deg\r\n"
                 "i/k: forward/backward x cm\r\n"
                 "j/l: turn left/right x deg\r\n");
}

void checkAndDriveToCliff(oi_t *sensor_data) {
    oi_update(sensor_data);  // Ensure fresh values

    // Debug: Print current sensor values
    char debug[100];
    sprintf(debug, "FL: %d | L: %d | FR: %d | R: %d\r\n",
        sensor_data->cliffFrontLeftSignal,
        sensor_data->cliffLeftSignal,
        sensor_data->cliffFrontRightSignal,
        sensor_data->cliffRightSignal);
    uart_sendStr(debug);

    int result = checkBumpOrCliffDuringMove(sensor_data);
    if (result == 2) {

        sprintf(debug, "FL: %d | L: %d | FR: %d | R: %d\r\n",
            sensor_data->cliffFrontLeftSignal,
            sensor_data->cliffLeftSignal,
            sensor_data->cliffFrontRightSignal,
            sensor_data->cliffRightSignal);
        uart_sendStr(debug);

        oi_setWheels(0,0);
        uart_sendStr("\r\nBlack hole detected! Driving to center...\r\n");
        uart_sendStr("\r\n Press V to auto park.");

//        int angle = 90;
//        cyBot_Scan scanner;
//        scan(angle, &scanner);
//
//        double dist = scanner.sound_dist;
//        sprintf(debug, "Approaching center: angle=%d deg, dist=%.2f cm\r\n", angle, dist);
//        uart_sendStr(debug);
//
//        turn(sensor_data, angle - 90);
//        move_forward(sensor_data, (int)(dist * 10));
//        uart_sendStr("Arrived! Playing sound...\r\n");
//        oi_play_song(1);
    }
}

void auto_park(oi_t *sensor) {
    oi_update(sensor); // clear data
    bool inside_parking = false;
    bool exiting = false; // flag to determine if we're exiting a hole
    double forward_distance = 0;
    double backwards_distance = 0;

    // go into the black hole
    while(!inside_parking) {

        park_forward(sensor, 60);
        oi_update(sensor);

        if(check_bumpers(sensor)) {
            uart_sendStr("Bumper pressed!!!");
            oi_setWheels(0,0);
            break;
        }

        char debug[100];
           sprintf(debug, "\r\nFL: %d | L: %d | FR: %d | R: %d\r\n",
                   sensor->cliffFrontLeftSignal,
                   sensor->cliffLeftSignal,
                   sensor->cliffFrontRightSignal,
                   sensor->cliffRightSignal);
        uart_sendStr(debug);

        if (sensor->cliffFrontLeftSignal > 900 &&  sensor-> cliffFrontRightSignal > 900 && sensor->cliffLeftSignal > 900 && sensor->cliffRightSignal > 900) {
            uart_sendStr("\r\nWe are in the black hole.");
            inside_parking = true;
        }
    }

    timer_waitMillis(2000);

    if(inside_parking) {
        uart_sendStr("\r\nWould you like to auto align?");
        while((sensor->cliffFrontLeftSignal > 500) && (sensor->cliffLeftSignal > 500) && (sensor->cliffFrontRightSignal > 500) && (sensor->cliffRightSignal > 500))
        {
            if(check_bumpers(sensor)) {
                uart_sendStr("\r\nThis is not the destination! Object detected inside of parking garage!!!");
                oi_setWheels(0,0);
                uart_sendStr("\r\n Getting out of here!");
                exiting = true;
                while (backwards_distance <= forward_distance) {
                    backwards_distance += park_backwards(sensor, 5);
                }
                break;
            }

            if (!exiting) {
              uart_sendStr("\r\n Going to opposite side....");
              forward_distance += park_forward(sensor, 35);
            }


        }

        // if no object has been detected inside the parking zone, then try to align.
        if(!check_bumpers(sensor) && !exiting) {
            uart_sendStr("\r\n*Moving backwards*");
            timer_waitMillis(1000);
            park_backwards(sensor, 120);

            uart_sendStr("\r\n*Turning right");
            timer_waitMillis(500);
            turn(sensor, 90);

        }
        else {
            uart_sendStr("\r\n Getting out of here!");
            while (backwards_distance <= forward_distance) {
                backwards_distance += park_backwards(sensor, 5);
            }
        }

        timer_waitMillis(250);
        while((sensor->cliffFrontLeftSignal > 500) && (sensor->cliffLeftSignal > 500) && (sensor->cliffFrontRightSignal > 500) && (sensor->cliffRightSignal > 500))
        {
            if(check_bumpers(sensor) || exiting) {
                uart_sendStr("\r\nThis is not the destination! Object detected inside of parking garage!!!");
                oi_setWheels(0,0);
                break;
            }

            if(!check_bumpers(sensor) && !exiting) {
                uart_sendStr("\r\n Going to opposite side...");
                park_forward(sensor, 35);
            }

        }

        if(!check_bumpers(sensor) && !exiting) {
            uart_sendStr("\r\n **ALIGNING**");
            park_backwards(sensor, 120);
        }


    }




}

    int main(void) {
    timer_init();
    lcd_init();
    uart_interrupt_init();
    scan_init();
    adc_init();

    char buffer[100];
    oi_t* sensor_data = oi_alloc();
    oi_init(sensor_data);

    load_cybot_songs(sensor_data);
    lcd_puts("Ready to drive");
    uart_sendStr("\r\nCyBot Ready. Press h for commands.\r\n");

    while (1) {
        if (command_flag) {
            char cmd = command_byte;
            command_flag = 0;
            int custom_input;
            double dist_result;
            cyBot_Scan scanner;

            switch (cmd) {
                case 'g':
                    scan_active = true;
                    uart_sendStr("\r\nStarting scan...\r\n");
                    lcd_clear();
                    lcd_puts("Scanning...");
                    scanFullForObjects();
                    break;
                case 's':
                    scan_active = false;
                    uart_sendStr("\r\nScan interrupted.\r\n");
                    lcd_clear();
                    lcd_puts("Scan stopped");
                    break;
                case 'y':
                    uart_sendStr("\r\nIdle scan at 90...\r\n");
                    scan(90, &scanner);
                    {
                        float ir_dist_y = 157000.0 * pow(scanner.IR_raw_val, -1.176);
                        sprintf(buffer, "Angle 90 | Ping: %.2f cm | IR: %.2f cm\r\n", scanner.sound_dist, ir_dist_y);
                        uart_sendStr(buffer);
                    }
                    break;
                case 'u':
                    uart_sendStr("\r\nEnter angle (0 to 180): ");
                    custom_input = cyBot_readInt();
                    if (custom_input >= 0 && custom_input <= 180) {
                        scan(custom_input, &scanner);
                        float ir_dist_u = 157000.0 * pow(scanner.IR_raw_val, -1.176);
                        sprintf(buffer, "\r\nAngle %d | Ping: %.2f cm | IR: %.2f cm\r\n",
                                custom_input, scanner.sound_dist, ir_dist_u);
                        uart_sendStr(buffer);
                    } else {
                        uart_sendStr("\r\nInvalid angle.\r\n");
                    }
                    break;
                case 'm':
                    uart_sendStr("\r\nPlaying tune...\r\n");
                    lcd_clear();
                    lcd_puts("Victory!");
                    oi_play_song(0);
                    break;
                case 'b':
                    uart_sendStr("\r\nPlaying arrival sound...\r\n");
                    oi_play_song(1);
                    break;
                case 'w':
                    uart_sendStr("\r\nMoving forward 10cm\r\n");
                    double moved = move_forward(sensor_data, 100);
                    if (moved >= 100) {
                        sprintf(buffer, "Moved %.2fcm\r\n", moved / 10.0);
                        uart_sendStr(buffer);
                        checkAndDriveToCliff(sensor_data);
                    } else {
//                        oi_setWheels(0, 0);
                        uart_sendStr("Movement stopped early due to hazard. No black hole check.\r\n");
                    }
                    break;
                case 'x':
                    uart_sendStr("\r\nMoving backward 10cm\r\n");
                    dist_result = move_backwards(sensor_data, 100)/10.0;
                    sprintf(buffer, "Moved %.2fcm\r\n", dist_result);
                    uart_sendStr(buffer);
                    break;
                case 'a':
                    uart_sendStr("\r\nTurning left 30\r\n");
                    turn(sensor_data, 30);
                    break;
                case 'd':
                    uart_sendStr("\r\nTurning right 30\r\n");
                    turn(sensor_data, -30);
                    break;
                case 'i':
                    uart_sendStr("\r\nForward how many cm? ");
                    custom_input = cyBot_readInt() * 10;
                    if (custom_input != 0) {
                        sprintf(buffer, "\r\nMoving forward %dcm\r\n", custom_input/10);
                        uart_sendStr(buffer);
                        double moved_i = move_forward(sensor_data, custom_input);
                        if (moved_i >= custom_input) {
                            sprintf(buffer, "Moved %.2fcm\r\n", moved_i / 10.0);
                            uart_sendStr(buffer);
                            checkAndDriveToCliff(sensor_data);
                        } else {
//                            oi_setWheels(0, 0);
                            uart_sendStr("Movement stopped early due to hazard. No black hole check.\r\n");
                        }
                    }
                    break;
                case 'k':
                    uart_sendStr("\r\nBackward how many cm? ");
                    custom_input = cyBot_readInt() * 10;
                    if (custom_input != 0) {
                        sprintf(buffer, "\r\nMoving backward %dcm\r\n", custom_input/10);
                        uart_sendStr(buffer);
                        dist_result = move_backwards(sensor_data, custom_input)/10.0;
                        sprintf(buffer, "Moved %.2fcm\r\n", dist_result);
                        uart_sendStr(buffer);
                    }
                    break;
                case 'j':
                    uart_sendStr("\r\nTurn left how many deg? ");
                    custom_input = cyBot_readInt();
                    if (custom_input != 0) {
                        sprintf(buffer, "\r\nTurning left %d\r\n", custom_input);
                        uart_sendStr(buffer);
                        turn(sensor_data, custom_input);
                    }
                    break;
                case 'l':
                    uart_sendStr("\r\nTurn right how many deg? ");
                    custom_input = cyBot_readInt();
                    if (custom_input != 0) {
                        sprintf(buffer, "\r\nTurning right %d\r\n", custom_input);
                        uart_sendStr(buffer);
                        turn(sensor_data, -custom_input);
                    }
                    break;
                case 'v':
                    uart_sendStr("\r\nAutoparking");
                    auto_park(sensor_data);
                    break;

                case 'h':
                    display_commands();
                    break;
                default:
                    uart_sendStr("\r\nUnknown command.\r\n");
                    break;
            }

            uart_sendStr("\r\nEnter next command:\r\n");
        }
    }

    oi_free(sensor_data);
    return 0;
}
