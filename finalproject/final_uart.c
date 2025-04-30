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
#include "final_uart.h"
#include <stdbool.h>
#include "driverlib/interrupt.h"

// These variables are declared as examples for your use in the interrupt handler.
volatile char command_byte = -1; //  Default to no command received (-1 means nothing yet)
volatile int command_flag = 0; // flag to tell the main program a special command was received

void uart_interrupt_init(void){
    // Enable clock to GPIO port B
    SYSCTL_RCGCGPIO_R |= 0x02;

    // Enable clock to UART1
    SYSCTL_RCGCUART_R |= 0x02;

    // Wait for GPIOB and UART1 peripherals to be ready
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {};
    while ((SYSCTL_PRUART_R & 0x02) == 0) {};

    // Enable alternate functions on port B pins
    GPIO_PORTB_AFSEL_R |= 0x03;

    // Enable digital functionality on port B pins
    GPIO_PORTB_DEN_R |= 0x0F;

    // Enable UART1 Rx and Tx on port B pins
    GPIO_PORTB_PCTL_R &= 0xFFFFFF00; // Clear the bits, reset the last 8.
    GPIO_PORTB_PCTL_R |= 0x00000011; // Set bits 0 and 4 to 1.

    // Calculate baud rate
    uint16_t iBRD = 0x8; // Use equations
    uint16_t fBRD = 0x2C; // Use equations

    // Turn off UART1 while setting it up
    UART1_CTL_R &= 0xFFFFFFF0;

    // Set baud rate
    UART1_IBRD_R = iBRD;
    UART1_FBRD_R = fBRD;

    // Set frame: 8 data bits, 1 stop bit, no parity, no FIFO
    UART1_LCRH_R = 0x00000060;

    // Use system clock as source
    UART1_CC_R = 0x0;

    // Re-enable UART1 and also enable RX, TX
    UART1_CTL_R |= 0x301;

    ////// Enable interrupts

    // First clear RX interrupt flag (clear by writing 1 to ICR)
    UART1_ICR_R |= 0x10;

    // Enable RX raw interrupts in interrupt mask register
    UART1_IM_R |= 0x10;

    // NVIC setup: set priority of UART1 interrupt to 1 in bits 21-23
    NVIC_PRI1_R = (NVIC_PRI1_R & 0xFF0FFFFF) | 0x00200000;

    // NVIC setup: enable interrupt for UART1, IRQ #6, set bit 6
    NVIC_EN0_R |= 0x40;

    // Tell CPU to use ISR handler for UART1
    IntRegister(INT_UART1, UART1_Handler);

    // Globally allow CPU to service interrupts
    IntMasterEnable();
}

void uart_sendChar(char data){
    // Waiting until FIFO is not full
    while((UART1_FR_R & 0x20) != 0) {}  //  Corrected condition (checks TX Full flag)
    UART1_DR_R = data;
}

char uart_receive_nonBlocking(void) {
    return (UART1_FR_R & 0x10) ? 0 : (char) (UART1_DR_R & 0xFF);
}

void uart_sendStr(const char *data){
    // While it is not null, send characters one by one
    while(*data != '\0') {
        uart_sendChar(*data++);
    }
}

// Interrupt handler for receive interrupts
void UART1_Handler(void) {
    char byte_received;
    if (UART1_MIS_R & 0x10) {  // RX event
        UART1_ICR_R |= 0x10;   // Clear interrupt

        byte_received = (char)(UART1_DR_R & 0xFF);
        uart_sendChar(byte_received);

        // Show newline on carriage return
        if (byte_received == '\r') {
            uart_sendChar('\n');
        }

        // Allow all relevant commands to set the flag
        if (byte_received == 'g' || byte_received == 's' || byte_received == 't' ||
            byte_received == 'w' || byte_received == 'a' || byte_received == 'd' ||
            byte_received == 'x' || byte_received == 'h' || byte_received == 'i' ||
            byte_received == 'j' || byte_received == 'k' || byte_received == 'l' ||
            byte_received == '\r' || byte_received == '\n' ||
            byte_received == '0' || byte_received == '1' || byte_received == '2' || byte_received == '3' || byte_received == '4'  || byte_received == '5' || byte_received == '6' || byte_received == '7'  || byte_received == '8' || byte_received == '9'
            ) {
            command_byte = byte_received;
            command_flag = 1;
        }
    }
}

