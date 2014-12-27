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
#include "USART-Commands.h"
#include "EEPROM-Defaults.h"
#include "Config.h"
#include "Flags.h" // normally included through others as well, but since this project has well designed headers, that's no problem, and this adds clarity.


/*
 * Internal Helper Functions:
 */
inline static void SetHouseOn(uint8_t HouseNumber);
inline static void SetHouseOff(uint8_t HouseNumber);
inline static void ProcessRandomHouseToggle();
inline static void ProcessPatternStep();
inline static void SetNoEffectOutputs();
inline static void StopUSARTTX();
inline static void StartUSARTTX();
inline static void HandleLastCommand();

uint8_t EEMEM	EE_TargetNumberHousesDayTime = DEF_DAY_TIME_TARGET_NUMBER_HOUSES;
uint8_t EEMEM	EE_MinimumNumberHouses = DEF_MINIMUM_NUMBER_OF_HOUSES;
uint8_t EEMEM	EE_TargetNumberHousesNightTime = DEF_NIGHT_TIME_TARGET_NUMBER_HOUSES;
uint8_t EEMEM	EE_PatternStepCount = DEF_PATTERN_STEP_COUNT;
uint8_t	EEMEM	EE_MainStepDelay = DEF_MAIN_STEPDELAY;
uint8_t EEMEM	EE_RandomStepDelay = DEF_RANDOM_STEP_DELAY;
uint8_t EEMEM	EE_PatternData[MAXIMUM_PATTERN_SIZE] = DEF_PATTERN;
uint8_t	EEMEM	EE_PostDelayTicksNightTime = DEF_NIGHTTIME_TICKS;
uint8_t EEMEM	EE_StartupDelay = DEF_STARTUP_DELAY;
uint8_t EEMEM	EE_MainFlags = DEF_MAINFLAGS;

uint8_t	WDTPostDecrement; // Decrementer for the WDT interrupt
uint8_t LastRandom; // Random number, updated at a constant interval (standard config about 125 unique random bytes per second)
uint8_t	TempRandom; // Temporary memory for creating the random number
uint8_t	RandomBitCount; // Counter for the random generator
uint8_t	CurrentTargetHouses; // Number of houses we want turned on at present
uint8_t	NightTimeDecreaseDecounter; // Decrementer for the slow increase or decrease of target house numbers
uint8_t	RandomStack[RANDOM_STACK_SIZE];
uint8_t	EEPROMBlockWriteBuffer[EEPROM_BLOCK_WRITE_BUFFER];

uint8_t	CurrentPatternIndex; // pointer byte to the index of the pattern data
//uint8_t SerialByteCount; //
//uint8_t SerialInBuf_Index; // 
//uint8_t SerialOutBuf_Index; // 

uint8_t	MainFlags; // Byte of main functionality flags
uint8_t OpFlags; // Operational flags, only read-able externally

uint8_t LastCommand;
uint8_t CurrentUSARTPacketInCount;
uint8_t CurrentUSARTPacketOutCount;
//uint8_t SerialPacketPayloadLength; // 
uint8_t	TargetUSARTPacketInCount;
uint8_t SerialInBuffer[SERIAL_BUFFER_SIZE]; // Serial port read (RX) buffer
uint8_t SerialOutBuffer[SERIAL_BUFFER_SIZE]; // Serial port send (TX) buffer



int main(void)
{	
	// Pattern Data will be handled directly from EE from now on:
	// Read the configuration bytes:
	//RAM_PatternStepCount = eeprom_read_byte(&EE_PatternStepCount);
	//
	// Read the pattern data, up to the number of steps in the pattern:
	//eeprom_read_block((void *) RAM_PatternData, (const void*) EE_PatternData, MAXIMUM_PATTERN_SIZE);
	
	CurrentTargetHouses = eeprom_read_byte(&EE_TargetNumberHousesDayTime);
	
	// set startup delay
	WDTPostDecrement = eeprom_read_byte(&EE_StartupDelay);

	NightTimeDecreaseDecounter = eeprom_read_byte(&EE_PostDelayTicksNightTime);
	
	MainFlags = eeprom_read_byte(&EE_MainFlags);
	
	
	DDRB = DDRB_STARTUP;
	DDRD = DDRD_STARTUP;
	
	SetNoEffectOutputs();
	
	LastRandom = 0x00;
	TempRandom = 0x00;
	RandomBitCount = 0x00;
	OpFlags = 0x00;
	LastCommand = 0x00;
	CurrentUSARTPacketInCount = 0x00;
	CurrentUSARTPacketOutCount = 0x00;
	
	WDTCR = WDTCSR_STARTUP;
	
	OCR0A = OCR0A_STARTUP;
	TCCR0A = TCCR0A_STARTUP;
	TCCR0B = TCCR0B_STARTUP;
	
	TIMSK = TIMSK_STARTUP;
	
	sei();
	
	while(1)
    {
		// Semaphore system has been shelved for now, since it seems to be optimised away, no matter
		// what kind of, or how many empty assembler directives I sprinkle throughout the code:
		
		/*
		if( (MainFlags & FLAG_PROCESS_PATTERNSTEP) == FLAG_PROCESS_PATTERNSTEP )
		{	// If the flag to process a patternstep is set:
			asm __volatile__ (""); // prevent compiler optimisation
			// (for some reason the compiler thinks the inline functions are ineffectual?!)
			MainFlags &= ~FLAG_PROCESS_PATTERNSTEP; // unset the flag
			ProcessPatternStep();// process a patternstep
		}
		if( (MainFlags & FLAG_PROCESS_RANDOMHOUSE) == FLAG_PROCESS_RANDOMHOUSE )
		{
			asm __volatile__ (""); // prevent compiler optimisation
			//PORTB = LastRandom;
			MainFlags &= ~FLAG_PROCESS_RANDOMHOUSE;
			ProcessRandomHouseToggle();
		}
		//PORTB = LastRandom;
		//*/
		if( ( OpFlags & FLAG_OP_USART_HANDLE_COMMAND ) == FLAG_OP_USART_HANDLE_COMMAND )
		{
			HandleLastCommand();	
		}
    }
}







/* upon completed receipt of a byte: */
ISR(USART0_RX_vect)
{
	uint8_t	USART_Status = UCSRA;
	uint8_t CurrentByte = UDR;
	
	if( ( USART_Status & (1<<FE|1<<UPE) ) != 0 )
	{
		// If there was a framing error or a parity error, ignore and return (low-level idle line)
		return;
	}
	
	if( CurrentUSARTPacketInCount == 0 )
	{ 
		if( CurrentByte != 0 )
		{ // Command = 0 is illegal, not defined, hard-coded, not allowed, not ever, gives no response (idle line)
			LastCommand = CurrentByte;
			if( ( CurrentByte & CMD_MASK_CommandSize ) == CMD_MASK_SingleByte )
			{ // The new command is a single-byte one, so decode and respond through mainloop right away:
				OpFlags |= FLAG_OP_USART_HANDLE_COMMAND;
				// Do not increase the counter, since the next byte will be a command again.
			}
			else if( ( CurrentByte & CMD_MASK_CommandSize ) == CMD_MASK_TwoByteCMD )
			{ // New command is a two byte command
				CurrentUSARTPacketInCount++;
				TargetUSARTPacketInCount = 1;
			}
			else if( ( CurrentByte & CMD_MASK_CommandSize ) == CMD_MASK_MultiByte )
			{
				CurrentUSARTPacketInCount++;
				TargetUSARTPacketInCount = 2; // Set the target to two for now, we'll update it when we know more.
			}
			else
			{ // send a NACK: Command 
				CurrentUSARTPacketOutCount = 1;
				SerialOutBuffer[1] = CMD_RESPONSE_NACK; // The output buffer operates in reverse, so to send the NACK first, set it as the last
				SerialOutBuffer[0] = CurrentByte;
				LastCommand = 0; // clear command
				CurrentUSARTPacketInCount = 0; // reset counter
				StartUSARTTX();
			}
		}
		// else do nothing: Ignore zero command-bytes as high-level idle line.
	}
	else if( CurrentUSARTPacketInCount == 1 )
	{ // if we receive the second byte:
		if( TargetUSARTPacketInCount == 1 )
		{ // if we wanted only two bytes, store the byte, do nothing further and handle command:
			SerialInBuffer[0] = CurrentByte;
			OpFlags |= FLAG_OP_USART_HANDLE_COMMAND;
		}
		else
		{ // if we expect more than two bytes, the second bytes defines how many bytes we expect, excluding the command byte:
			TargetUSARTPacketInCount = CurrentByte + 1; // because we started at index 0, where the payload number is offset 1, we only need to add 1.
			CurrentUSARTPacketInCount++;
		}
	}
	else
	{
		SerialInBuffer[CurrentUSARTPacketInCount - 2] = CurrentByte;
		if( CurrentUSARTPacketInCount == TargetUSARTPacketInCount )
		{ // reached the end of the packet, handle command:
			OpFlags |= FLAG_OP_USART_HANDLE_COMMAND;
		}
		else
		{
			CurrentUSARTPacketInCount++;
		}
	}
}

/* upon data register empty: */
ISR(USART0_UDRE_vect)
{
	
	// if complete:
	StopUSARTTX();
}




/*
 WDT Interrupt; The main time-base for the patterning and random generation.
 This routine sets a flag at final time-out, after which the main routine does the bulk
 of further processing, to keep overhead low.
 */
ISR(WDT_OVERFLOW_vect)
{	
	WDTPostDecrement--;
	
	if( WDTPostDecrement == 0)
	{
		// action!:

// Only if the reset is disabled in the project's hardware can we use it as an input:
#ifdef	RESET_IS_DISABLED
		// Check the mode we are running in, to determine what action to take:
		if( (PIN_MODE & PORTIN_MODE) == PIN_MODE )
		{ //  If high, we run in random mode
			// first of all: load the tick delay:
#endif
			WDTPostDecrement = eeprom_read_byte(&EE_RandomStepDelay);
			ProcessRandomHouseToggle();
			//MainFlags |= FLAG_PROCESS_RANDOMHOUSE;

#ifdef	RESET_IS_DISABLED
		}
		else
		{ // else we run in pattern mode
			WDTPostDecrement = eeprom_read_byte(&EE_MainStepDelay);
			ProcessPatternStep();
			//MainFlags |= FLAG_PROCESS_PATTERNSTEP;
		}
#endif
		
	}
	// We can just re-enable the interrupt here, since none of the mentioned risks really apply to us:
	WDTCR |= (1<<WDIE);
}


/*
 * Interrupt of Timer0, configured to trigger at an interval by defines.
 *    at each interrupt the random bit inputs will read, after 8 reads the 
 *    generated byte will be copied into "LastRandom" byte.
 *
 * The noise was measured at about 230kHz average mean frequency,
 *   so this can run at up to 100kHz, allowing 12.5 thousand random
 *   bytes per second, but since we only need them a few times per
 *   second, we run this at about 1kHz, allowing 125bytes per second
 *   which then can be used by the WDT interrupt.
 */
ISR(TIMER0_COMPA_vect)
{
//	LastRandom++;

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
	
	if( RandomBitCount == 0x08 )
	{
		// Reached a full byte here
		LastRandom = TempRandom;
		TempRandom = 0;
		// don't reset counter, to keep track of odd/even bytes...
	}
	// (( if is 2 bytes less than else if and they do the same: ))
	else if( (RandomBitCount & 0x10) == 0x10 )
	{
		// Reached a second full byte here, reset moment for the counter:
		LastRandom = TempRandom;
		TempRandom = 0;
		RandomBitCount = 0;
	}
	//*/
}











/* ##############################################################################
 #
 #   Start of internal static functions;;
 #
 # ##############################################################################*/

inline static void StartUSARTTX()
{
	
}

inline static void StopUSARTTX()
{
	
}

inline static void HandleLastCommand()
{
	
}

/*
 * Process the next pattern step
 * TODO: Create a pattern stepping process
 */
inline static void ProcessPatternStep()
{
	asm(""); // prevent compiler optimisation
}


/*
 * Handle the setting or toggling of the houses in random mode:
 */
inline static void ProcessRandomHouseToggle()
{
	uint8_t	NumberOfHouses, temp;
	
	if( (PIN_NIGHTMODE & PORTIN_NIGHTMODE) == PIN_NIGHTMODE )
	{ // if we're in night mode:
		temp = eeprom_read_byte(&EE_TargetNumberHousesNightTime);
		if( CurrentTargetHouses > temp )
		{ // and the current number of target houses is still larger than the desired number
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
	NumberOfHouses = 0;
	
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


/*
Sets the port output pins to the values as desired by the flagbyte, can be called to inferfere with any pattern or random
but is intended to switch the houses on/off when patterning is turned on or off
*/
inline static void SetNoEffectOutputs()
{
	if( (MainFlags & FLAG_MAIN_NOEFFECT_HOUSE_STATES_ON) == FLAG_MAIN_NOEFFECT_HOUSE_STATES_ON )
	{
		PORTB |= PORTB_STARTUP_ALL_ON; // For compatibility with other pin functionality, use or-is
		PORTD |= PORTD_STARTUP_ALL_ON; // to only update the desired bits
	}
	else
	{
		PORTB &= ~PORTB_STARTUP_ALL_ON; // For compatibility with other pin functionality, use and-is
		PORTD &= ~PORTD_STARTUP_ALL_ON; // to only update the desired bits
	}
}