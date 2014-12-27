/*

*/

#ifndef __LED_DRIVER_FLAGS_H__
#define __LED_DRIVER_FLAGS_H__


#define		FLAG_MAIN_ENABLE_EFFECTS					0x10
#define		FLAG_MAIN_NOEFFECT_HOUSE_STATES_ON			0x01

#define		FLAG_OP_USART_TX_ENABLE						0x01
#define		FLAG_OP_USART_RX_ENABLE						0x02
#define		FLAG_OP_EEPROM_INPROGRESS					0x04 // Currently an interrupt driven EEPROM procedure is in progress.
#define		FLAG_OP_USART_HANDLE_COMMAND				0x08


#endif // __LED_DRIVER_FLAGS_H__


