/*=============================================================================
 * File        : delay.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for simple blocking millisecond/microsecond
 *               delays based on Timer0.
 *===========================================================================*/
#ifndef DELAY_H
#define DELAY_H

#include "types.h"

void delay_ms(u32 ms);   /* Block for approximately 'ms' milliseconds */
void delay_us(u32 us);   /* Block for approximately 'us' microseconds */

#endif

