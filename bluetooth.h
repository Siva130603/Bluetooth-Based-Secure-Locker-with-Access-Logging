/*=============================================================================
 * File        : bluetooth.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for the interrupt-driven HC-05 Bluetooth command
 *               receiver running over UART1.
 *===========================================================================*/
#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "types.h"

void bluetooth_init(u32 baud);          /* Configure UART1 + enable the RX interrupt for the HC-05 module */
u8   bluetooth_available(void);         /* Returns non-zero once a complete command has been received      */
void bluetooth_read_command(char *buf); /* Copy the received command into buf and clear the "ready" flag    */
void bluetooth_clear(void);             /* Discard any partially received command                          */
u8   bluetooth_pending(void);           /* Returns non-zero if digits have arrived but no '#' yet           */

#endif

