/*=============================================================================
 * File        : motor.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Drives the locker's DC motor through an L293D H-bridge
 *               driver IC. Only 2 direction-control lines are used; the
 *               L293D enable pin is assumed tied HIGH (always enabled) or
 *               wired to a fixed logic level outside of this driver.
 *
 * Wiring (GPIO Port 1):
 *   IN1 -> P1.24   (L293D input 1 for this motor channel)
 *   IN2 -> P1.25   (L293D input 2 for this motor channel)
 *
 * Direction truth table:
 *   Forward : IN1 = 1, IN2 = 0
 *   Reverse : IN1 = 0, IN2 = 1
 *   Stop    : IN1 = 0, IN2 = 0   (both inputs low -> motor coasts/brakes)
 *===========================================================================*/
#include <lpc214x.h>
#include "motor.h"

#define MOTOR_IN1   (1UL << 24)   /* P1.24 - L293D IN1 */
#define MOTOR_IN2   (1UL << 25)   /* P1.25 - L293D IN2 */

/* Configure both direction-control pins as outputs and start with the
 * motor stopped so the locker does not move unexpectedly at power-up. */
void motor_init(void)
{
    PINSEL2 = 0x00000000;                  /* Ensure Port 1 pins used are plain GPIO */
    IO1DIR |= (MOTOR_IN1 | MOTOR_IN2);      /* IN1, IN2 as outputs */
    IO1CLR  = (MOTOR_IN1 | MOTOR_IN2);      /* Start in the stopped state */
}

/* Drive the motor forward (used to open the locker). */
void motor_forward(void)
{
    IO1SET = MOTOR_IN1;   /* IN1 = 1 */
    IO1CLR = MOTOR_IN2;   /* IN2 = 0 */
}

/* Drive the motor in reverse (used to close the locker). */
void motor_reverse(void)
{
    IO1SET = MOTOR_IN2;   /* IN2 = 1 */
    IO1CLR = MOTOR_IN1;   /* IN1 = 0 */
}

/* Stop the motor by driving both inputs low. */
void motor_stop(void)
{
    IO1CLR = (MOTOR_IN1 | MOTOR_IN2);
}

