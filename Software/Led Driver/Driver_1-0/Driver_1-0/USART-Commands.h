/*
 This header file defines the command set for the Atmel Tiny Pattern Chips.
 Created: 27 december 2014
			For: Christmas Village Controller Project
 Intended to be portable to later projects, possibly with some extensions, allowing extensions.
 
 Copyright (c) 2014: Asmyldof
            http://www.asmyldof.com
			
 This command set is split in:
  - One byte commands: Simple actions that can be executed right away
  - Two byte commands: Simple set-value/get-register commands, quick to execute, but allow a single-byte payload
  - Three and more byte commands: A command, followed by a payload length specifier, followed by a payload of 1 or more bytes
  
  Not all commands are simply lumped into the three-byte command set, to make the system more efficient with the data-line and
     interrupt handling time (the lower the average number of byte to be sent per command, the less time is spent in USART interrupt).
*/

#ifndef __LED_DRIVER_USART_CMDS_H__
#define __LED_DRIVER_USART_CMDS_H__

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
#define		CMD_EnableEffects				0x10 // standard ACK/NACK response
#define		CMD_DisableEffects				0x11 // standard ACK/NACK repsonse

#define		CMD_ReadPatternData				0x20 // Response is EXACK with patterndata
#define		CMD_ReadRandomDelay				0x21 // Response is EXACK with delay byte
#define		CMD_ReadPatternDelay			0x22 // Response is EXACK with delay byte
#define		CMD_ReadPatternLength			0x23
#define		CMD_ReadStartupDelay			0x24
#define		CMD_ReadPostDelayTicksNight		0x25
#define		CMD_ReadMinimumHouses			0x26
#define		CMD_ReadNightTimeHouses			0x27
#define		CMD_ReadDayTimeHouses			0x28
	

#define		CMD_GetRandomNumber				0x2C // Response is EXACK with a random number byte
#define		CMD_ReadMainFlags				0x2D // Response is EXACK with MainFlag byte
#define		CMD_GetOperationalFlags			0x2E // Response is EXACK with OpFlag byte

#define		CMD_StoreCurrentUBRR			0x38 // Store the current UBRR into EEPROM
// First set the UBRR with the setUBRR command, then send the store command at the new speed
// this makes sure that the new speed can be comunicated at before it is set as default start-up
// Best practice would be to add a PING/PONG between the two commands to properly verify

#define		CMD_Ping						0x40 // should get a PONG back
#define		CMD_GetVersion					0x41 // gets EXACK, followed by Major, Minor and Release version of the device (after a byte count of 3 of course)
#define		CMD_GetDeviceSignature			0x42 // gets EXACK, followed by the three signature bytes of the device (after a byte count of 6 of course), followed by 3 bytes Asmyldof Project Signature (in case Asmyldof later creates a complete support suite)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Double Byte Commands (0xC0 through 0xFF)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_SetRandomDelay				0xC0 // standard ACK/NACK response
#define		CMD_SetPatternDelay				0xC1 // standard ACK/NACK response
#define		CMD_SetMinimumHouses			0xC2 // standard ACK/NACK response
#define		CMD_SetNightTimeHouses			0xC3 // ACK or EXNACK
#define		CMD_SetDayTimeHouses			0xC4 // ACK or EXNACK
#define		CMD_SetPatternLength			0xC5 // ACK or EXNACK
#define		CMD_SetStartupDelay				0xC6
#define		CMD_SetPostDelayTicksNight		0xC7

#define		CMD_SetHouseOn					0xD0 // followed by house number ; ACK or EXNACK: NumberOutOfBounds
#define		CMD_SetHouseOff					0xD1 // followed by house number ; ACK or EXNACK

#define		CMD_SetUBRR						0xE0 // standard ACK/NACK response (Allows change of USART speed)
// Make sure the resulting speed is no less than 14.4k for triggering the bootloader without time-out interrupts (these can be very
//    difficult to debug)
// Will send an ACK out _before_ the new UBRR value is set (so that the PC can await an ACK/NACK before changing its own endpoint


#define		CMD_SetAndStoreMainFlags		0xE1 // standard ACK/NACK response


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Multibyte Commands: (0x80 through 0xBF)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_SetPatternData				0x80 // EXACK or EXNACK
/*
Followed by:
[byte] size of this packet's payload: Advised 1 to 16 bytes, as smallest block size for pattern devices will be 16bytes burst
[byte] size of total multi-packet payload. if size of total == size of this, then exack follows right after eeprom ram buffer is
		filled and the serial interface will be free to be used in other ways.
		When the total payload exceeds the payload of this packet, more datablocks should be sent using the DataContinued
		command, in this case the EXACK will come only after the total EEPROM buffer has been written, alerting the PC that
		the next packet can be sent. As the PC waits for an ACK/NACK response (or should) before doing more, this blocks up
		the line until the devices recognizes that the last packet was received, which again will be loaded into the RAM buffer
		and an ACK will be sent.
[block] data, minimum 1 byte, maximum is implementation dependent, advised to keep at 16bytes or less.

EXACK response: [ EXACK | CMD_SetPatternData | PayloadTotal | PayloadReceived | PayloadRemaining ]
So in short, when "PayloadRemaining" is zero, the device is ready for new work. Alternatively the PC can decide to abandon
without any consequences other than not completing the pattern write, once an ACK/EXACK is received the PC is free to
send any new command. And, as long as power is not cycled, the transfer can be picked up at any time, at the point
where it was left off. Only the CMD_SetPatternData command will reset the procedure.

Remaining datablocks can be sent with CMD_SetPatternDataContinued
*/
#define		CMD_SetPatternDataContinued		0x81
/*
EXACK response: [ EXACK | CMD_SetPatternDataContinued | PayloadTotal | PayloadReceived | PayloadRemaining ]
So in short, when "PayloadRemaining" is zero, the device is ready for new work. Alternatively the PC can decide to abandon
without any consequences other than not completing the pattern write, once an ACK/EXACK is received the PC is free to
send any new command. And, as long as power is not cycled, the transfer can be picked up at any time, at the point
where it was left off. Only the CMD_SetPatternData command will reset the procedure.
*/

#define		CMD_GoToBootloader				0xAF	// Not yet implemented
#define		CMD_GoToBootloaderPackLength	0x03
#define		CMD_GoToBootloaderSig1			0xFF
#define		CMD_GoToBootloaderSig2			0x00
#define		CMD_GoToBootloaderSig3			0xAA
/*
Should be sent as follows:
 [ CMD_GoToBootloader | CMD_GoToBootloaderPackLength | CMD_GoToBootloaderSig1 | CMD_GoToBootloaderSig2 | CMD_GoToBootloaderSig3 ]
 After the last byte is received and verified the ACK is sent, once the transmission is complete the device will be reset to bootloader
 If any of the bytes miss, or are not sent within the preset follow-up time-out, or are wrong NACK will be returned and nothing will happen.
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Responses 0x01 through 0x0F (0x00 is invalid in both directions for a first byte):
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_RESPONSE_ACK				0x01	// followed by the command sent (command received, understood and decoded)
#define		CMD_RESPONSE_EXACK				0x05	// followed by the command sent, a payload size and the requested payload
#define		CMD_RESPONSE_NACK				0x02	// followed by the command sent (command unknown)
#define		CMD_RESPONSE_EXNACK				0x06	// followed by the command sent and a third byte explaining why it was denied
/*
ACK = 0x01, NACK = 0x02, Extended = 0x04
*/

#define		CMD_RESPONSE_PONG				0x0B	// Special response to the Ping command

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * EXNACK Reasons:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define		CMD_EXNACK_PayloadInterrupted			0x0A // No data received before the line was reset.
#define		CMD_EXNACK_CommandNotFound				0x05 // Comparable to normal NACK: Command failed without reason
#define		CMD_EXNACK_PatternLengthTooHigh			0xA0 // Given pattern-payload length exceeds the amount of data allowed in this hardware
#define		CMD_EXNACK_SetHousesLowerThanMinimum	0x50 // Tried to set a target number lower than the set minimum
#define		CMD_EXNACK_MinimumTooHigh				0xAA // Tried to set a minimum higher than an existing target number of houses
#define		CMD_EXNACK_CommandUnsupported			0x55 // Should always be implemented PC side, in case command sets change HW side
#define		CMD_EXNACK_NumberOutOfBounds			0xC3 // A requested house number is too large.


#endif // __LED_DRIVER_USART_CMDS_H__


