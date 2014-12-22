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
#include <avr/eeprom.h>

// Default EEPROM Preloads:
#define		DEF_DAY_TIME_TARGET_NUMBER_HOUSES	8
#define		DEF_NIGHT_TIME_TARGET_NUMBER_HOUSES	2
#define		DEF_MINIMUM_NUMBER_OF_HOUSES		0
#define		DEF_MAIN_STEPDELAY					25 // number of 1s WDT interrupts per step in pattern
#define		DEF_RANDOM_STEP_DELAY				60 // number of 1s WDT interrupts per step in random
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

#define		MAXIMUM_PATTERN_SIZE			100  // Take into account the stack and 8 bytes of RAM used in general

#define		EEPROM_PATTERN_HOUSENUMMASK		0xF0
#define		EEPROM_PATTERN_DELAYTIMMASK		0x0F


#define		DDRA_STARTUP					0x00
#define		DDRB_STARTUP					0xFF
#define		DDRD_STARTUP					0x60

#define		PORTA_STARTUP					0x00
#define		PORTB_STARTUP					0xFF
#define		PORTD_STARTUP					0x60

#define		INITIAL_HOUSE_COUNT				10 // number of houses that is on at startup (as defined above on the port)

uint8_t EEMEM	EE_TargetNumberHousesDayTime = DEF_DAY_TIME_TARGET_NUMBER_HOUSES;
uint8_t EEMEM	EE_MinimumNumberHouses = DEF_MINIMUM_NUMBER_OF_HOUSES;
uint8_t EEMEM	EE_TargetNumberHousesNightTime = DEF_NIGHT_TIME_TARGET_NUMBER_HOUSES;
uint8_t EEMEM	EE_PatternStepCount = DEF_PATTERN_STEP_COUNT;
uint8_t	EEMEM	EE_MainStepDelay = DEF_MAIN_STEPDELAY;
uint8_t EEMEM	EE_RandomStepDelay = DEF_RANDOM_STEP_DELAY;
uint8_t EEMEM	EE_PatternData[MAXIMUM_PATTERN_SIZE] = DEF_PATTERN;

uint8_t	RAM_TargetNumberHousesDayTime;
uint8_t RAM_MinimumNumberHouses;
uint8_t RAM_TargetNumberHousesNightTime;
uint8_t RAM_PatternStepCount;
uint8_t RAM_PatternData[MAXIMUM_PATTERN_SIZE];
uint8_t	RAM_MainStepDelay;
uint8_t RAM_RandomStepDelay;


int main(void)
{
	// Read the configuration bytes:
	RAM_TargetNumberHousesDayTime = eeprom_read_byte(&EE_TargetNumberHousesDayTime);
	RAM_MinimumNumberHouses = eeprom_read_byte(&EE_MinimumNumberHouses);
	RAM_TargetNumberHousesNightTime = eeprom_read_byte(&EE_TargetNumberHousesNightTime);
	RAM_PatternStepCount = eeprom_read_byte(&EE_PatternStepCount);
	RAM_MainStepDelay = eeprom_read_byte(&EE_MainStepDelay);
	RAM_RandomStepDelay = eeprom_read_byte(&EE_RandomStepDelay);
	
	// Read the pattern data, up to the number of steps in the pattern:
	eeprom_read_block((void *) RAM_PatternData, &EE_PatternData, RAM_PatternStepCount);
	
	
	
	DDRB = DDRB_STARTUP;
	DDRD = DDRD_STARTUP;
	
	PORTB = PORTB_STARTUP;
	PORTD = PORTD_STARTUP;
	
	
	
	
	
	while(1)
    {
        //TODO:: Please write your application code 
    }
}