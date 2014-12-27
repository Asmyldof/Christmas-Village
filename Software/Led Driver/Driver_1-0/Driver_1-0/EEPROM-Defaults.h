/*

*/

#ifndef __LED_DRIVER_EEPROM_DEFAULTS_H__
#define __LED_DRIVER_EEPROM_DEFAULTS_H__

#include "Flags.h"

// Default EEPROM Preloads:
#define		DEF_DAY_TIME_TARGET_NUMBER_HOUSES	8 // during the day the device will slowly increase to this number
#define		DEF_NIGHT_TIME_TARGET_NUMBER_HOUSES	3 // at night time mode the device will slowly decrease to this number
#define		DEF_MINIMUM_NUMBER_OF_HOUSES		0 // used when trying to set a number of houses with serial interface
#define		DEF_MAIN_STEPDELAY					25 // number of 1s WDT interrupts per step in pattern
#define		DEF_RANDOM_STEP_DELAY				50 // number of 1s WDT interrupts per step in random
#define		DEF_NIGHTTIME_TICKS					80 // number of RandomSteps between target number houses decrements
#define		DEF_STARTUP_DELAY					240 // 240 quarter second startup: 1 minute
#define		DEF_UBRR_VALUE						7 // U2X=1, crystal 7.3728MHz --> 7=115.2kBaud; 17 -> 57.6kBaud; 31 -> 28.8kBaud; 63 -> 14.4kBaud; 191 -> 4.8kBaud
// Make sure the resulting speed is no less than 14.4k for triggering the bootloader without time-out interrupts (these can be very
//    difficult to debug)
// If the bootloader procedure is preceded by setting the UBRR to a number below 63, the default can be anything, but do note: it is only an 8bit value, the upper UBRR byte
//     will always be zero (let's be honest, speeds below 9.6k are quite pointless this century anyway.

// build up startup defaults from flags in Flags.h:
#define		DEF_MAINFLAGS						FLAG_MAIN_NOEFFECT_HOUSE_STATES_ON | FLAG_MAIN_ENABLE_EFFECTS


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



#endif // __LED_DRIVER_EEPROM_DEFAULTS_H__


