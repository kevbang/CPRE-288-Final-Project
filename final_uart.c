/*
*
*   final_uart.c
*
*
*
*   @author Kevin Tran
*   @date 4/24/2025
*
*   [pg. ###] = Page number in Tiva Datasheet.
*/

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include "uart-interrupt.h"
#include <stdbool.h>
#include "driverlib/interrupt.h"

void uart_init(void) {

    SYSCTL_RCGCGPIO_R |= 0x02; // Enable clock to GPIO Port B

    SYSCTL_RCGCUART_R |= 0x02; // Enable clock to UART1

    // Wait for GPIOB and UART1 peripherals to be ready
    while((SYSCTL_PRGPIO_R & 0x02) == 0) {};
    while((SYSCTL_PRUART_R & 0x02) == 0) {};

    // Enable alternate functions on Port B pins. [pg. 672]
    GPIO_PORTB_AFSEL_R |= 0x03;

    // Enable digital functionality on Port B pins. [pg. 683]
    GPIO_PORTB_DEN_R |= 0x0F;

    // Enable UART1 RX and TX on Port B pins. [pg. 689]
    GPIO_PORTB_PCTL_R &= 0xFFFFFF00; // Clear the first 8 bits
    GPIO_PORTB_PCTL_R |= 0x00000011; // Set bits 0-7 to (1) enable Pin 0 and Pin 1.

    // Turn off UART1 while setting it up
    UART1_CTL_R &= 0xFFFFFFF0;

    // Set Baud Rate (assuming lab baud rate)
    uint16_t iBRD = 0x8;
    uint16_t fBRD = 0x2C;

    UART1_IBRD_R = iBRD;
    UART1_FBRD_R = fBRD;

    // Set serial parameters. [pg. 916]

    UART1_LCRH_R = 0x70; // Enable FIFO mode, and 8 bits

    UART1_CC_R = 0x0; // Use system clock

    // Re-enable UART1 and also enable RX, TX
    UART1_CTL_R |= 0x301;

}

// Tested
void uart_sendChar(char data) {
    while((UART1_FR_R & 0x20) != 0) {};

    UART1_DR_R = data;

}

// Tested
char uart_receive(void) {
    return (UART1_FR_R & 0x10) ? 0 : (char) (UART1_DR_R & 0xFF);
}

// Tested
void uart_sendStr(const char *data) {
    // While it is not null, send characters one by one
    while(*data != '\0') {
        uart_sendChar(*data++);
    }
}




