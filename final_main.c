/**
 * Main file for CPRE 2880 Final Project
 * @author Kevin Tran
 *
 */

#include "Timer.h"
#include "lcd.h"
#include "servo.h"
#include "final_uart.h"


// Uncomment or add any include directives that are needed
#warning "Possible unimplemented functions"
#define REPLACEME 0



int main(void) {
	timer_init(); // Must be called before lcd_init(), which uses timer functions
	lcd_init();

	uart_init();


	while(1) {
	    char input = uart_receive();

	    if(input == 'k') {
	       uart_sendChar('z');
	       uart_sendStr("Works");
	    }

	}





	return 0;


}


