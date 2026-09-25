/*=============================================================================
 * File        : security.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for tamper detection, the UART/RTC-timestamped
 *               access-logging helper, and first-boot password provisioning.
 *===========================================================================*/
#ifndef SECURITY_H
#define SECURITY_H

#include "types.h"

void security_init(void);              /* Configure the tamper-switch input pin           */
u8   tamper_detected(void);             /* Returns 1 if the tamper switch is currently open */
void check_tamper_and_alert(void);      /* Poll the tamper switch and raise an alert on change */
u8   tamper_is_active(void);            /* Returns 1 while an unresolved tamper alert is active */
void ensure_default_passwords(void);    /* Load factory-default passwords on first boot     */
void log_event(const char *msg);        /* Print a timestamped event line over UART0         */
void log_prefix(void);                  /* Print just the "[DD/MM/YYYY HH:MM:SS] " bracket -
                                          * used for multi-part lines that append a value      */

#endif

