#include "ping.h"
#include "Timer.h"
#include <inc/tm4c123gh6pm.h>
#include "driverlib/interrupt.h"

volatile uint32_t start_time = 0;
volatile uint32_t end_time = 0;
volatile int echo_captured = 0;

void ping_init(void) {
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R3;  // Enable Timer3
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;    // Enable Port B
    while (!(SYSCTL_PRTIMER_R & SYSCTL_PRTIMER_R3)) {}
    while (!(SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R1)) {}

    GPIO_PORTB_AFSEL_R |= 0x08;   // Enable PB3 alternate function
    GPIO_PORTB_PCTL_R &= ~0x0000F000;
    GPIO_PORTB_PCTL_R |= 0x00007000;  // T3CCP1
    GPIO_PORTB_DEN_R |= 0x08;
    GPIO_PORTB_DIR_R &= ~0x08;    // Input

    TIMER3_CTL_R &= ~TIMER_CTL_TBEN;
    TIMER3_CFG_R = TIMER_CFG_16_BIT;
    TIMER3_TBMR_R = TIMER_TBMR_TBMR_CAP | TIMER_TBMR_TBCMR | TIMER_TBMR_TBCDIR;
    TIMER3_CTL_R |= TIMER_CTL_TBEVENT_BOTH;
    TIMER3_TBILR_R = 0xFFFF;
    TIMER3_TBPR_R = 0xFF;
    TIMER3_ICR_R = TIMER_ICR_CBECINT;
    TIMER3_IMR_R |= TIMER_IMR_CBEIM;

    NVIC_EN1_R |= (1 << (INT_TIMER3B - 48 - 32)); // IRQ #36
    IntMasterEnable();
    TIMER3_CTL_R |= TIMER_CTL_TBEN;
}

void ping_trigger(void) {
    GPIO_PORTB_DIR_R |= 0x08;      // PB3 output
    GPIO_PORTB_AFSEL_R &= ~0x08;   // Regular GPIO
    GPIO_PORTB_DATA_R &= ~0x08;    // LOW
    timer_waitMicros(2);
    GPIO_PORTB_DATA_R |= 0x08;     // HIGH
    timer_waitMicros(5);
    GPIO_PORTB_DATA_R &= ~0x08;    // LOW

    GPIO_PORTB_AFSEL_R |= 0x08;    // back to alt func
    GPIO_PORTB_PCTL_R |= 0x00007000;
    GPIO_PORTB_DIR_R &= ~0x08;     // Input again

    echo_captured = 0;
}

float ping_getDistance(void) {
    ping_trigger();
    int timeout = 30000;
    while (!echo_captured && timeout > 0) {
        timer_waitMicros(1);
        timeout--;
    }

    if (timeout <= 0) {
        return -1;
    }

    int pulse_width;
    if (start_time > end_time) {
        pulse_width = (start_time - end_time);
    } else {
        pulse_width = (0xFFFFFF - end_time + start_time);
    }

    float time_us = pulse_width * 0.0625;  // 16 MHz clock
    float distance_cm = time_us / 58.0;

    return distance_cm;
}

void TIMER3B_Handler(void) {
    TIMER3_ICR_R = TIMER_ICR_CBECINT;  // Clear interrupt

    static int state = 0;
    uint32_t time = TIMER3_TBR_R;

    if (state == 0) {
        start_time = time;
        state = 1;
    } else {
        end_time = time;
        echo_captured = 1;
        state = 0;
    }
}
