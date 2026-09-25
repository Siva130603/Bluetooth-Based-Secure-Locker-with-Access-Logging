/*=============================================================================
 * File        : keypad.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Driver for a standard 4x4 matrix keypad used to enter the
 *               Level-2 password and to operate the admin menu.
 *
 * Wiring (all on GPIO Port 1):
 *   Rows (driven as outputs) -> P1.16, P1.17, P1.18, P1.19
 *   Cols (read as inputs, pulled up externally) -> P1.20, P1.21, P1.22, P1.23
 *
 * Scanning method: rows are held HIGH, then one row at a time is pulled
 * LOW; if a key in that row is pressed, its column line will read LOW.
 *===========================================================================*/
#include <lpc214x.h>
#include "keypad.h"
#include "delay.h"

#define ROW_MASK   (0x0FUL << 16)   /* P1.16-P1.19 : the 4 row lines (outputs) */
#define COL_MASK   (0x0FUL << 20)   /* P1.20-P1.23 : the 4 column lines (inputs) */

/* Standard 4x4 keypad character layout, row-major order. */
static const u8 key_map[4][4] =
{
    {'1','2','3','4'},
    {'5','6','7','8'},
    {'9','0','*','#'},
    {'A','B','C','D'}
};

/* Configure the row pins as outputs (idle HIGH) and the column pins as
 * inputs, ready for scanning. */
void keypad_init(void)
{
    PINSEL2 = 0x00000000;   /* Ensure Port 1 pins used here are plain GPIO */
    IO1DIR |=  ROW_MASK;    /* Rows  = outputs */
    IO1DIR &= ~COL_MASK;    /* Columns = inputs */
    IO1SET = ROW_MASK;      /* Idle state: all rows HIGH (no key pressed yet) */
}

/* Perform one full scan of the keypad matrix.
 * Returns the ASCII code of the first pressed key found, or 0 if no key
 * is currently pressed. Includes simple debounce + key-release wait so a
 * single press is never read as multiple repeated keys. */
u8 keypad_scan(void)
{
    u8 r, c;

    for (r = 0; r < 4; r++)
    {
        IO1SET = ROW_MASK;                 /* All rows HIGH ... */
        IO1CLR = (1UL << (16 + r));         /* ... except the row being scanned (LOW) */
        delay_us(50);                       /* Let the line settle before reading    */

        for (c = 0; c < 4; c++)
        {
            if (!(IO1PIN & (1UL << (20 + c))))   /* Column read LOW -> key pressed */
            {
                while (!(IO1PIN & (1UL << (20 + c))));  /* Wait for key release (blocking) */
                delay_ms(20);                            /* Debounce delay after release   */
                IO1SET = ROW_MASK;                       /* Restore rows to idle HIGH       */
                return key_map[r][c];                    /* Return the corresponding character */
            }
        }
    }

    IO1SET = ROW_MASK;   /* No key found in this pass; restore idle state */
    return 0;
}

/* Blocking helper: keep scanning until a key is actually pressed, then
 * return it. Used anywhere the code needs to wait for user input. */
u8 keypad_getkey(void)
{
    u8 key = 0;
    while (key == 0)
        key = keypad_scan();
    return key;
}

