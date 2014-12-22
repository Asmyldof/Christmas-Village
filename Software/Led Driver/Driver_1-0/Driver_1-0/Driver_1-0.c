/*
 * Driver_1_0.c
 *
 * Created: 22-12-2014 20:54:38
 *  Author: Robert
 */ 
// PORTB = output 1 through 8
// PORTD6 = output 9
// PORTD5 = output 10
// PORTD4 = Random Bitstream Positive
// PORTD3 / INT1 = Random Bitstream Negative
// PORTD2 / INT0 = Night Mode input
// PORTD0 / RXD = Serial interface
// PORTD1 / TXD = Serial interface
// PORTA2 / RESET = Random / Pattern select

#include <avr/io.h>
#include <avr/interrupt.h>

int main(void)
{
    
	DDRB = 0xFF;
	DDRD = 0x60;
	
	PORTB = 0xFF;
	PORTD = 0x60;
	
	while(1)
    {
        //TODO:: Please write your application code 
    }
}