/**
 * Driver for ping sensor
 * @file ping.c
 * @author Kevin Tran
 */

#include "ping.h"
#include "Timer.h"

// Global shared variables
// Use extern declarations in the header file

volatile uint32_t g_start_time = 0;
volatile uint32_t g_end_time = 0;
volatile enum{LOW, HIGH, DONE} g_state = LOW; // State of ping echo pulse


void ping_init (void){

  // YOUR CODE HERE

    /*
     * much of these values are in the Tm4c header file btw
     */

    // enable clock Timer 3
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R3;
    // enable I/O register for Port B
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;

    while((SYSCTL_PRTIMER_R & SYSCTL_PRTIMER_R3) == 0) {};
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R1) == 0) {};

    // Enabling the alternate function on pin PB3
    GPIO_PORTB_AFSEL_R |= (1 << 3);

    GPIO_PORTB_PCTL_R &= ~GPIO_PCTL_PB3_M; // clear PCTL register

    //Set PB3 to T3CCP1 timer
    GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB3_T3CCP1;

    GPIO_PORTB_DEN_R |= (1 << 3); // enabling  digital on PB3
    GPIO_PORTB_DIR_R &= ~(1 << 3); // set PB3 as input.

    // According to Bai book (9.2.6.3):

    TIMER3_CTL_R &= ~TIMER_CTL_TBEN; // disable the timer before configuration
    TIMER3_CFG_R = TIMER_CFG_16_BIT; // set as a 16-bit timer. We will end up adding another 8 bits later.

    //capture mode, edge time mode, and count direction ( down )
    TIMER3_TBMR_R = TIMER_TBMR_TBMR_CAP | TIMER_TBMR_TBCMR | TIMER_TBMR_TBCDIR;
    TIMER3_CTL_R = TIMER_CTL_TBEVENT_BOTH;
    TIMER3_TBPR_R = 0xFF; // 8 bit max value (prescale value) GIVEN
    TIMER3_TBILR_R = 0xFFFF; // GIVEN

    TIMER3_IMR_R |= TIMER_IMR_CBEIM;
    TIMER3_ICR_R = TIMER_ICR_CBECINT; // clear interrupt
    TIMER3_CTL_R = TIMER_CTL_TBEN; // enable timer 3b

    IntRegister(INT_TIMER3B, TIMER3B_Handler);
    IntEnable(INT_TIMER3B);
    IntMasterEnable();

}



void ping_trigger (void){
    g_state = LOW;
    // Disable timer and disable timer interrupt
    TIMER3_CTL_R &= ~TIMER_CTL_TBEN;
    TIMER3_IMR_R &= ~TIMER_IMR_CBEIM;
    // Disable alternate function (disconnect timer from port pin)
    GPIO_PORTB_AFSEL_R &= ~(1 << 3);

    // YOUR CODE HERE FOR PING TRIGGER/START PULSE

    GPIO_PORTB_DIR_R |= (1 << 3);
    GPIO_PORTB_DATA_R &= ~(1 << 3); // set to low (0)
    timer_waitMillis(200);

    GPIO_PORTB_DATA_R |= (1 << 3); // set back to high (1)
    timer_waitMillis(200);

    GPIO_PORTB_DATA_R &= ~(1 << 3); // set back to low

    // Clear an interrupt that may have been erroneously triggered
    TIMER3_ICR_R = TIMER_IMR_CBEIM;
    // Re-enable alternate function, timer interrupt, and timer
    GPIO_PORTB_DIR_R &= ~(1 << 3);
    GPIO_PORTB_AFSEL_R |= (1 << 3);
    TIMER3_IMR_R |= TIMER_IMR_CBEIM;
    TIMER3_CTL_R |= TIMER_CTL_TBEN;
}

void TIMER3B_Handler(void){

  // YOUR CODE HERE
  // As needed, go back to review your interrupt handler code for the UART lab.
  // What are the first lines of code in the ISR? Regardless of the device, interrupt handling
  // includes checking the source of the interrupt and clearing the interrupt status bit.
  // Checking the source: test the MIS bit in the MIS register (is the ISR executing
  // because the input capture event happened and interrupts were enabled for that event?
  // Clearing the interrupt: set the ICR bit (so that same event doesn't trigger another interrupt)
  // The rest of the code in the ISR depends on actions needed when the event happens.

}

float ping_getDistance (void){

    // YOUR CODE HERE

}
