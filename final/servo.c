#include "servo.h"
#include "Timer.h"
#include <inc/tm4c123gh6pm.h>

void servo_init(void) {
    SYSCTL_RCGCGPIO_R |= 0x02;  // Port B
    SYSCTL_RCGCTIMER_R |= 0x02; // Timer 1
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {}
    while ((SYSCTL_PRTIMER_R & 0x02) == 0) {}

    GPIO_PORTB_AFSEL_R |= 0x20;
    GPIO_PORTB_PCTL_R &= ~0x00F00000;
    GPIO_PORTB_PCTL_R |= 0x00700000;
    GPIO_PORTB_DEN_R |= 0x20;
    GPIO_PORTB_DIR_R |= 0x20;

    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER1_CFG_R = TIMER_CFG_16_BIT;
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD | TIMER_TAMR_TACMR | TIMER_TAMR_TAAMS;
    TIMER1_CTL_R |= TIMER_CTL_TAEVENT_POS;
    TIMER1_TAILR_R = 320000;  // 20ms period at 16 MHz
    TIMER1_TAMATCHR_R = 28000; // ~1.5ms pulse width (neutral position)
    TIMER1_TAPR_R = 0;
    TIMER1_CTL_R |= TIMER_CTL_TAEN;
}

void servo_move(int degrees) {
    int pulse_width = (int)(16000 + (degrees * (32000.0 / 180.0)));
    TIMER1_TAMATCHR_R = TIMER1_TAILR_R - pulse_width;
    timer_waitMillis(100);
}
