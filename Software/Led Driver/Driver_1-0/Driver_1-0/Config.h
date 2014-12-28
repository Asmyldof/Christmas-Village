/*

*/

// Project Hardware set up as follows:
// PORTB = output 1 through 8
// PORTD6 = output 9
// PORTD5 = output 10
// PORTD4 = Random Bitstream Positive
// PORTD3 / INT1 = Random Bitstream Negative
// PORTD2 / INT0 = Night Mode input
// PORTD0 / RXD = Serial interface
// PORTD1 / TXD = Serial interface
// PORTA2 / RESET = Random / Pattern select
//
// Crystal = 7.3728 MHz
// Serial Baud = 115.2k
// U2X = 0
// UBRR = 3
// Set to: 1 start bit, 2 stop bits, odd parity, 8 data bits

#ifndef __LED_DRIVER_CONFIG_H__
#define __LED_DRIVER_CONFIG_H__

//#define		DEBUGGING_TIMESCALE // if defined, the WDT times out about 60 times as often (16ms), to turn a minute into 2 seconds.
//#define		RESET_IS_DISABLED // Define if the reset is going to be disabled
#define		STORE_FLAGBYTES_IN_GPIOR // Store the flag bytes in General Purpose Registers, for efficiency
#define		STORE_PACKETOUTCOUNT_INGPIOR2 // Store the last random in GPIOR2, for efficiency (250 byte code size reduction)

#define		VERSION_MAJOR						1
#define		VERSION_MINOR						1
#define		VERSION_RELEASE						11

#define		PROJECT_SIGNATURE_HIGH				0x80 // MSB high, publicly available project 
//   (means care must be taken with a potential future Asmyldof Suite when a project is "compiled at home")
//   Remaining bits signify stuff that is not decided yet, possibly internal differentiations between project types
//   for non-open-source (MSB zero) embedded devices.

#define		PROJECT_SIGNATURE_MID				0x00 // Mid & Low together make an incremental 16bit project number for each MSB category
#define		PROJECT_SIGNATURE_LOW				0x03



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
#define		WDTCSR_STARTUP				( (1<<WDIE)|(1<<WDP2) ) // Prescaled to about 1/4 second interval, enable the watchdog interrupt
#endif	// DEBUGGING_TIMESCALE

#define		TCCR0A_STARTUP					(1<<WGM01) // Select CTC Mode on OCR0A
#define		TCCR0B_STARTUP					( (1<<CS01)|(1<<CS00) ) // Prascale by 64
#define		OCR0A_STARTUP					11 // Overflow at 115 to sample at about 1kHz (about 125 truly random numbers per second)

#define		TIMSK_STARTUP					(1<<OCIE0A)

#define		UCSRA_STARTUP					0//(1<<U2X) // Don't enable high speed mode, 460.8 kBaud (circa 40kbyte/s?) is more than enough
#define		UCSRB_STARTUP					( (1<<RXCIE)|(1<<RXEN) ) // interrupt on RX, Enable RX continuously (the RX interrupt will check what we receive makes sense), UDRIE will be enabled when starting TX
// In order: enable parity: odd (UPM1:0 = 11), set 2 stop bits (USBS = 1), set to 8bit (UCSZ2:0 = 011) - Note UCSZ2 is in UCSRB when 9 bit seems like a good idea 
#define		UCSRC_STARTUP					( (1<<UPM1)|(1<<UPM0)|(1<<USBS)|(1<<UCSZ1)|(1<<UCSZ0) )

#define		TXPIN_PORT						PORTD
#define		TXPIN_DDR						DDRD
#define		TXPIN_PIN						(1<<PORTD1)

#endif // __LED_DRIVER_USART_CMDS_H__


