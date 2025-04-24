/*
 * adc.c
 * author: Kevin Tran
 */

#include "adc.h"

void adc_init() {

    // pg. 352
    // enable adc module 0
    SYSCTL_RCGCADC_R |= 0x01;

    //pg. 340
    // enable Port B
    SYSCTL_RCGCGPIO_R |= 0x02;

    //pg. 406
    while((SYSCTL_PRGPIO_R & 0x02) == 0) {};

    // set bit 4 as input
    GPIO_PORTB_DIR_R &= ~0x10;

    GPIO_PORTB_AFSEL_R |= 0x10;
    GPIO_PORTB_DEN_R &= ~0x10;
    GPIO_PORTB_AMSEL_R |= 0x10;

    while((SYSCTL_PRADC_R & 0x01) != 0x01) {};

    ADC0_PC_R = (ADC0_PC_R & ~0xF) | 0x1;
    ADC0_SSPRI_R = 0x0123;
    ADC0_ACTSS_R &= ~0x08;
    ADC0_EMUX_R &= ~0xF000;
    ADC0_SSMUX3_R = (ADC0_SSMUX3_R & ~0x000F) | 0xA;
    ADC0_SSCTL3_R = 0x06;
    ADC0_IM_R &= ~0x08;
    ADC0_ACTSS_R |= 0x08;


}



uint16_t adc_read() {
    uint16_t result;
    ADC0_PSSI_R |= 0x08;                // initate ss3
    while((ADC0_RIS_R & 0x08) == 0) {}; // wait for conversion done

    result = ADC0_SSFIFO3_R & 0xFFF; // read result
    ADC0_ISC_R |= 0x08;                 // acknowledge completion, clear bit
    return result;
}
