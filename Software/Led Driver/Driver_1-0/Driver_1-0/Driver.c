/*
 * Driver.c
 *
 * Created: 22-12-2014 20:54:38
 *  Author: Robert
 *
 * Even with externalised headers this file is starting to grow large with the whole Serial procedures, but 
 * until the full Serial system has been tested and verified, it will stay that way, as the influence of splitting
 * in different ways on the compiled file size is something that should not be researched in between code expansion
 * As the compiled size is growing quickly with the addition of the Serial protocol, file size will become a 
 * priority soon, and everything needs to be fully functional before we start ripping the rudimentary guts out
 * of our code.
 * 
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
// U2X = 1
// UBRR = 7

// TODO: Extend ifdefs around GPIOR use blocks to include "If defined GPIOR" for compatibility.

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
static void StartUSARTTX();
inline static void HandleLastCommand();
static void BuildStandardAckAndTX();
static uint8_t PopRandomNumber();
static void PushRandomNumber(uint8_t input);
static void BuildFourByteExackAndTX(uint8_t payload);
inline static uint8_t __eeprom_read_byte(uint8_t * EEPromAddress);
inline static void __eeprom_update_byte(uint8_t * EEPromAddress, uint8_t ByteSource);
inline static void EepromStartIWrite();
inline static void BuildExNackAndTX(uint8_t reason);
inline static void PostTXActions();
inline static void BuildNackAndTX();

uint8_t EEMEM	EE_TargetNumberHousesDayTime = DEF_DAY_TIME_TARGET_NUMBER_HOUSES; // 1
uint8_t EEMEM	EE_MinimumNumberHouses = DEF_MINIMUM_NUMBER_OF_HOUSES; // 2
uint8_t EEMEM	EE_TargetNumberHousesNightTime = DEF_NIGHT_TIME_TARGET_NUMBER_HOUSES; // 3
uint8_t EEMEM	EE_PatternStepCount = DEF_PATTERN_STEP_COUNT; // 4 
uint8_t	EEMEM	EE_MainStepDelay = DEF_MAIN_STEPDELAY; // 5
uint8_t EEMEM	EE_RandomStepDelay = DEF_RANDOM_STEP_DELAY; // 6
uint8_t	EEMEM	EE_PostDelayTicksNightTime = DEF_NIGHTTIME_TICKS; // 7
uint8_t EEMEM	EE_StartupDelay = DEF_STARTUP_DELAY; // 8
uint8_t EEMEM	EE_MainFlags = DEF_MAINFLAGS; // 9
uint8_t EEMEM	EE_UBRR_Startup = DEF_UBRR_VALUE; // 10

uint8_t EEMEM	EE_PatternData[MAXIMUM_PATTERN_SIZE] = DEF_PATTERN;


uint8_t	WDTPostDecrement; // Decrementer for the WDT interrupt
uint8_t	TempRandom; // Temporary memory for creating the random number
uint8_t	RandomBitCount; // Counter for the random generator
uint8_t	CurrentTargetHouses; // Number of houses we want turned on at present
uint8_t	NightTimeDecreaseDecounter; // Decrementer for the slow increase or decrease of target house numbers
uint8_t AvailableRandomNumbers;
uint8_t	RandomStack[RANDOM_STACK_SIZE];
uint8_t	EEPROMBlockWriteBuffer[EEPROM_BLOCK_WRITE_BUFFER];
uint8_t UBRRBuffer; // buffer memory to store the UBRR byte to write after tx of an ACK.
uint8_t	CurrentPatternIndex; // pointer byte to the index of the pattern data

#ifdef	STORE_FLAGBYTES_IN_GPIOR
#define		MainFlags		GPIOR0 // Byte of main functionality flags
#define		OpFlags			GPIOR1 // Operational flags, only read-able externally
#else
uint8_t	volatile	MainFlags; // Byte of main functionality flags
uint8_t	volatile	OpFlags; // Operational flags, only read-able externally
#endif

#ifdef STORE_PACKETOUTCOUNT_INGPIOR2
#define		CurrentUSARTPacketOutCount		GPIOR2
#else
uint8_t CurrentUSARTPacketOutCount; // If put in GPIOR2, compile size goes from 2774 to 2512
#endif

uint8_t LastRandom; // Random number, updated at a constant interval (standard config about 125 unique random bytes per second)
uint8_t LastCommand;
uint8_t CurrentUSARTPacketInCount;
uint8_t	TargetUSARTPacketInCount;
uint8_t SerialInBuffer[SERIAL_BUFFER_SIZE]; // Serial port read (RX) buffer
uint8_t SerialOutBuffer[SERIAL_BUFFER_SIZE]; // Serial port send (TX) buffer



int main(void)
{	
	uint8_t temp;
	// Pattern Data will be handled directly from EE from now on:
	// Read the configuration bytes:
	//RAM_PatternStepCount = eeprom_read_byte(&EE_PatternStepCount);
	//
	// Read the pattern data, up to the number of steps in the pattern:
	//eeprom_read_block((void *) RAM_PatternData, (const void*) EE_PatternData, MAXIMUM_PATTERN_SIZE);
	
	CurrentTargetHouses = __eeprom_read_byte(&EE_TargetNumberHousesDayTime);
	
	// set startup delay
	WDTPostDecrement = __eeprom_read_byte(&EE_StartupDelay);

	NightTimeDecreaseDecounter = __eeprom_read_byte(&EE_PostDelayTicksNightTime);
	
	MainFlags = __eeprom_read_byte(&EE_MainFlags);
	
	temp = __eeprom_read_byte(&EE_UBRR_Startup);
	
	UBRRHighRegister = 0x00;
	UBRRLowRegister = temp;
	
	DDRA = DDRA_STARTUP;
	DDRB = DDRB_STARTUP;
	DDRD = DDRD_STARTUP;
	
	// To make sure the TX line is in mark (on) state when the transmitter is turned off, set the pin as output and high:
	TXPIN_PORT |= TXPIN_PIN;
	TXPIN_DDR |= TXPIN_PIN;
	
	USART_CSRA = UCSRA_STARTUP;
	USART_CSRB = UCSRB_STARTUP; 
	USART_CSRC = UCSRC_STARTUP;
	
	SetNoEffectOutputs();
	PORTA = PORTA_STARTUP;
	
	OpFlags = 0x00;
	CurrentUSARTPacketOutCount = 0x00;
	CurrentUSARTPacketInCount = 0x00;
	LastRandom = 0x00;
	TempRandom = 0x00;
	RandomBitCount = 0x00;
	LastCommand = 0x00;
	AvailableRandomNumbers = 0x00;
	
	WDTCR = WDTCSR_STARTUP;
	
	OCR_RANDOM_A = OCR_RANDOM_A_STARTUP;
	TCCR_RANDOM_A = TCCR_RANDOM_A_STARTUP;
	TCCR_RANDOM_B = TCCR_RANDOM_B_STARTUP;
	
	TIMSK_REGISTER = TIMSK_STARTUP;
	
	sei();
	
	while(1)
    {
		// With the move to volatile and/or GPIOR flagging this should work again, but for now we're 
		// focusing on completing the USART system, so we'll try that later:
		/*
		if( (MainFlags & FLAG_PROCESS_PATTERNSTEP) == FLAG_PROCESS_PATTERNSTEP )
		{	// If the flag to process a patternstep is set:
			ProcessPatternStep();// process a patternstep
			MainFlags &= ~FLAG_PROCESS_PATTERNSTEP; // unset the flag
		}
		if( (MainFlags & FLAG_PROCESS_RANDOMHOUSE) == FLAG_PROCESS_RANDOMHOUSE )
		{
			ProcessRandomHouseToggle();
			MainFlags &= ~FLAG_PROCESS_RANDOMHOUSE;
		}
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
	uint8_t CurrentByte = UDR; // always read, to clear interrupt and free buffer
	
	if( ( USART_STATUS_FLAG_REGISTER & USART_ERROR_FLAGS ) != 0 )
	{
		// If there was a framing error or a parity error, ignore and return (low-level idle line)
		return;
	}
	
	if( CurrentUSARTPacketInCount == 0 )
	{ 
		if( CurrentByte != 0 )
		{ // Command = 0 is illegal, not defined, hard-coded, not allowed, not ever, gives no response (high-level idle line)
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
				BuildNackAndTX();
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
	// The following is basically a carry test that works on GPIOR and uint8_t when decrementing.
	if( ( CurrentUSARTPacketOutCount & 0x80 ) == 0x80 )
	{ // If we have an overflow (underflow) stop transmission and handle post-TX checks:
		UCSRB |= (1<<TXCIE); // enable TX complete interrupt (we do this here, to avoid prematurely triggering when loading new data takes too long due to interrupt queueing).
		StopUSARTTX(); // pre-emtively stop the transmitter, so it doesn't ask for another byte at some point.
	}
	else
	{	
		UDR = SerialOutBuffer[CurrentUSARTPacketOutCount]; // get current byte into the serial output register
		CurrentUSARTPacketOutCount--;
	}
}


// When transmission is done: (this may become nescesary after a small test)
ISR(USART0_TX_vect)
{
	// This interrupt only happens when the last byte had already been loaded and afterwards the TX system completed transferring all data:
	PostTXActions();
	// turn off the interrupt to make sure no inappropriate triggers happen at a next transfer:
	UCSRB &= ~(1<<TXCIE);
	
	// as we are doing half-duplex type communication, enable the receiver once the last bit is sent:
	USART_CSRB |= (1<<RXEN);
}


/*
 WDT Interrupt; The main time-base for the patterning and random generation.
 This routine sets a flag at final time-out, after which the main routine does the bulk
 of further processing, to keep overhead low.
 */
ISR(WDT_OVERFLOW_vect)
{	
	// if the enable effects flag is not set, do nothing:
	if( ( MainFlags & FLAG_MAIN_ENABLE_EFFECTS ) != FLAG_MAIN_ENABLE_EFFECTS )
		return;
	
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
			WDTPostDecrement = __eeprom_read_byte(&EE_RandomStepDelay);
			ProcessRandomHouseToggle();
			//MainFlags |= FLAG_PROCESS_RANDOMHOUSE;

#ifdef	RESET_IS_DISABLED
		}
		else
		{ // else we run in pattern mode
			WDTPostDecrement = __eeprom_read_byte(&EE_MainStepDelay);
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
		PushRandomNumber(TempRandom);
		TempRandom = 0;
		// don't reset counter, to keep track of odd/even bytes...
	}
	// (( if is 2 bytes less than else if and they do the same: ))
	else if( (RandomBitCount & 0x10) == 0x10 )
	{
		// Reached a second full byte here, reset moment for the counter:
		LastRandom = TempRandom;
		PushRandomNumber(TempRandom);
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

inline static void BuildNackAndTX()
{
	CurrentUSARTPacketOutCount = 1;
	SerialOutBuffer[1] = CMD_RESPONSE_NACK; // The output buffer operates in reverse, so to send the NACK first, set it as the last
	SerialOutBuffer[0] = LastCommand;
	LastCommand = 0; // clear command
	CurrentUSARTPacketInCount = 0; // reset counter
	StartUSARTTX();
}


static void BuildFourByteExackAndTX(uint8_t payload)
{
	CurrentUSARTPacketOutCount = 3;
	SerialOutBuffer[3] = CMD_RESPONSE_EXACK;
	SerialOutBuffer[2] = LastCommand;
	SerialOutBuffer[1] = 1; // followed by one byte
	SerialOutBuffer[0] = payload;
	LastCommand = 0; // clear command
	CurrentUSARTPacketInCount = 0; // reset counter
	StartUSARTTX();
}

inline static void BuildExNackAndTX(uint8_t reason)
{
	CurrentUSARTPacketOutCount = 2;
	SerialOutBuffer[2] = CMD_RESPONSE_EXNACK;
	SerialOutBuffer[1] = LastCommand;
	SerialOutBuffer[0] = reason;
	LastCommand = 0; // clear command
	CurrentUSARTPacketInCount = 0; // reset counter
	StartUSARTTX();
}

/*
 The random Stack is only used for the getRandomNumber serial function, so it is allowed to
 run empty, when it is, it will become blocking, untill there is a new random number.
 The internal WDT interrupt will use the "LastRandom" value, independently, so this will never cause
 a race condition, so long PopRandomNumber is only used from non-interrupt places (void main(void))
*/
static uint8_t PopRandomNumber()
{
	while( AvailableRandomNumbers == 0 ) { }; // block while there are no random numbers
	
	return RandomStack[AvailableRandomNumbers--];
}

/*
 Add a new random number to the stack, once that is done, increase the counter (to make sure when Pop is blocking
 that it will get an actual random number, not an old repeat.
*/
static void PushRandomNumber(uint8_t input)
{
	uint8_t temp = (RANDOM_STACK_SIZE - 1) - AvailableRandomNumbers;
	if( temp == 0 ) // little trick to allow the system to optimize more easily between here and the if at the end
	{ // if the stack was full, shift it all down, to "keep it fresh (may be removed later, because true random doesn't really need fresh
		for(int8_t itt = (RANDOM_STACK_SIZE - 1); itt > 0; itt--)
		{
			RandomStack[itt - 1] = RandomStack[itt];
		}
	}
	
	RandomStack[AvailableRandomNumbers] = input;
	
	if( temp != 0 )
	{
		AvailableRandomNumbers++;
	}
}

static void StartUSARTTX()
{	
	// as we are doing half-duplex type communication, disable the receiver once we start transmitting (startTX is only called once a command has been fully parsed, so this is safe).
	USART_CSRB &= ~(1<<RXEN);
	
	UDR = SerialOutBuffer[CurrentUSARTPacketOutCount]; // get current byte into the serial output register and decrement counter
	CurrentUSARTPacketOutCount--;
	
	USART_CSRB |= ( (1 << TXEN)|(1 << UDRIE) ); // Enable the transmitter and UDRE interrupt
	// Note: Enabling the UDRIE and TXEN at the same time should always be done after loading the UDR with the first byte,
	//    because, of course, else the interrupt will trigger immediately.
	//    During refactoring this occurred, as a lot of statements were juggled around and compacted, and due to the high speed of
	//    development this was overlooked. Luckily 30 minutes of creative debugging solved the problems.
}

inline static void StopUSARTTX()
{
	USART_CSRB &= ~( (1<<TXEN)|(1<<UDRIE) ); // turn off udre interrupt and the transmitter
}


static void BuildStandardAckAndTX()
{
	CurrentUSARTPacketOutCount = 1;
	SerialOutBuffer[1] = CMD_RESPONSE_ACK; // The output buffer operates in reverse, so to send the ACK first, set it as the last
	SerialOutBuffer[0] = LastCommand;
	LastCommand = 0; // clear command
	CurrentUSARTPacketInCount = 0; // reset USART counter
	StartUSARTTX();
}


/*
Invokes PopRandomNumber: Should not be called from an interrupt while PopRandomNumber is a blocking function.
*/
inline static void HandleLastCommand()
{
	uint8_t Command = LastCommand;
	
	switch (Command)
	{
		case CMD_EnableEffects:
			MainFlags |= FLAG_MAIN_ENABLE_EFFECTS;
			BuildStandardAckAndTX();
			break;
		case CMD_DisableEffects:
			MainFlags &= ~FLAG_MAIN_ENABLE_EFFECTS;
			BuildStandardAckAndTX();
			break;
		case CMD_ReadRandomDelay:
			BuildFourByteExackAndTX( __eeprom_read_byte(&EE_RandomStepDelay) );
			break;
		case CMD_ReadPatternDelay:
			BuildFourByteExackAndTX( __eeprom_read_byte(&EE_MainStepDelay) );
			break;
		case CMD_ReadPatternLength:
			BuildFourByteExackAndTX( __eeprom_read_byte(&EE_PatternStepCount) );
			break;
		case CMD_ReadStartupDelay:
			BuildFourByteExackAndTX( __eeprom_read_byte(&EE_StartupDelay) );
			break;
		case CMD_ReadPostDelayTicksNight:
			BuildFourByteExackAndTX( __eeprom_read_byte(&EE_PostDelayTicksNightTime) );
			break;
		case CMD_ReadMinimumHouses:
			BuildFourByteExackAndTX( __eeprom_read_byte(&EE_MinimumNumberHouses) );
			break;
		case CMD_ReadNightTimeHouses:
			BuildFourByteExackAndTX( __eeprom_read_byte(&EE_TargetNumberHousesNightTime) );
			break;
		case CMD_ReadDayTimeHouses:
			BuildFourByteExackAndTX( __eeprom_read_byte(&EE_TargetNumberHousesDayTime) );
			break;
		case CMD_ReadMainFlags:
			BuildFourByteExackAndTX( MainFlags );
			break;
		case CMD_GetOperationalFlags:
			BuildFourByteExackAndTX( OpFlags );
			break;
		case CMD_StoreCurrentUBRR:
			__eeprom_update_byte(&EE_UBRR_Startup, UBRRL); // Compiler should handle the SFRIO -> REG -> EEPROM, even when we replace with our own function later (interrupt driven)
			BuildStandardAckAndTX();
			break;
		case CMD_Ping:
			CurrentUSARTPacketOutCount = 0;
			SerialOutBuffer[0] = CMD_RESPONSE_PONG;
			LastCommand = 0; // clear command
			CurrentUSARTPacketInCount = 0; // reset USART counter
			StartUSARTTX();
			break;
		case CMD_GetVersion:
			CurrentUSARTPacketOutCount = 4;
			SerialOutBuffer[4] = CMD_RESPONSE_EXACK;
			SerialOutBuffer[3] = 3; // 3 bytes follow
			SerialOutBuffer[2] = VERSION_MAJOR;
			SerialOutBuffer[1] = VERSION_MINOR;
			SerialOutBuffer[0] = VERSION_RELEASE;
			LastCommand = 0; // clear command
			CurrentUSARTPacketInCount = 0; // reset USART counter
			StartUSARTTX();
			break;
		case CMD_SetRandomDelay:
			__eeprom_update_byte(&EE_RandomStepDelay, SerialInBuffer[0]);
			BuildStandardAckAndTX();
			break;
		case CMD_SetPatternDelay:
			__eeprom_update_byte(&EE_MainStepDelay, SerialInBuffer[0]);
			BuildStandardAckAndTX();
			break;
		case CMD_SetMinimumHouses:
			if( ( SerialInBuffer[0] > __eeprom_read_byte(&EE_TargetNumberHousesNightTime) ) ||
				( SerialInBuffer[0] > __eeprom_read_byte(&EE_TargetNumberHousesDayTime) ) )
			{ // if the minimum is higher than an existing target, respond with error and don't store
				BuildExNackAndTX(CMD_EXNACK_MinimumTooHigh);
			}
			else
			{
				__eeprom_update_byte(&EE_MinimumNumberHouses, SerialInBuffer[0]);
				BuildStandardAckAndTX();
			}
			break;
		case CMD_SetNightTimeHouses:
			if( SerialInBuffer[0] < __eeprom_read_byte(&EE_MinimumNumberHouses) )
			{ // target is lower than minimum, respon with error and don't store
				BuildExNackAndTX(CMD_EXNACK_SetHousesLowerThanMinimum);
			}
			else
			{
				__eeprom_update_byte(&EE_TargetNumberHousesNightTime, SerialInBuffer[0]);
				BuildStandardAckAndTX();
			}
			break;
		case CMD_SetDayTimeHouses:
			if( SerialInBuffer[0] < __eeprom_read_byte(&EE_MinimumNumberHouses) )
			{ // target is lower than minimum, respon with error and don't store
				BuildExNackAndTX(CMD_EXNACK_SetHousesLowerThanMinimum);
			}
			else
			{
				__eeprom_update_byte(&EE_TargetNumberHousesDayTime, SerialInBuffer[0]);
				BuildStandardAckAndTX();
			}
			break;
		case CMD_SetPatternLength:
			if( SerialInBuffer[0] >= MAXIMUM_PATTERN_SIZE )
			{ // if the pattern length is too high for the available memory:
				BuildExNackAndTX(CMD_EXNACK_PatternLengthTooHigh);
			}
			else
			{
				__eeprom_update_byte(&EE_PatternStepCount, SerialInBuffer[0]);
				BuildStandardAckAndTX();
			}
			break;
		case CMD_SetStartupDelay:
			__eeprom_update_byte(&EE_StartupDelay, SerialInBuffer[0]);
			BuildStandardAckAndTX();
			break;
		case CMD_SetPostDelayTicksNight:
			__eeprom_update_byte(&EE_PostDelayTicksNightTime, SerialInBuffer[0]);
			BuildStandardAckAndTX();
			break;
		case CMD_SetHouseOn:
			if( SerialInBuffer[0] <= 9)
			{
				SetHouseOn( SerialInBuffer[0] );
				BuildStandardAckAndTX();
			}
			else
			{
				BuildExNackAndTX(CMD_EXNACK_NumberOutOfBounds);
			}
			break;
		case CMD_SetHouseOff:
			if( SerialInBuffer[0] <= 9)
			{
				SetHouseOff( SerialInBuffer[0] );
				BuildStandardAckAndTX();
			}
			else
			{
				BuildExNackAndTX(CMD_EXNACK_NumberOutOfBounds);
			}
			break;
		case CMD_SetUBRR:
			UBRRBuffer = SerialInBuffer[0];
			BuildStandardAckAndTX();
			break;
		case CMD_SetAndStoreMainFlags:
			MainFlags = SerialInBuffer[0];
			__eeprom_update_byte(&EE_MainFlags, MainFlags);
			BuildStandardAckAndTX();
			break;
		case CMD_GetRandomNumber:
			BuildFourByteExackAndTX( PopRandomNumber() );
			break;
		case CMD_GetDeviceSignature:
			// TODO: Read each signature byte;
			CurrentUSARTPacketOutCount = 7;
			SerialOutBuffer[7] = CMD_RESPONSE_EXACK;
			SerialOutBuffer[6] = 6; // 6 bytes to follow
			SerialOutBuffer[5] = 0; // reading of device signature not yet implemented
			SerialOutBuffer[4] = 0; // TODO: implement reading of device signature.
			SerialOutBuffer[3] = 0;
			SerialOutBuffer[2] = PROJECT_SIGNATURE_HIGH;
			SerialOutBuffer[1] = PROJECT_SIGNATURE_MID;
			SerialOutBuffer[0] = PROJECT_SIGNATURE_LOW;
			LastCommand = 0; // clear command
			CurrentUSARTPacketInCount = 0; // reset USART counter
			StartUSARTTX();
			break;
		case CMD_ReadPatternData:
			
			break;
		case CMD_SetPatternData:
			
			break;
		case CMD_SetPatternDataContinued:
			
			break;
		case CMD_GoToBootloader:
			if( ( CurrentUSARTPacketInCount == (CMD_GoToBootloaderPackLength + 1) ) && /* Because of the way of counting incoming bytes, we need to compare to packet length + 1 */\
				( SerialInBuffer[0] == CMD_GoToBootloaderSig1 ) && \
				( SerialInBuffer[1] == CMD_GoToBootloaderSig2 ) && \
				( SerialInBuffer[2] == CMD_GoToBootloaderSig3 ) )
			{ // IFF entire bootloader packet correct, create ack and enable reboot to bootloader when done:
				OpFlags |= FLAG_OP_GOTO_BOOTLOADER_AFTERTX;
				BuildStandardAckAndTX();
				break;
			}
			// if the packet was not correct, fall through into the default NACK response (NO break!):
		default:
			// Not in this list = Not recognised:
			BuildNackAndTX();
			break;
	}
	OpFlags &= ~FLAG_OP_USART_HANDLE_COMMAND; // handling of command completed, release flag before we finish.
}

inline static void PostTXActions()
{
	if( ( OpFlags & FLAG_OP_SET_UBRR_AFTERTX ) == FLAG_OP_SET_UBRR_AFTERTX )
	{
		UBRRHighRegister = 0x00; // make sure the 16bit high byte is zero
		UBRRLowRegister = UBRRBuffer;
	}
	else if( ( OpFlags & FLAG_OP_GOTO_BOOTLOADER_AFTERTX ) == FLAG_OP_GOTO_BOOTLOADER_AFTERTX )
	{
		// TODO: Create bootloader system
	}/*
	else if( ( OpFlags & FLAG_OP_SET_UBRR_AFTERTX ) == FLAG_OP_SET_UBRR_AFTERTX )
	{
		
	}*/
}

/*
 * Process the next pattern step
 * TODO: Create a pattern stepping process
 */
inline static void ProcessPatternStep()
{
	
}


/*
 * Handle the setting or toggling of the houses in random mode:
 */
inline static void ProcessRandomHouseToggle()
{
	uint8_t	NumberOfHouses, temp;
	
	if( (PIN_NIGHTMODE & PORTIN_NIGHTMODE) == PIN_NIGHTMODE )
	{ // if we're in night mode:
		temp = __eeprom_read_byte(&EE_TargetNumberHousesNightTime);
		if( CurrentTargetHouses > temp )
		{ // and the current number of target houses is still larger than the desired number
			NightTimeDecreaseDecounter--; // decrease the ticker
			if( NightTimeDecreaseDecounter == 0 )
			{ // if the ticker reached zero:
				CurrentTargetHouses--; // decrease the target house number by one and reset the ticker:
				NightTimeDecreaseDecounter = __eeprom_read_byte(&EE_PostDelayTicksNightTime);
			}
		}
	}
	else
	{
		temp = __eeprom_read_byte(&EE_TargetNumberHousesDayTime);
		if( CurrentTargetHouses < temp )
		{
			NightTimeDecreaseDecounter--;
			if( NightTimeDecreaseDecounter == 0 )
			{
				CurrentTargetHouses++;
				NightTimeDecreaseDecounter = __eeprom_read_byte(&EE_PostDelayTicksNightTime);
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


/*
 Wrapper for blocking eeprom call, including a check on internal flags.
 TODO: consider consolidating this transfer in the eeprom R/W interrupted process, possibly saving program space on total
*/
inline static uint8_t __eeprom_read_byte(uint8_t * EEPromAddress)
{
	uint8_t temp;
	while( (OpFlags & FLAG_OP_EEPROM_INPROGRESS) ) {}; // block while we're in an interrupt driven access
	OpFlags |= FLAG_OP_EEPROM_INPROGRESS;
	temp = eeprom_read_byte(EEPromAddress);
	OpFlags &= ~FLAG_OP_EEPROM_INPROGRESS;
	return temp;
}

/*
 Wrapper for blocking eeprom call, including a check on internal flags.
 TODO: consider consolidating this transfer in the eeprom R/W interrupted process, possibly saving program space on total
*/
inline static void __eeprom_update_byte(uint8_t * EEPromAddress, uint8_t ByteSource)
{
	while( (OpFlags & FLAG_OP_EEPROM_INPROGRESS) ) {}; // block while we're in an interrupt driven access
	
	OpFlags |= FLAG_OP_EEPROM_INPROGRESS;
	eeprom_update_byte(EEPromAddress, ByteSource);
	OpFlags &= ~FLAG_OP_EEPROM_INPROGRESS;
}


inline static void EepromStartIWrite()
{
	//OpFlags |= FLAG_OP_EEPROM_INPROGRESS;
	// enable interrupt
	// load first address
	// load/write first byte
}

/*
ISR(EEPROM_Ready_vect)
{
	OpFlags &= ~FLAG_OP_EEPROM_INPROGRESS;
}
*/