/*=============================================================================
 * File        : delay.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Blocking delay routines built on the LPC2148's Timer0,
 *               used throughout the project (LCD timing, keypad debounce,
 *               I2C timing, buzzer patterns, motor run time, etc).
 *
 * PCLK is assumed to be 15 MHz (see the PLL configuration in
 * projectmain.c). Timer0 is configured in timer mode with a prescaler
 * (T0PR) chosen so that T0TC increments once per millisecond or once per
 * microsecond, letting the delay simply be "wait until T0TC reaches the
 * requested count".
 *
 * T0CTCR = 0x00 -> Timer0 counts PCLK cycles (timer mode, not counter mode)
 * T0TCR  = 0x02 -> Reset the timer counter and prescale counter
 * T0TCR  = 0x01 -> Enable (start) the timer counter
 *===========================================================================*/
#include <lpc214x.h>
#include "delay.h"

/* Busy-wait for approximately 'ms' milliseconds.
 * Prescaler = 15000 -> with PCLK = 15 MHz, T0TC increments once every
 * 15000 / 15,000,000 s = 1 ms, so waiting for T0TC == ms gives a
 * millisecond-accurate delay. */
void delay_ms(u32 ms)
{
    T0CTCR = 0x00;         /* Plain timer mode                          */
    T0PR   = 15000 - 1;    /* Prescaler: 1 timer tick every 1 ms         */
    T0TC   = 0x00;         /* Reset the tick counter                     */
    T0TCR  = 0x02;         /* Reset the timer (counter + prescaler)      */
    T0TCR  = 0x01;         /* Start the timer                            */
    while (T0TC < ms);     /* Busy-wait until the requested time elapses */
    T0TCR  = 0x00;         /* Stop the timer                             */
}

/* Busy-wait for approximately 'us' microseconds.
 * Prescaler = 15 -> with PCLK = 15 MHz, T0TC increments once every
 * 15 / 15,000,000 s = 1 us. */
void delay_us(u32 us)
{
    T0CTCR = 0x00;
    T0PR   = 15 - 1;       /* Prescaler: 1 timer tick every 1 us */
    T0TC   = 0x00;
    T0TCR  = 0x02;
    T0TCR  = 0x01;
    while (T0TC < us);
    T0TCR  = 0x00;
}

