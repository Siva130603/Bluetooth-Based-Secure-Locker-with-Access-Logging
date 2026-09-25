/*=============================================================================
 * File        : eeprom.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for the I2C-based external EEPROM used as
 *               non-volatile password storage.
 *===========================================================================*/
#ifndef EEPROM_H
#define EEPROM_H

#include "types.h"

void i2c_init(void);                                            /* Configure I2C0 for talking to the EEPROM */
void eeprom_byte_write(u16 addr, u8 data);                       /* Write a single byte at 'addr'            */
u8   eeprom_byte_read(u16 addr);                                 /* Read a single byte from 'addr'           */
void eeprom_write_str(u16 addr, const char *str, u8 len);        /* Write 'len' bytes starting at 'addr'     */
void eeprom_read_str(u16 addr, char *buf, u8 len);               /* Read 'len' bytes starting at 'addr'      */

#endif

