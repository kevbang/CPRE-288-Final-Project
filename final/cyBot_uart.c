#include "cyBot_uart.h"
#include <inc/tm4c123gh6pm.h>

void cyBot_uart_init(void) {
    SYSCTL_RCGCGPIO_R |= 0x02; // Enable clock to port B
    SYSCTL_RCGCUART_R |= 0x02; // Enable clock to UART1

    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {};
    while ((SYSCTL_PRUART_R & 0x02) == 0) {};

    GPIO_PORTB_AFSEL_R |= 0x03;    // PB0 and PB1 for UART1
    GPIO_PORTB_PCTL_R &= ~0x000000FF;
    GPIO_PORTB_PCTL_R |= 0x00000011;
    GPIO_PORTB_DEN_R |= 0x03;
    GPIO_PORTB_DIR_R |= 0x02;
    GPIO_PORTB_DIR_R &= ~0x01;

    UART1_CTL_R &= ~UART_CTL_UARTEN;
    UART1_IBRD_R = 8;
    UART1_FBRD_R = 44;
    UART1_LCRH_R = UART_LCRH_WLEN_8;
    UART1_CC_R = UART_CC_CS_SYSCLK;
    UART1_CTL_R |= UART_CTL_RXE | UART_CTL_TXE | UART_CTL_UARTEN;
}

void cyBot_sendByte(char data) {
    while ((UART1_FR_R & UART_FR_TXFF) != 0); // Wait if FIFO full
    UART1_DR_R = data;
}

char cyBot_getByte_blocking(void) {
    while (UART1_FR_R & UART_FR_RXFE); // Wait until RX not empty
    return (char)(UART1_DR_R & 0xFF);
}
