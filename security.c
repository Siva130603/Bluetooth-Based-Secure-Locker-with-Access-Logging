/*=============================================================================
 * File        : security.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Ties together tamper detection, RTC-timestamped access
 *               logging (sent out over UART0 for a PC to capture), and
 *               one-time initialisation of the default EEPROM passwords.
 *
 * Wiring:
 *   Tamper switch -> P0.4, active LOW (switch pulls the pin to GND when
 *                    the enclosure is opened/tampered with; an external
 *                    pull-up resistor keeps the pin HIGH when closed)
 *===========================================================================*/
#include <lpc214x.h>
#include <string.h>
#include "security.h"
#include "defines.h"
#include "lcd.h"
#include "uart.h"
#include "rtc.h"
#include "eeprom.h"
#include "buzzer.h"

#define TAMPER_PIN   (1UL << 4)   /* P0.4 - tamper switch input */

/* Configure the tamper switch pin as a GPIO input. */
void security_init(void)
{
    PINSEL0 &= ~(3UL << 8);   /* P0.4 as plain GPIO (not an alternate function) */
    IO0DIR  &= ~TAMPER_PIN;   /* Configure as input */
}

/* Read the current state of the tamper switch.
 * The switch is active LOW, so a HIGH pin reading means "not tampered"
 * (returns 0) and a LOW reading means "tampered" (returns 1). */
u8 tamper_detected(void)
{
    if (IO0PIN & TAMPER_PIN) return 0;   /* Pin HIGH -> switch closed -> OK       */
    else                     return 1;   /* Pin LOW  -> switch open  -> tampered  */
}

/* Print just the "[DD/MM/YYYY HH:MM:SS] " timestamp bracket, with no
 * trailing newline. Used by multi-part UART lines (e.g. "[BT] Received
 * password: 1234") that build up more than one piece on the same line,
 * so every line on the terminal - not just single-message ones - carries
 * the same timestamp format. */
void log_prefix(void)
{
    char stamp[20];
    rtc_get_stamp(stamp);

    uart0_string("[");
    uart0_string(stamp);
    uart0_string("] ");
}

/* Print a single timestamped log line over UART0 in the form:
 * "[DD/MM/YYYY HH:MM:SS] <message>\r\n"
 * This is intended to be captured by a PC terminal for an audit trail. */
void log_event(const char *msg)
{
    log_prefix();
    uart0_string(msg);
    uart0_string("\r\n");
}

/* Module-level (not just a one-call-scope local) so tamper_is_active()
 * can report the current state to the main loop, which uses it to halt
 * normal operation (no Bluetooth/keypad auth, no RTC idle screen) for as
 * long as the tamper condition remains unresolved - see the note on
 * check_tamper_and_alert() below for exactly what "unresolved" covers. */
static u8 tamper_active = 0U;

/* Poll the tamper switch/loop once.
 *
 * On a fresh transition into "tampered" (edge-detected via tamper_active
 * so the alert only fires once per event, not continuously), this shows
 * a persistent "TAMPER ALERT" screen, logs the event, and sounds the
 * buzzer. Unlike a one-shot message, this screen is NOT overwritten by
 * the normal standby/RTC display: the main loop checks tamper_is_active()
 * and skips its normal display/auth logic entirely while it's set, so
 * the alert stays visible - and the system refuses Bluetooth/keypad
 * unlock attempts - until the switch/loop reads closed again.
 *
 * IMPORTANT WIRING NOTE: this can only ever detect what's actually wired
 * to the single tamper input (P0.4). It does NOT, on its own, detect a
 * wire being cut/removed from the LCD, motor, keypad, buzzer, EEPROM, or
 * Bluetooth connectors - there is no such thing as an MCU pin that
 * "watches" an arbitrary signal wire elsewhere in the circuit for
 * continuity. Also note that with a plain switch + external pull-up (as
 * wired here), cutting the switch's OWN wire looks identical to "switch
 * closed" (pin stays pulled HIGH) - it will NOT trigger an alert.
 *   To genuinely catch "any wire cut or connector pulled" - including the
 * tamper wire itself - the standard approach (used in real tamper-evident
 * enclosures) is a single normally-closed loop: run one continuous fine
 * wire in series through every connector/header on the board and back to
 * this same P0.4 input, with a pull-DOWN (not pull-up) on the pin.
 * As long as every connector is seated and every wire intact, the loop
 * stays closed and the pin reads HIGH; unplugging or cutting ANYTHING
 * along that loop - including the loop wire itself - breaks continuity
 * and pulls the pin LOW, which this same firmware logic already treats
 * as "tampered". No firmware change is needed for that upgrade beyond
 * this file - it's a wiring/enclosure change (see project docs). */
void check_tamper_and_alert(void)
{
    if (tamper_detected())
    {
        if (tamper_active == 0U)   /* Only trigger on the transition into "tampered" */
        {
            tamper_active = 1U;
            lcd_clear();
            lcd_string("** TAMPER **");
            lcd_gotoxy(1, 0);
            lcd_string("ALERT LOGGED");
            log_event("Tamper detected - normal operation halted");
            buzzer_alert(5);
        }
    }
    else
    {
        if (tamper_active == 1U)   /* Switch/loop closed again -> clear and re-arm */
        {
            tamper_active = 0U;
            log_event("Tamper cleared - resuming normal operation");
        }
    }
}

/* Non-blocking check for the main loop: while this returns 1, normal
 * operation (Bluetooth/keypad authentication, the RTC idle screen) is
 * suspended so the "TAMPER ALERT" screen stays on the LCD instead of
 * being overwritten. The admin button still works during this, so the
 * admin can enter the menu to investigate/reset without needing the
 * tamper condition to clear first. */
u8 tamper_is_active(void)
{
    return tamper_active;
}

/* On first boot (or after a blank/corrupted EEPROM), write the 4-byte
 * "LKR1" magic marker plus the two factory-default passwords
 * (Level-1/Bluetooth = "1234", Level-2/keypad = "5678") into the EEPROM.
 * On subsequent boots the marker will already match and this function
 * does nothing, preserving any passwords the admin has since changed. */
void ensure_default_passwords(void)
{
    char magic[5];

    eeprom_read_str(EEPROM_MAGIC_ADDR, magic, 4);

    if (strcmp(magic, "LKR1") != 0)
    {
        eeprom_write_str(EEPROM_MAGIC_ADDR, "LKR1", 4);
        eeprom_write_str(EEPROM_L1_ADDR, "1234", PWD_LEN);
        eeprom_write_str(EEPROM_L2_ADDR, "5678", PWD_LEN);
        log_event("EEPROM initialized with default passwords");
    }
}

