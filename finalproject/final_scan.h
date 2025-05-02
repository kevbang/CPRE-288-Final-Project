#ifndef CYBOT_SCAN_H_
#define CYBOT_SCAN_H_

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


void scan_init(void);                   // Initializes servo, ping, and IR
void scan(int angle, cyBot_Scan* getScan);  // Scans at an angle, fills struct
void idleScan(void);

extern int right_calibration_value;
extern int left_calibration_value;

#endif
