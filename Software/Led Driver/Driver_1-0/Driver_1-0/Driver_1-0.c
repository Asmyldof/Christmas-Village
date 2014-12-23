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
// 
// Crystal = 7.3728 MHz
// Serial Baud = 115.2k
// U2X = 0
// UBRR = 3

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#define		DEBUGGING_TIMESCALE // if defined, the WDT times out about 60 times as often (16ms), to turn a minute into 2 seconds.

// Default EEPROM Preloads:
#define		DEF_DAY_TIME_TARGET_NUMBER_HOUSES	8 // during the day the device will slowly increase to this number
#define		DEF_NIGHT_TIME_TARGET_NUMBER_HOUSES	2 // at night time mode the device will slowly decrease to this number
#define		DEF_MINIMUM_NUMBER_OF_HOUSES		0 // used when trying to set a number of houses with serial interface
#define		DEF_MAIN_STEPDELAY					25 // number of 1s WDT interrupts per step in pattern
#define		DEF_RANDOM_STEP_DELAY				60 // number of 1s WDT interrupts per step in random
#define		DEF_NIGHTTIME_TICKS					10 // number of RandomSteps between target number houses decrements

#define		DEF_PATTERN_STEP_COUNT				90
// in the pattern, the first nibble is the house to toggle, the second nibble is the post-delay steps untill the
//   next action is taken.
#define		DEF_PATTERN							{	0x11, 0xAF, 0x06, 0xA3, 0x21, 0x02, 0x95, 0x11, 0x50, 0x13, \
													0x45, 0x1B, 0x23, 0x54, 0xA2, 0x15, 0x86, 0x27, 0x19, 0x28, \
													0x23, 0x71, 0x44, 0x26, 0x63, 0x66, 0xA7, 0x36, 0x98, 0x7F, \
													0x87, 0x05, 0x38, 0x97, 0x77, 0x83, 0x18, 0x43, 0x56, 0x5C, \
													0x43, 0x24, 0x72, 0x4A, 0x23, 0x49, 0x39, 0x50, 0x42, 0x96, \
													0x51, 0x59, 0x94, 0x63, 0x15, 0x50, 0x81, 0x67, 0x74, 0x22, \
													0x70, 0x88, 0x3C, 0x72, 0x83, 0x9A, 0x02, 0x72, 0x39, 0x47, \
													0x93, 0xA1, 0xA4, 0x38, 0x92, 0xAE, 0x13, 0x8D, 0x24, 0x90, \
													0x17, 0x12, 0x27, 0x16, 0x31, 0x24, 0x34, 0x95, 0xA3, 0x62, \
													}

#define		MAXIMUM_PATTERN_SIZE			95  // Take into account the stack and 8 bytes of RAM used in general

#define		EEPROM_PATTERN_HOUSENUMMASK		0xF0
#define		EEPROM_PATTERN_DELAYTIMMASK		0x0F

// Location of the mode pin (project hardware: PortA2 / Reset):
#define		PORTIN_MODE						PINA
#define		PIN_MODE						(1 << PINA2)
// if MODE pin = high random mode runs, if MODE pin = low it runs in pattern mode

// PORTB = output 1 through 8
// PORTD6 = output 9
// PORTD5 = output 10
// PORTD4 = Random Bitstream Positive
// PORTD3 / INT1 = Random Bitstream Negative
// PORTD2 / INT0 = Night Mode input
// PORTD0 / RXD = Serial interface
// PORTD1 / TXD = Serial interface
// PORTA2 / RESET = Random / Pattern select
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
#define		PORTB_STARTUP					0xFF
#define		PORTD_STARTUP					0x60

#ifdef	DEBUGGING_TIMESCALE
	#define		WDTCSR_STARTUP				(1<<WDIE) // Prescaled to about 16 mili second interval, enable the watchdog interrupt
#else	// DEBUGGING_TIMESCALE
	#define		WDTCSR_STARTUP				(1<<WDIE)|(1<<WDP2)|(1<<WDP1) // Prescaled to about 1 second interval, enable the watchdog interrupt
#endif	// DEBUGGING_TIMESCALE

#define		TCCR0A_STARTUP					(1<<WGM01) // Select CTC Mode on OCR0A
#define		TCCR0B_STARTUP					(1<<CS01)|(1<<CS00) // Prascale by 64
#define		OCR0A_STARTUP					115 // Overflow at 115 to sample at about 1kHz

#define		TIMSK_STARTUP					(1<<TOIE0)


/*
 * Internal Helper Functions:
 */
inline static void SetHouseOn(uint8_t HouseNumber);
inline static void SetHouseOff(uint8_t HouseNumber);


uint8_t EEMEM	EE_TargetNumberHousesDayTime = DEF_DAY_TIME_TARGET_NUMBER_HOUSES;
uint8_t EEMEM	EE_MinimumNumberHouses = DEF_MINIMUM_NUMBER_OF_HOUSES;
uint8_t EEMEM	EE_TargetNumberHousesNightTime = DEF_NIGHT_TIME_TARGET_NUMBER_HOUSES;
uint8_t EEMEM	EE_PatternStepCount = DEF_PATTERN_STEP_COUNT;
uint8_t	EEMEM	EE_MainStepDelay = DEF_MAIN_STEPDELAY;
uint8_t EEMEM	EE_RandomStepDelay = DEF_RANDOM_STEP_DELAY;
uint8_t EEMEM	EE_PatternData[MAXIMUM_PATTERN_SIZE] = DEF_PATTERN;
uint8_t	EEMEM	EE_PostDelayTicksNightTime = DEF_NIGHTTIME_TICKS;

uint8_t RAM_PatternStepCount;
uint8_t RAM_PatternData[MAXIMUM_PATTERN_SIZE];

uint8_t	WDTPostDecrement;
uint8_t LastRandom;
uint8_t	TempRandom;
uint8_t	RandomBitCount;
uint8_t	CurrentTargetHouses;
uint8_t	NightTimeDecreaseDecounter;

int main(void)
{
	// Read the configuration bytes:
	RAM_PatternStepCount = eeprom_read_byte(&EE_PatternStepCount);
	
	// Read the pattern data, up to the number of steps in the pattern:
	eeprom_read_block((void *) RAM_PatternData, &EE_PatternData, RAM_PatternStepCount);
	
	CurrentTargetHouses = eeprom_read_byte(&EE_TargetNumberHousesDayTime);
	
	// high = random mode, low = pattern
	if( (PIN_MODE & PORTIN_MODE) == PIN_MODE )
		WDTPostDecrement = eeprom_read_byte(&EE_RandomStepDelay);
	else
		WDTPostDecrement = eeprom_read_byte(&EE_MainStepDelay);
	
	DDRB = DDRB_STARTUP;
	DDRD = DDRD_STARTUP;
	
	PORTB = PORTB_STARTUP;
	PORTD = PORTD_STARTUP;
	
	LastRandom = 0x00;
	TempRandom = 0x00;
	RandomBitCount = 0x00;
	
	WDTCR = WDTCSR_STARTUP;
	
	OCR0A = OCR0A_STARTUP;
	TCCR0A = TCCR0A_STARTUP;
	TCCR0B = TCCR0B_STARTUP;
	
	TIMSK = TIMSK_STARTUP;
	
	
	
	sei();
	
	while(1)
    {
        //TODO:: Please write your application code 
    }
}


ISR(WDT_OVERFLOW_vect)
{
	uint8_t	NumberOfHouses, temp;
	
	NumberOfHouses = 0;
	
	WDTPostDecrement--;
	
	if( WDTPostDecrement == 0)
	{
		// action!:
		// Check the mode we are running in, to determine what action to take:
		if( (PIN_MODE & PORTIN_MODE) == PIN_MODE )
		{ //  If high, we run in random mode
			// first of all: load the tick delay:
			WDTPostDecrement = eeprom_read_byte(&EE_RandomStepDelay);
			
			if( (PIN_NIGHTMODE & PORTIN_NIGHTMODE) == PIN_NIGHTMODE )
			{ // if we're in night mode:
				temp = eeprom_read_byte(&EE_TargetNumberHousesNightTime);
				if( CurrentTargetHouses > temp )
				{ // and the current number of target houses is still larger than the desired nuber
					NightTimeDecreaseDecounter--; // decrease the ticker
					if( NightTimeDecreaseDecounter == 0 )
					{ // if the ticker reached zero:
						CurrentTargetHouses--; // decrease the target house number by one and reset the ticker:
						NightTimeDecreaseDecounter = eeprom_read_byte(&EE_PostDelayTicksNightTime);
					}
				}
			}
			else
			{
				temp = eeprom_read_byte(&EE_TargetNumberHousesDayTime);
				if( CurrentTargetHouses < temp )
				{
					NightTimeDecreaseDecounter--;
					if( NightTimeDecreaseDecounter == 0 )
					{
						CurrentTargetHouses++;
						NightTimeDecreaseDecounter = eeprom_read_byte(&EE_PostDelayTicksNightTime);
					}
				}
			}
			
			// then count the current number of houses (just walk through the defines, progmem aplenty:
			if( (PORTIN_HOUSE0 & PIN_HOUSE0) == PIN_HOUSE0)
				NumberOfHouses++;
			if( (PORTIN_HOUSE1 & PIN_HOUSE1) == PIN_HOUSE1)
				NumberOfHouses++;
			if( (PORTIN_HOUSE2 & PIN_HOUSE2) == PIN_HOUSE2)
				NumberOfHouses++;
			if( (PORTIN_HOUSE3 & PIN_HOUSE3) == PIN_HOUSE3)
				NumberOfHouses++;
			if( (PORTIN_HOUSE4 & PIN_HOUSE4) == PIN_HOUSE4)
				NumberOfHouses++;
			if( (PORTIN_HOUSE5 & PIN_HOUSE5) == PIN_HOUSE5)
				NumberOfHouses++;
			if( (PORTIN_HOUSE6 & PIN_HOUSE6) == PIN_HOUSE6)
				NumberOfHouses++;
			if( (PORTIN_HOUSE7 & PIN_HOUSE7) == PIN_HOUSE7)
				NumberOfHouses++;
			if( (PORTIN_HOUSE8 & PIN_HOUSE8) == PIN_HOUSE8)
				NumberOfHouses++;
			if( (PORTIN_HOUSE9 & PIN_HOUSE9) == PIN_HOUSE9)
				NumberOfHouses++;
			
			temp = (LastRandom & 0x0F);
			// first: Let LastRandom (4 bit / lower nibble as generated) determine
			// whether we compare larger than maximum houses, or larger equal maximum
			// (even greater range of random behaviour)
			if( temp > 9 )
			{
				temp -= 6; // lower nibble has a maximum of 15, so when larger than 9, subtracting 6 will 
							// again give a full range. Fun thing about true random: you can do what you want
							// mathematically and it'll stay random.
				if( NumberOfHouses > CurrentTargetHouses )
				{
					SetHouseOff(temp);
				}
				else
				{
					SetHouseOn(temp);
				}
			}
			else
			{
				if( NumberOfHouses >= CurrentTargetHouses )
				{
					SetHouseOff(temp);
				}
				else
				{
					SetHouseOn(temp);
				}
			}
		}
		else
		{ // else we run in pattern mode
			WDTPostDecrement = eeprom_read_byte(&EE_MainStepDelay);
			// pattern stuff	(TODO)
		}
		
	}
	
	// We can just re-enable the interrupt here, since none of the mentioned risks really apply to us:
	WDTCR |= (1<WDIE);
}


/*
 * Interrupt of Timer0, configured to trigger at an interval by defines.
 *    at each interrupt the random bit inputs will read
 */
ISR(TIMER0_OVF_vect)
{
	
	// First move up the current temprandom by one bit:
	TempRandom = TempRandom << 1;
	
	// To eliminate all bias from the random numbers, invert the behaviour on the odd bytes
	// (the temperature dependent drift between the WDT and crystal will take care of random
	//   byte selection)
	if( (RandomBitCount & 0x08) == 0x08 )
	{ // odd byte:
		if( (RandomBitCount & 0x01) == 0x01 )
		{ // odd bits come from positive:
			if( (PORTIN_RANDOM_POS & PIN_RANDOM_POS) == PIN_RANDOM_POS)
				TempRandom |= 0x01;
		}
		else
		{ // even bits come from negative
			if( (PORTIN_RANDOM_NEG & PIN_RANDOM_NEG) == PIN_RANDOM_NEG)
				TempRandom |= 0x01;
		}	
	} 
	else
	{ // even byte:
		if( (RandomBitCount & 0x01) == 0x01 )
		{ // odd bits come from negative:
			if( (PORTIN_RANDOM_NEG & PIN_RANDOM_NEG) == PIN_RANDOM_NEG)
				TempRandom |= 0x01;
		}
		else
		{ // even bits come from positive:
			if( (PORTIN_RANDOM_POS & PIN_RANDOM_POS) == PIN_RANDOM_POS)
				TempRandom |= 0x01;
		}
	}
	
	RandomBitCount++; // increase bit count
	
	if( (RandomBitCount & 0x08) == 0x08 )
	{
		// Reached a full byte here
		LastRandom = TempRandom;
		TempRandom = 0;
		// don't reset counter, to keep track of odd/even bytes...
	}
	// (( if is 2 bytes less than else if and they do the same: ))
	if( (RandomBitCount & 0x10) == 0x10 )
	{
		// Reached a full byte here, but also a reset moment for the counter:
		LastRandom = TempRandom;
		TempRandom = 0;
		RandomBitCount = 0;
	}
}





/*
 Set the house pointed to on, if not in range, choose 0 (checks, if wanted, should be done before calling)
*/
inline static void SetHouseOn(uint8_t HouseNumber)
{
	switch(HouseNumber)
	{
		case 0:
		default:
			PORTOUT_HOUSE0 |= PIN_HOUSE0;
			break;
		case 1:
			PORTOUT_HOUSE1 |= PIN_HOUSE1;
			break;
		case 2:
			PORTOUT_HOUSE2 |= PIN_HOUSE2;
			break;
		case 3:
			PORTOUT_HOUSE3 |= PIN_HOUSE3;
			break;
		case 4:
			PORTOUT_HOUSE4 |= PIN_HOUSE4;
			break;
		case 5:
			PORTOUT_HOUSE5 |= PIN_HOUSE5;
			break;
		case 6:
			PORTOUT_HOUSE6 |= PIN_HOUSE6;
			break;
		case 7:
			PORTOUT_HOUSE7 |= PIN_HOUSE7;
			break;
		case 8:
			PORTOUT_HOUSE8 |= PIN_HOUSE8;
			break;
		case 9:
			PORTOUT_HOUSE9 |= PIN_HOUSE9;
			break;
	}
}


/*
 Set the house pointed to off, if not in range, choose 0 (checks, if wanted, should be done before calling)
*/
inline static void SetHouseOff(uint8_t HouseNumber)
{
	switch(HouseNumber)
	{
		case 0:
		default:
			PORTOUT_HOUSE0 &= ~PIN_HOUSE0;
			break;
		case 1:
			PORTOUT_HOUSE1 &= ~PIN_HOUSE1;
			break;
		case 2:
			PORTOUT_HOUSE2 &= ~PIN_HOUSE2;
			break;
		case 3:
			PORTOUT_HOUSE3 &= ~PIN_HOUSE3;
			break;
		case 4:
			PORTOUT_HOUSE4 &= ~PIN_HOUSE4;
			break;
		case 5:
			PORTOUT_HOUSE5 &= ~PIN_HOUSE5;
			break;
		case 6:
			PORTOUT_HOUSE6 &= ~PIN_HOUSE6;
			break;
		case 7:
			PORTOUT_HOUSE7 &= ~PIN_HOUSE7;
			break;
		case 8:
			PORTOUT_HOUSE8 &= ~PIN_HOUSE8;
			break;
		case 9:
			PORTOUT_HOUSE9 &= ~PIN_HOUSE9;
			break;
	}
}