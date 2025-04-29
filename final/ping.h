#ifndef PING_H_
#define PING_H_

#include <stdint.h>

// Functions
void ping_init(void);
void ping_trigger(void);
float ping_getDistance(void);

extern volatile uint32_t start_time;
extern volatile uint32_t end_time;
extern volatile int echo_captured;

#endif /* PING_H_ */
