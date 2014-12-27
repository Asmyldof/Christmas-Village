/*

*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Bit Masks for quick comparisons
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define		CMD_MASK_CommandSize		0xC0 // upper two bits reflect command length
#define		CMD_MASK_TwoByteCMD			0xC0 // upper two bits one is a two byte
#define		CMD_MASK_MultiByte			0x80 // upper two bits 10 is a multibyte with payload
#define		CMD_MASK_SingleByte			0x00 // upper two bits zero is a single byte (direct execution) command
// Any commands sent not fitting this mask will be responded to with NACK

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Single Byte Commands (0x10 through 0x50)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_EnableEffects			0x10 // standard ACK/NACK response
#define		CMD_DisableEffects			0x11 // standard ACK/NACK repsonse

#define		CMD_ReadPatternData			0x20 // Response is EXACK with patterndata
#define		CMD_ReadRandomDelay			0x21 // Response is EXACK with delay byte
#define		CMD_ReadPatternDelay		0x22 // Response is EXACK with delay byte

#define		CMD_StoreCurrentUBRR		0x30 // Store the current UBRR into EEPROM
// First set the UBRR with the setUBRR command, then send the store command at the new speed
// this makes sure that the new speed can be comunicated at before it is set as default start-up
// Best practise would be to add a PING/PONG between the two commands to properly verify

#define		CMD_Ping					0x40 // should get a PONG back
#define		CMD_GetVersion				0x41 // gets EXACK, followed by Major, Minor and Release version of the device

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Double Byte Commands (0xC0 through 0xFF)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_SetRandomDelay			0xC0 // standard ACK/NACK response
#define		CMD_SetPatternDelay			0xC1 // standard ACK/NACK response
#define		CMD_SetMinimumHouses		0xC2 // standard ACK/NACK response
#define		CMD_SetNightTimeHouses		0xC3 // ACK or EXNACK
#define		CMD_SetDayTimeHouses		0xC4 // ACK or EXNACK
#define		CMD_SetPatternLength		0xC5 // ACK or EXNACK
#define		CMD_SetUBRR					0xC6 // standard ACK/NACK response (Allows change of USART speed)


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Multibyte Commands: (0x80 through 0xBF)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_SetPatternData			0x80 // ACK or EXNACK


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Responses:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_RESPONSE_ACK			0x00	// followed by the command sent (command received, understood and decoded)
#define		CMD_RESPONSE_EXACK			0x01	// followed by the command sent, a payload size and the requested payload
#define		CMD_RESPONSE_NACK			0x02	// followed by the command sent (command unknown)
#define		CMD_RESPONSE_EXNACK			0x03	// followed by the command sent and a third byte explaining why it was denied


#define		CMD_RESPONSE_PONG			0x0B	// Special response to the Ping command

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * EXNACK Reasons:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_EXNACK_PayloadInterrupted			0x0A // No data received before the line was reset.
#define		CMD_EXNACK_CommandNotFound				0x05 // Comparable to normal NACK: Command failed without reason
#define		CMD_EXNACK_PatternLengthTooHigh			0xA0 // Given pattern-payload length exceeds the amount of data allowed in this hardware
#define		CMD_EXNACK_SetHousesLowerThanMinimum	0x50 // Tried to set a target number lower than the set minimum
#define		CMD_EXNACK_MinimumTooHigh				0xAA // Tried to set a minimum higher than an existing target number of houses
#define		CMD_EXNACK_CommandUnsupported			0x55 // Should always be implemented PC side, in case command sets change HW side






