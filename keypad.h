/*=============================================================================
 * File        : keypad.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for the 4x4 matrix keypad driver.
 *===========================================================================*/
#ifndef KEYPAD_H
#define KEYPAD_H

#include "types.h"

void keypad_init(void);  /* Configure row (output) and column (input) GPIO lines */
u8   keypad_scan(void);  /* Scan once; returns the pressed key's ASCII code, or 0 if none */
u8   keypad_getkey(void); /* Block until a key is pressed, then return its ASCII code */

#endif

