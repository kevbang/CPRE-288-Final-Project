#ifndef CYBOT_SCAN_H_
#define CYBOT_SCAN_H_

#include "open_interface.h"  // FIXED: for oi_t

typedef struct {
    float sound_dist;  // Ping distance
    int IR_raw_val;    // Raw IR ADC reading
} cyBot_Scan;

// Object structure
typedef struct {
    int start_angle;
    int end_angle;
    int avg_angle;
    double ping_distance;
    double ir_distance_avg;
    double width_cm;
} object_t;

void scan_init(void);
void scan(int angle, cyBot_Scan* getScan);
void scanFullForObjects(void);
void idleScan(void);
int checkBumpOrCliffDuringMove(oi_t* sensor_data);

extern int right_calibration_value;
extern int left_calibration_value;

#endif
