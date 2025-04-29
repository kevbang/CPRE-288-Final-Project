#ifndef SERVO_H_
#define SERVO_H_

void servo_init(void);
void servo_move(int degrees);

extern int right_calibration_value;
extern int left_calibration_value;

#endif /* SERVO_H_ */
