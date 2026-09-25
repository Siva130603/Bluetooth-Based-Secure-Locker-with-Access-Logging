/*=============================================================================
 * File        : projectmain.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Target      : NXP/Philips LPC2148 (ARM7TDMI-S) @ 60 MHz core clock
 *
 * SYSTEM OVERVIEW
 * ----------------
 * A two-factor locker: the user first sends a 4-digit password from a
 * phone over Bluetooth (Level-1), and if correct is then prompted to
 * enter a second 4-digit password on a physical keypad (Level-2). Only
 * when both match does the microcontroller drive a DC motor (via an
 * L293D H-bridge) to open, then automatically close, the locker. Every
 * significant event (boot, tamper, password results, locker open/close,
 * admin actions) is timestamped using the on-chip RTC and streamed out
 * over UART0 as a simple audit log for a PC to capture.
 *
 * An admin push-button (external interrupt EINT2) opens a separate menu
 * (see menu.c) for RTC Setting (time/date/day) and changing either
 * stored password; a tamper switch is continuously monitored and
 * triggers an alarm (LCD message + buzzer + log entry) if the enclosure
 * is opened. The RTC Setting menu was merged in from the separate
 * "EnviroTime" project and rewritten to use this project's own LCD/
 * keypad/buzzer/RTC/EEPROM drivers. (The old Alarm sub-menu that was
 * also merged in, and its check_alarm() main-loop hook, have since been
 * removed at the admin's request.)
 *
 * Once the locker has been opened and closed after a successful two-
 * factor unlock, the LCD shows a live RTC clock screen (date + day on
 * line 1, time centred on line 2) instead of the usual "Waiting BT Pwd"
 * idle screen. This clock screen keeps waiting for either the next
 * Bluetooth password attempt or the admin button, the same as the
 * normal idle wait; if the admin button is not pressed within 5
 * seconds the LCD simply reverts to the plain idle screen while the
 * wait for a Bluetooth command continues unchanged. This repeats after
 * every successful open/close operation.
 *
 * At power-up the LCD splash screen reads "Secure Locker" / "BT Auth System".
 *
 * PIN CONNECTIONS
 * ----------------
 *   LCD (16x2, 4-bit mode):
 *     RS               -> P0.16
 *     EN               -> P0.17
 *     D4 .. D7         -> P0.18 .. P0.21
 *
 *   UART0 (PC debug / access-log console):
 *     TXD0             -> P0.0
 *     RXD0             -> P0.1
 *
 *   I2C0 (AT24C256 EEPROM - password storage):
 *     SCL0             -> P0.2
 *     SDA0             -> P0.3
 *     EEPROM VCC       -> 3V3
 *
 *   Tamper switch (active LOW, external pull-up to 3V3):
 *     Signal           -> P0.4
 *
 *   Admin push-button (EINT2, falling edge):
 *     Signal           -> P0.7
 *
 *   UART1 / HC-05 Bluetooth module:
 *     TXD1 (MCU -> HC-05 RXD) -> P0.8
 *     RXD1 (MCU <- HC-05 TXD) -> P0.9
 *     HC-05 GND        -> GND
 *     HC-05 VCC        -> 5V
 *
 *   4x4 matrix keypad:
 *     Rows (outputs)   -> P1.16 .. P1.19
 *     Cols (inputs)    -> P1.20 .. P1.23
 *
 *   DC motor (locker latch actuator, via L293D H-bridge):
 *     IN1              -> P1.24
 *     IN2              -> P1.25
 *
 *   Buzzer (audible alert):
 *     Signal           -> P1.26
 *
 * DEFAULT / FACTORY PASSWORDS (written to EEPROM on first boot only)
 * ----------------
 *   Level-1 (Bluetooth, sent as "1234#" from the phone app) = 1234
 *   Level-2 (keypad, entered after Level-1 succeeds)          = 5678
 *===========================================================================*/
#include <lpc214x.h>
#include <string.h>

#include "types.h"
#include "defines.h"
#include "delay.h"
#include "lcd.h"
#include "keypad.h"
#include "buzzer.h"
#include "motor.h"
#include "uart.h"
#include "bluetooth.h"
#include "eeprom.h"
#include "rtc.h"
#include "security.h"
#include "menu.h"

static void pll_feed(void);
static void SystemInit_SecureLocker(void);
static void DisplayStandby(void);
static void DisplayAccessGranted(void);
static void DisplayAccessDenied(const char *reason);
static void DisplayRTCNow(void);
static void rtc_idle_and_wait(void);
static void bt_watchdog_poll(u32 poll_interval_ms);
static void read_keypad_password(char *buf);
static void open_locker_sequence(void);

/* How long the post-close RTC clock screen is shown before the LCD
 * reverts to the plain idle screen if the admin button hasn't been
 * pressed; the underlying wait for a Bluetooth command is unaffected
 * and simply continues either way. */
#define RTC_IDLE_REVERT_MS   5000U
#define RTC_IDLE_POLL_MS      200U

/* Required "feed" sequence (0xAA then 0x55 written to PLL0FEED) that must
 * follow any write to the PLL control/configuration registers for the
 * change to actually take effect; this is a fixed hardware requirement
 * of the LPC2148's PLL, not project-specific logic. */
static void pll_feed(void)
{
    PLL0FEED = 0xAA;
    PLL0FEED = 0x55;
}

/* One-time hardware bring-up: configure the PLL for a 60 MHz core clock
 * from a 12 MHz crystal, set up the Memory Accelerator Module (MAM) for
 * that clock speed, then initialise every peripheral driver used by the
 * project (LCD, both UARTs, I2C/EEPROM, RTC, keypad, buzzer, motor,
 * tamper input, and the admin-button interrupt). */
static void SystemInit_SecureLocker(void)
{
    /* PLL setup:
       Crystal = 12MHz
       CCLK    = 60MHz
       PCLK    = 15MHz */
    MAMCR = 0x00;              /* Disable the MAM while changing clock speed */

    PLL0CON = 0x00;            /* Disable the PLL before reconfiguring it     */
    pll_feed();

    PLL0CFG = 0x24;            /* M=5, P=2 -> CCLK = 12MHz * (M+1) = 60MHz    */
    pll_feed();

    PLL0CON = 0x01;            /* Enable the PLL (not yet connected)          */
    pll_feed();

    while (!(PLL0STAT & (1UL << 10)));   /* Wait for the PLL to lock */

    PLL0CON = 0x03;            /* Enable AND connect the PLL as the clock source */
    pll_feed();

    VPBDIV = 0x00;             /* PCLK = CCLK / 4 = 15MHz (VPB divider = 1/4) */

    MAMTIM = 0x04;             /* MAM fetch cycles tuned for 60MHz operation  */
    MAMCR  = 0x02;             /* Re-enable the MAM in fully-enabled mode      */

    /* Bring up every peripheral driver used by the system. */
    lcd_init();
    uart0_init(9600);
    bluetooth_init(9600);
    i2c_init();
    rtc_init();
    keypad_init();
    buzzer_init();
    motor_init();
    security_init();
    admin_int_init();
}

/* Show the idle/waiting screen while the system waits for a Bluetooth
 * password attempt. */
static void DisplayStandby(void)
{
    lcd_clear();
    lcd_string("Waiting BT Pwd");
    lcd_gotoxy(1,0);
    lcd_string("Send from App#");
}

/* Show the "access granted" message on the LCD. */
static void DisplayAccessGranted(void)
{
    lcd_clear();
    lcd_string("ACCESS GRANTED!");
    lcd_gotoxy(1,0);
    lcd_string("Opening Locker..");
}

/* Show the "access denied" message on the LCD (short reason on line 2,
 * matching what's shown), log the specific reason for the audit trail,
 * and sound the buzzer as feedback. */
static void DisplayAccessDenied(const char *reason)
{
    lcd_clear();
    lcd_string("ACCESS DENIED!");
    lcd_gotoxy(1,0);
    lcd_string(reason);
    log_event(reason);
    buzzer_alert(5);
}

/* Show a live RTC clock screen on the 16x2 LCD:
 *   Line 1 (left-aligned) : DD/MM/YYYY DOW   e.g. "05/08/2026 WED"
 *   Line 2 (centred)      : HH:MM:SS         e.g. "   12:34:56    "
 * "HH:MM:SS" is 8 characters, so it is centred on the 16-column line
 * by starting it at column 4 (4 blank columns on each side). */
static void DisplayRTCNow(void)
{
    static const char *dow_names[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
    u8  d  = (u8)DOM;
    u8  m  = (u8)MONTH;
    u16 y  = (u16)YEAR;
    u8  hh = (u8)HOUR;
    u8  mm = (u8)MIN;
    u8  ss = (u8)SEC;
    u8  dw = (u8)DOW;

    lcd_clear();

    lcd_gotoxy(0, 0);
    lcd_data((d / 10) + '0');
    lcd_data((d % 10) + '0');
    lcd_data('/');
    lcd_data((m / 10) + '0');
    lcd_data((m % 10) + '0');
    lcd_data('/');
    lcd_data((u8)((y / 1000) % 10) + '0');
    lcd_data((u8)((y / 100)  % 10) + '0');
    lcd_data((u8)((y / 10)   % 10) + '0');
    lcd_data((u8)(y % 10) + '0');
    lcd_data(' ');
    lcd_string((dw <= 6) ? dow_names[dw] : "???");

    lcd_gotoxy(1, 4);
    lcd_data((hh / 10) + '0');
    lcd_data((hh % 10) + '0');
    lcd_data(':');
    lcd_data((mm / 10) + '0');
    lcd_data((mm % 10) + '0');
    lcd_data(':');
    lcd_data((ss / 10) + '0');
    lcd_data((ss % 10) + '0');
}

/* Call once per idle-loop poll (from either the plain standby wait or
 * rtc_idle_and_wait) with that loop's poll interval. Tracks how long a
 * half-typed Bluetooth command (digits received but no '#' yet) has been
 * sitting unfinished; if it exceeds BT_PENDING_TIMEOUT_MS, the buffer is
 * discarded via bluetooth_clear() so a later, separate send doesn't get
 * silently appended onto stale leftovers (e.g. "7896" sent without '#',
 * then "7896#" sent later, would otherwise concatenate into
 * "78967896#" - a real password wrapped in a command that no longer
 * matches). Resets its internal timer as soon as the buffer is empty
 * again, ready to time the next half-typed command from zero. */
static void bt_watchdog_poll(u32 poll_interval_ms)
{
    static u32 pending_ms = 0;

    if (!bluetooth_pending())
    {
        pending_ms = 0;
        return;
    }

    pending_ms += poll_interval_ms;

    if (pending_ms >= BT_PENDING_TIMEOUT_MS)
    {
        bluetooth_clear();
        pending_ms = 0;
        log_event("Bluetooth: stale partial command auto-cleared (no '#' received in time)");
    }
}

/* Shown after the locker has closed following a successful two-factor
 * unlock. Displays the live RTC clock screen and keeps waiting for
 * either a fresh Bluetooth password attempt or the admin button -
 * exactly like the normal idle wait in the main loop - but if the
 * admin button hasn't been pressed within RTC_IDLE_REVERT_MS the LCD
 * content switches over to the plain idle screen; the wait for a
 * Bluetooth command is unaffected and simply continues in the
 * background either way. Returns as soon as either event occurs, so
 * the main loop can service the admin menu or the Bluetooth command. */
static void rtc_idle_and_wait(void)
{
    u32 elapsed_ms = 0;

    while (1)
    {
        check_tamper_and_alert();

        if (tamper_is_active())
            return;   /* Let the main loop take over and hold the alert screen */

        if (elapsed_ms < RTC_IDLE_REVERT_MS)
            DisplayRTCNow();
        else
            DisplayStandby();

        bt_watchdog_poll(RTC_IDLE_POLL_MS);

        if (admin_flag)
            return;   /* Let the main loop service the admin menu */

        if (bluetooth_available())
            return;   /* Let the main loop process the Bluetooth command */

        delay_ms(RTC_IDLE_POLL_MS);
        elapsed_ms += RTC_IDLE_POLL_MS;
    }
}

/* Read the Level-2 password from the keypad with basic line-editing:
 *   0-9  -> append the digit (masked with '*' on the LCD)
 *   '*'  -> backspace: erase the previous digit from both the buffer
 *           and the LCD
 *   '#'  -> clear the entire entry and start again from the beginning
 * Any other key is ignored. Returns once exactly PWD_LEN digits have
 * been accepted. */
static void read_keypad_password(char *buf)
{
    u8 i = 0;
    char k;

    while (i < PWD_LEN)
    {
        k = keypad_getkey();
        if(k>='0' && k<='9')
        {
          buf[i++]=k;
          lcd_data('*');
        }
        else if(k == '*')          /* Backspace: remove the last entered digit */
        {
          if(i>0)
          {
            i--;
            buf[i] ='\0';

            lcd_gotoxy(1, i);       /* Move cursor back to the erased position */
            lcd_data(' ');           /* Blank out the '*' that was shown there   */
            lcd_gotoxy(1, i);        /* Leave the cursor ready for the next digit */
          }
        }
        else if(k == '#')          /* Clear: erase the whole entry so far */
        {
          while(i>0)
          {
             i--;
             buf[i] = '\0';

             lcd_gotoxy(1, i);
             lcd_data(' ');
          }
          lcd_gotoxy(1, 0);
        }
        // buf[i++] = k;
      //  lcd_data('*');
    }
    buf[PWD_LEN] = '\0';
}

/* Runs the physical locker actuation sequence once both passwords have
 * matched: show "access granted", turn the motor forward ONCE for a
 * single ~180-degree turn to open the locker (gate-style), hold it open,
 * then turn the motor in reverse ONCE for the same duration to bring it
 * back to its original (closed) position. Every stage is written to the
 * audit log.
 *
 * A "busy" guard prevents this function from ever being re-entered or
 * overlapped (e.g. if it were accidentally called again while already
 * running) - only one forward pulse and one reverse pulse can ever be in
 * flight, which is what stops the motor from appearing to run forward
 * and reverse "at the same time". */
static void open_locker_sequence(void)
{
    static u8 locker_busy = 0;

    if (locker_busy)
        return;         /* Already mid-sequence - ignore a duplicate call */
    locker_busy = 1;

    DisplayAccessGranted();
    log_event("Access granted, opening locker");

    /* --- Single forward pulse: turn the gate/latch open ~180 degrees --- */
    motor_stop();                 /* Make sure we start from a full stop   */
    motor_forward();
    delay_ms(MOTOR_ROTATE_MS);    /* One short, tuned pulse - NOT a multi-second
                                    * continuous run, so the motor turns once
                                    * instead of spinning through several
                                    * full rotations. */
    motor_stop();

    log_event("Locker opened");

    lcd_clear();
    lcd_string("Locker OPEN");
    lcd_gotoxy(1,0);
    lcd_string("Take your item");
    delay_ms(5000);              /* Hold the gate open for the visitor    */

    lcd_clear();
    lcd_string("LOCKER CLOSE");

    /* --- Brief full-stop settle time before reversing direction --------
     * Reversing an H-bridge output straight from one direction to the
     * other while the motor still has momentum can cause a mechanical/
     * electrical jolt; a short stop first avoids that. */
    delay_ms(MOTOR_SETTLE_MS);

    /* --- Single reverse pulse: return the gate/latch to its original
     * (closed) position, using the exact same duration as the opening
     * pulse so it ends up back where it started. --- */
    motor_reverse();
    delay_ms(MOTOR_ROTATE_MS);
    motor_stop();

    log_event("Locker closed");

    /* Confirmation screen, matching the reference project, shown for 2 s
     * before the RTC idle screen (see rtc_idle_and_wait) takes over. */
    lcd_clear();
    lcd_string("Locker LOCKED");
    lcd_gotoxy(1,0);
    lcd_string("Session Complete");
    delay_ms(2000);

    locker_busy = 0;
}

int main(void)
{
    char bt_cmd[BT_BUF_SIZE];
    char l1_pwd[PWD_LEN + 1];
    char l2_pwd[PWD_LEN + 1];
    char kp_pwd[PWD_LEN + 1];
    u8   show_rtc_idle = 0;   /* Set after a successful open/close cycle so the
                                * next idle wait shows the RTC clock screen  */

    SystemInit_SecureLocker();

    /* Restore whatever Date/Time/Day the admin last configured, so a
     * reset/reflash doesn't silently discard it. Only on a genuinely
     * first-ever boot (no saved value in EEPROM yet) does it fall back
     * to a fixed default - the admin can then set the real date/time via
     * the admin menu, which persists it for every boot after this one. */
    if (!rtc_restore_from_eeprom())
    {
        rtc_set_date(1, 1, 2024);
        rtc_set_time(12, 0, 0);
    }

    /* On the very first boot (or if the EEPROM was blank/corrupted),
     * populate it with the factory-default passwords. */
    ensure_default_passwords();

    lcd_clear();
    lcd_string("Secure Locker");
    lcd_gotoxy(1,0);
    lcd_string("BT Auth System");

    uart0_string("\r\n====================================\r\n");
    uart0_string(" Bluetooth Secure Locker Started\r\n");
    uart0_string("====================================\r\n");

    log_event("System booted");

    delay_ms(2000);

    while (1)
    {
        /* Highest priority: if the admin button was pressed, service
         * the admin menu before doing anything else. */
        if (admin_flag)
        {
            log_event("Admin button pressed");
            admin_menu();
        }

        check_tamper_and_alert();

        if (tamper_is_active())
        {
            /* Tamper alert is showing on the LCD - leave it there instead
             * of letting DisplayStandby()/DisplayRTCNow() overwrite it,
             * and refuse to process Bluetooth/keypad auth until the
             * switch/loop reads closed again. The admin button above
             * still works, so the admin can investigate without needing
             * the tamper condition to clear first. */
            bt_watchdog_poll(200);
            delay_ms(200);
            continue;
        }

        if (show_rtc_idle)
        {
            /* Locker just closed after a successful unlock: show the
             * live RTC clock screen while waiting (see rtc_idle_and_wait
             * for exactly what it does and when it reverts). */
            show_rtc_idle = 0;
            log_event("Idle: showing RTC clock screen, waiting for BT/admin");
            rtc_idle_and_wait();
        }
        else
        {
            log_event("System ready - waiting for Bluetooth password");
            DisplayStandby();

            /* Wait for a Bluetooth command to arrive, while still polling
             * the tamper switch and bailing out early if the admin button
             * is pressed during the wait. */
            while (!bluetooth_available())
            {
                check_tamper_and_alert();

                if (tamper_is_active())
                    break;

                bt_watchdog_poll(100);

                if (admin_flag)
                    break;

                delay_ms(100);
            }
        }

        if (admin_flag)
             continue;   /* Go back to the top of the loop and open the admin menu */

        if (tamper_is_active())
             continue;   /* Go back to the top; the tamper-halt branch holds the alert screen */

        bluetooth_read_command(bt_cmd);

        if (bt_cmd[0] == '\0')
            continue;   /* Ignore an empty command (e.g. a stray terminator) */

        log_event("Bluetooth command received");
        log_prefix();
        uart0_string("[BT] Received password: ");
        uart0_string(bt_cmd);
        uart0_string("\r\n");

        /* --- Level 1: check the Bluetooth password --- */
        eeprom_read_str(EEPROM_L1_ADDR, l1_pwd, PWD_LEN);

        log_prefix();
        uart0_string("[BT] Stored Level-1 password: ");
        uart0_string(l1_pwd);
        uart0_string("\r\n");

        if (strcmp(bt_cmd, l1_pwd) == 0)
        {
            lcd_clear();
            lcd_string("L1 Auth OK!");
            lcd_gotoxy(1,0);
            lcd_string("Enter L2 Pass:");

            log_event("Level-1 Bluetooth password matched");
            delay_ms(1000);

            lcd_clear();
            lcd_string("KEYPAD PWD:");
            lcd_gotoxy(1,0);

            log_event("Waiting for Level-2 keypad password");

            /* --- Level 2: prompt for and check the keypad password --- */
            read_keypad_password(kp_pwd);

            eeprom_read_str(EEPROM_L2_ADDR, l2_pwd, PWD_LEN);

            log_prefix();
            uart0_string("[KEYPAD] Entered password: ");
            uart0_string(kp_pwd);
            uart0_string("\r\n");
            log_prefix();
            uart0_string("[KEYPAD] Stored Level-2 password: ");
            uart0_string(l2_pwd);
            uart0_string("\r\n");

            if (strcmp(kp_pwd, l2_pwd) == 0)
            {
                log_event("Level-2 keypad password matched - ACCESS GRANTED");
                open_locker_sequence();

                /* Next idle wait shows the RTC clock screen instead of
                 * the plain "Waiting BT Pwd" standby screen. */
                show_rtc_idle = 1;
            }
            else
            {
                DisplayAccessDenied("Wrong KPD Pass");
            }
        }
        else
        {
            log_event("Level-1 Bluetooth password MISMATCH - ACCESS DENIED");
            DisplayAccessDenied("Wrong BT Pass");
        }

        delay_ms(1000);
    }
}

