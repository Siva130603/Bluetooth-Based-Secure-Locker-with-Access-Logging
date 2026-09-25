/*=============================================================================
 * File        : menu.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for the admin menu, entered via an external
 *               push-button interrupt (EINT2). Provides a unified RTC
 *               Setting (Time/Date/Day) sub-menu on top of the existing
 *               Password-change function, using SecureLocker's own
 *               lcd/keypad/buzzer/rtc/eeprom drivers. The Alarm sub-menu
 *               that used to live here (and its check_alarm() main-loop
 *               hook) has been removed at the admin's request; only RTC
 *               Setting and Password remain alongside Set.
 *===========================================================================*/
#ifndef MENU_H
#define MENU_H

#include "types.h"

void admin_int_init(void);   /* Configure P0.7 as EINT2 and enable the admin-button interrupt */
void admin_menu(void);       /* Run the interactive admin menu (blocking) until the user exits */

/* Set to 1 by the EINT2 ISR when the admin button is pressed; the main
 * loop checks this flag and calls admin_menu() when it is set. */
extern volatile unsigned char admin_flag;

#endif

