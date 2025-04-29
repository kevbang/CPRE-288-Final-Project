#include "open_interface.h"
#include "Timer.h"
#include "cyBot_uart.h"
#include <stdlib.h>
#include <inc/tm4c123gh6pm.h>

oi_t* oi_alloc(void) {
    return (oi_t*) malloc(sizeof(oi_t));
}

void oi_free(oi_t* self) {
    free(self);
}

void oi_init(oi_t* self) {
    // Initialize UART4 for CyBot communication
    SYSCTL_RCGCGPIO_R |= 0x04;  // Enable clock to Port C
    SYSCTL_RCGCUART_R |= 0x10;  // Enable clock to UART4

    while ((SYSCTL_PRGPIO_R & 0x04) == 0) {};
    while ((SYSCTL_PRUART_R & 0x10) == 0) {};

    GPIO_PORTC_AFSEL_R |= 0x30;        // PC4, PC5 alternate function
    GPIO_PORTC_PCTL_R &= ~0x00FF0000;
    GPIO_PORTC_PCTL_R |= 0x00110000;   // UART4 on PC4 (RX) and PC5 (TX)
    GPIO_PORTC_DEN_R |= 0x30;          // Enable digital on PC4, PC5
    GPIO_PORTC_DIR_R |= 0x20;           // PC5 output (TX)
    GPIO_PORTC_DIR_R &= ~0x10;          // PC4 input (RX)

    UART4_CTL_R &= ~UART_CTL_UARTEN;    // Disable UART
    UART4_IBRD_R = 8;                   // 115200 baud (assuming 16MHz)
    UART4_FBRD_R = 44;
    UART4_LCRH_R = UART_LCRH_WLEN_8;    // 8 data bits, no parity, 1 stop bit
    UART4_CC_R = UART_CC_CS_SYSCLK;     // Use system clock
    UART4_CTL_R |= UART_CTL_RXE | UART_CTL_TXE | UART_CTL_UARTEN; // Enable UART

    timer_waitMillis(500);

    cyBot_sendByte(128);  // Start command
    timer_waitMillis(20);
    cyBot_sendByte(131);  // Safe mode
    timer_waitMillis(20);

    self->distance = 0;
    self->angle = 0;
}

void oi_update(oi_t* self) {
    // Stub for updating sensors if needed
}

void oi_setWheels(int16_t right_wheel, int16_t left_wheel) {
    cyBot_sendByte(145);
    cyBot_sendByte((right_wheel >> 8) & 0xFF);
    cyBot_sendByte(right_wheel & 0xFF);
    cyBot_sendByte((left_wheel >> 8) & 0xFF);
    cyBot_sendByte(left_wheel & 0xFF);
}

void oi_resetDistance(oi_t* self) {
    self->distance = 0;
}

void oi_resetAngle(oi_t* self) {
    self->angle = 0;
}

double oi_getDistance(oi_t* self) {
    return self->distance;
}

double oi_getAngle(oi_t* self) {
    return self->angle;
}

void oi_readCliffSensors(uint8_t* left, uint8_t* front_left, uint8_t* front_right, uint8_t* right) {
    cyBot_sendByte(142); cyBot_sendByte(9);  *left = cyBot_getByte_blocking();
    cyBot_sendByte(142); cyBot_sendByte(10); *front_left = cyBot_getByte_blocking();
    cyBot_sendByte(142); cyBot_sendByte(11); *front_right = cyBot_getByte_blocking();
    cyBot_sendByte(142); cyBot_sendByte(12); *right = cyBot_getByte_blocking();
}

void oi_close(void) {
    oi_setWheels(0, 0);
}
