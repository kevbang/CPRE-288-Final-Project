#include <inc/tm4c123gh6pm.h>
#include "Timer.h"

void buzzer_init(void) {
    SYSCTL_RCGCGPIO_R |= 0x02;
    GPIO_PORTB_DIR_R |= 0x20;
    GPIO_PORTB_DEN_R |= 0x20;
    GPIO_PORTB_AFSEL_R &= ~0x20;
}

void buzzer_on(void) {
    GPIO_PORTB_DATA_R |= 0x20;
}

void buzzer_off(void) {
    GPIO_PORTB_DATA_R &= ~0x20;
}
