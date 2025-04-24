/**
 * @author Kevin Tran
 * 4/12/25
 */

#include "servo.h"
#include "Timer.h"


void servo_init(void) {

    SYSCTL_RCGCTIMER_R |= 0x02;
    SYSCTL_RCGCGPIO_R |= 0x02;

    SYSCTL_RCGCUART_R |= 0x01;

    while((SYSCTL_PRUART_R & 0x01) == 0);
    while((SYSCTL_PRGPIO_R & 0x02) == 0);

    GPIO_PORTB_AFSEL_R |= 0x20; // alternate function
    GPIO_PORTB_PCTL_R &= ~0x00F00000; // set PB5 as T1CCP1
    GPIO_PORTB_PCTL_R |= 0x00700000;
    GPIO_PORTB_DEN_R |= 0x20; // enable digital
    GPIO_PORTB_DIR_R |= 0x20; // output

    TIMER1_CTL_R &= ~0x100;

    TIMER1_CFG_R = 0x04; // 16 bit mode
    TIMER1_TBMR_R = 0x0A;

    TIMER1_TBILR_R = 320000 & 0xFFFF; // load 16 lower bits to ILR
    TIMER1_TBPR_R = (320000 >> 16) & 0xFF; // load 8 upper bits to prescaler

    TIMER1_TBMATCHR_R = (320000 - 16000) & 0xFFFF;
    TIMER1_TBPMR_R = ((320000 - 16000) >> 16) & 0xFF;

    TIMER1_CTL_R |= 0x100;


}


void servo_move(uint16_t degrees) {

    if(degrees > 180) {
        degrees = 180;
    }

    // 16 mhz * 20 ms
    uint32_t period = 320000;
    // linear y = m(x) + b
    uint32_t pulse_width = 8000 + ((degrees * (32000 - 6000)) / 180);
    // calculating the point to stop
    uint32_t match = period - pulse_width;

    // store match
    TIMER1_TBMATCHR_R = match & 0xFFFF; // 16 lower bits
    TIMER1_TBPMR_R = (match >> 16) & 0xFF; // 8 upper bits

}


