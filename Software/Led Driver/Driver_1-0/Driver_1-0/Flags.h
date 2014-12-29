/*

*/

#ifndef __LED_DRIVER_FLAGS_H__
#define __LED_DRIVER_FLAGS_H__


#define		FLAG_MAIN_ENABLE_EFFECTS					0x10
#define		FLAG_MAIN_NOEFFECT_HOUSE_STATES_ON			0x01

// Deprecated, because the USART supports all blocking/unblocking features needed:
//#define		FLAG_OP_USART_TX_ENABLE						0x01
//#define		FLAG_OP_USART_RX_ENABLE					0x02

#define		FLAG_OP_EEPROM_INPROGRESS					0x04 // Currently an interrupt driven EEPROM procedure is in progress.
#define		FLAG_OP_USART_HANDLE_COMMAND				0x08
#define		FLAG_OP_EEPROM_ISWRITING					0x10 // when 1 and EEPROM_INPROGRESS is set, we're writing, when 0 and EEPROM_INPROGRESS is set, we're reading.
#define		FLAG_OP_GOTO_BOOTLOADER_AFTERTX				0x20 // When the transmission is done, reboot to the bootloader
#define		FLAG_OP_SET_UBRR_AFTERTX					0x40 // write UBRR from the UBRR buffer after TX finished
#define		FLAG_OP__AFTERTX							0x80 // there was something else, but forgot right now, so keeping it reserved.

#endif // __LED_DRIVER_FLAGS_H__


