/*=============================================================================
 * File        : lcd.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for the 16x2 character LCD driver (4-bit mode).
 *===========================================================================*/
#ifndef LCD_H
#define LCD_H

#include "types.h"

void lcd_init(void);              /* Configure GPIO and initialise the LCD controller */
void lcd_cmd(u8 cmd);             /* Send a raw command byte (RS = 0)                 */
void lcd_data(u8 data);           /* Send a raw data/character byte (RS = 1)          */
void lcd_string(const char *str); /* Print a null-terminated string                   */
void lcd_clear(void);             /* Clear the display and home the cursor            */
void lcd_gotoxy(u8 row, u8 col);  /* Move the cursor to (row, col); row: 0 or 1        */
void lcd_int(s32 num);            /* Print a signed integer in decimal                */

#endif

