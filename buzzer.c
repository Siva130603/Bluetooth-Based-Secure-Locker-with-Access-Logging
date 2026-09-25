/*=============================================================================
 * File        : buzzer.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Simple GPIO-driven buzzer used to give an audible alert on
 *               wrong-password and tamper-detection events.
 *
 * Wiring:
 *   Buzzer (+) -> P1.26 (through a driver transistor if the buzzer draws
 *                 more current than the LPC2148 GPIO pin can safely supply)
 *   Buzzer (-) -> GND
 *===========================================================================*/
#include <lpc214x.h>
#include "buzzer.h"
#include "delay.h"

#define BUZZER_PIN   (1UL << 26)   /* P1.26 - buzzer control line */

/* Configure the buzzer pin as an output and make sure it starts silent. */
void buzzer_init(void)
{
    PINSEL2 = 0x00000000;     /* Ensure Port 1 pins used are plain GPIO */
    IO1DIR |= BUZZER_PIN;     /* Buzzer pin as output */
    IO1CLR  = BUZZER_PIN;     /* Start with the buzzer off */
}

/* Turn the buzzer on (continuous tone). */
void buzzer_on(void)
{
    IO1SET = BUZZER_PIN;
}

/* Turn the buzzer off. */
void buzzer_off(void)
{
    IO1CLR = BUZZER_PIN;
}

/* Sound the buzzer in a simple on/off pattern, repeated 'cycles' times,
 * with a 300 ms on / 300 ms off period. Used to signal wrong passwords or
 * a detected tamper condition. */
void buzzer_alert(u8 cycles)
{
    u8 i;
    for (i = 0; i < cycles; i++)
    {
        buzzer_on();
        delay_ms(300);
        buzzer_off();
        delay_ms(300);
    }
}

