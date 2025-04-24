/**
 * @author Kevin Tran
 * 4/12/25
 *
 */

#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inc/tm4c123gh6pm.h>


void servo_init(void);

void servo_move(uint16_t degrees);


#endif /* SERVO_H_ */
