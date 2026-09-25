/*=============================================================================
 * File        : rtc.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for the LPC2148's on-chip Real-Time Clock (RTC),
 *               used to timestamp every access-log entry.
 *===========================================================================*/
#ifndef RTC_H
#define RTC_H

#include "types.h"

void rtc_init(void);                                   /* Reset and start the on-chip RTC                     */
void rtc_set_time(u8 hh, u8 mm, u8 ss);                 /* Set the current time (24-hour format)               */
void rtc_set_date(u8 dd, u8 mon, u16 yy);               /* Set the current date                                 */
void rtc_get_stamp(char *buf);                          /* Format "DD/MM/YYYY HH:MM:SS" into buf (20 bytes min) */

void rtc_save_to_eeprom(void);      /* Persist the current Date/Time/Day to EEPROM (called after any admin edit) */
u8   rtc_restore_from_eeprom(void); /* Reload a previously saved Date/Time/Day; returns 0 if none was ever saved */

#endif

