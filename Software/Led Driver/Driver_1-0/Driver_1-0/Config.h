/*

*/

#ifndef __LED_DRIVER_CONFIG_H__
#define __LED_DRIVER_CONFIG_H__

//#define		DEBUGGING_TIMESCALE // if defined, the WDT times out about 60 times as often (16ms), to turn a minute into 2 seconds.
// #define	RESET_IS_DISABLED // Define if the reset is going to be disabled

#define		VERSION_MAJOR						1
#define		VERSION_MINOR						0
#define		VERSION_RELEASE						6



#define		MAXIMUM_PATTERN_SIZE			(E2END - 16)  // Take into account the available EEPROM, minus all other configuration bytes (version 1.0 / 1.1: 10 bytes)
#define		SERIAL_BUFFER_SIZE				16  // Size of serial port communication buffers
#define		RANDOM_STACK_SIZE				16  // can be anything between 2 and whatever still fits the memory
#define		EEPROM_BLOCK_WRITE_BUFFER		16  // Minimum 16, maximum, whatever fits.

#define		EEPROM_PATTERN_HOUSENUMMASK		0xF0
#define		EEPROM_PATTERN_DELAYTIMMASK		0x0F

// Location of the mode pin (project hardware: PortA2 / Reset):
#define		PORTIN_MODE						PINA
#define		PIN_MODE						(1 << PINA2)
// if MODE pin = high random mode runs, if MODE pin = low it runs in pattern mode


#define		PORTIN_NIGHTMODE				PIND
#define		PIN_NIGHTMODE					(1<<PIND2)


// LED outputs:
#define		PORTOUT_HOUSE0					PORTB
#define		PORTIN_HOUSE0					PINB
#define		PIN_HOUSE0						(1<<PORTB7)

#define		PORTOUT_HOUSE1					PORTB
#define		PORTIN_HOUSE1					PINB
#define		PIN_HOUSE1						(1<<PORTB6)

#define		PORTOUT_HOUSE2					PORTB
#define		PORTIN_HOUSE2					PINB
#define		PIN_HOUSE2						(1<<PORTB5)

#define		PORTOUT_HOUSE3					PORTB
#define		PORTIN_HOUSE3					PINB
#define		PIN_HOUSE3						(1<<PORTB4)

#define		PORTOUT_HOUSE4					PORTB
#define		PORTIN_HOUSE4					PINB
#define		PIN_HOUSE4						(1<<PORTB3)

#define		PORTOUT_HOUSE5					PORTB
#define		PORTIN_HOUSE5					PINB
#define		PIN_HOUSE5						(1<<PORTB2)

#define		PORTOUT_HOUSE6					PORTB
#define		PORTIN_HOUSE6					PINB
#define		PIN_HOUSE6						(1<<PORTB1)

#define		PORTOUT_HOUSE7					PORTB
#define		PORTIN_HOUSE7					PINB
#define		PIN_HOUSE7						(1<<PORTB0)

#define		PORTOUT_HOUSE8					PORTD
#define		PORTIN_HOUSE8					PIND
#define		PIN_HOUSE8						(1<<PORTD6)

#define		PORTOUT_HOUSE9					PORTD
#define		PORTIN_HOUSE9					PIND
#define		PIN_HOUSE9						(1<<PORTD5)


#define		PORTIN_RANDOM_POS				PIND
#define		PIN_RANDOM_POS					(1<<PIND4)
#define		PORTIN_RANDOM_NEG				PIND
#define		PIN_RANDOM_NEG					(1<<PIND3)


// Start-up values for the port and DDR registers:
#define		DDRA_STARTUP					0x00
#define		DDRB_STARTUP					0xFF
#define		DDRD_STARTUP					0x60 | (1 << PORTD1) // PORTD1 = TXD pin, make high too.

#define		PORTA_STARTUP					0x00
// Port definitions for all pins on:
#define		PORTB_STARTUP_ALL_ON			0xFF
#define		PORTD_STARTUP_ALL_ON			0x60

#ifdef	DEBUGGING_TIMESCALE
#define		WDTCSR_STARTUP				(1<<WDIE) // Prescaled to about 16 mili second interval, enable the watchdog interrupt
#else	// DEBUGGING_TIMESCALE
#define		WDTCSR_STARTUP				(1<<WDIE)|(1<<WDP2) // Prescaled to about 1/4 second interval, enable the watchdog interrupt
#endif	// DEBUGGING_TIMESCALE

#define		TCCR0A_STARTUP					(1<<WGM01) // Select CTC Mode on OCR0A
#define		TCCR0B_STARTUP					(1<<CS01)|(1<<CS00) // Prascale by 64
#define		OCR0A_STARTUP					11 // Overflow at 115 to sample at about 1kHz (about 125 truly random numbers per second)

#define		TIMSK_STARTUP					(1<<OCIE0A)



#endif // __LED_DRIVER_USART_CMDS_H__


