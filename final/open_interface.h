#ifndef OPEN_INTERFACE_H_
#define OPEN_INTERFACE_H_

#include <stdint.h>

typedef struct {
    uint8_t bumpLeft;
    uint8_t bumpRight;
    uint16_t cliffLeftSignal;
    uint16_t cliffFrontLeftSignal;
    uint16_t cliffFrontRightSignal;
    uint16_t cliffRightSignal;
    double distance;
    double angle;
} oi_t;

oi_t* oi_alloc(void);
void oi_free(oi_t* self);

void oi_init(oi_t* self);
void oi_update(oi_t* self);

void oi_setWheels(int16_t right_wheel, int16_t left_wheel);
void oi_resetDistance(oi_t* self);
void oi_resetAngle(oi_t* self);
double oi_getDistance(oi_t* self);
double oi_getAngle(oi_t* self);

// Cliff sensor reading
void oi_readCliffSensors(uint8_t* left, uint8_t* front_left, uint8_t* front_right, uint8_t* right);

void oi_close(void);

#endif
