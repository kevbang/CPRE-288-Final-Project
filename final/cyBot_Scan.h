#ifndef CYBOT_SCAN_H_
#define CYBOT_SCAN_H_

typedef struct {
    float sound_dist;  // Ping distance
    int IR_raw_val;    // Raw IR ADC reading
} cyBOT_Scan_t;

void cyBOT_init_Scan(void);                   // Initializes servo, ping, and IR
void cyBOT_Scan(int angle, cyBOT_Scan_t* getScan);  // Scans at an angle, fills struct

extern int right_calibration_value;
extern int left_calibration_value;

#endif
