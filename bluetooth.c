/*=============================================================================
 * File        : bluetooth.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Receives password commands from an HC-05 Bluetooth-to-serial
 *               module over UART1, using an interrupt service routine so
 *               the main loop never has to block waiting on individual
 *               bytes.
 *
 * HC-05 wiring (over UART1):
 *   HC-05 RXD <- P0.8  (LPC2148 TXD1)
 *   HC-05 TXD -> P0.9  (LPC2148 RXD1)
 *   HC-05 VCC -> 5V (module has its own onboard regulator/level shifting)
 *   HC-05 GND -> GND
 *
 * Protocol: the paired mobile phone/app sends the password digits followed
 * by a '#' terminator, e.g. "1234#". Any trailing carriage return / line
 * feed characters that the app appends after sending are ignored by the
 * receiver (see UART1_ISR below for why this matters).
 *===========================================================================*/
#include <lpc214x.h>
#include "bluetooth.h"
#include "defines.h"

/* Ring/line buffer that accumulates incoming characters until a
 * terminator is seen, plus the bookkeeping needed by the ISR and the
 * foreground (main-loop) code that eventually reads it out. */
volatile char bt_buffer[BT_BUF_SIZE];
volatile u8 bt_index = 0;       /* Current write position inside bt_buffer   */
volatile u8 bt_rx_ready = 0;    /* Set to 1 by the ISR once a full command has arrived */

__irq void UART1_ISR(void);

/* Configure UART1 for communication with the HC-05 module and enable its
 * receive-data-available interrupt on the VIC (Vectored Interrupt
 * Controller), channel 7 (UART1). */
void bluetooth_init(u32 baud)
{
    u32 divisor = 15000000UL / (16UL * baud);

    PINSEL0 &= ~(0x000F0000);              /* Clear P0.8/P0.9 function bits */
    PINSEL0 |=  (1UL << 16) | (1UL << 18); /* Select TXD1 (P0.8) / RXD1 (P0.9) */

    U1LCR = 0x83;                    /* 8N1, DLAB = 1 to program the divisor */
    U1DLL = divisor & 0xFF;
    U1DLM = (divisor >> 8) & 0xFF;
    U1LCR = 0x03;                    /* DLAB = 0, normal register access      */
    U1FCR = 0x07;                    /* Enable and reset TX/RX FIFOs           */

    U1IER = 0x01;                    /* Enable "Receive Data Available" interrupt */

    VICIntSelect &= ~(1UL << 7);     /* UART1 (VIC channel 7) -> IRQ, not FIQ */
    VICVectAddr1  = (u32)UART1_ISR;  /* Register our ISR in vectored slot 1   */
    VICVectCntl1  = 0x20 | 7;        /* Enable slot 1, assign it to source 7  */
    VICIntEnable  = (1UL << 7);      /* Unmask the UART1 interrupt source     */
}

/* UART1 receive interrupt handler: read every available byte from the
 * RX FIFO, appending it to bt_buffer. Only '#' terminates a command and
 * signals the main loop (via bt_rx_ready) that a full command is ready.
 *
 * IMPORTANT: '\r' and '\n' are deliberately IGNORED here, not treated as
 * additional terminators. Most Bluetooth serial-terminal apps
 * automatically append a trailing CR and/or LF after the text you send,
 * in addition to the '#' you typed (e.g. sending "1234#" actually
 * transmits '1','2','3','4','#','\n'). If that trailing '\n' were also
 * treated as a terminator, it would arrive in its own interrupt shortly
 * after the '#' - before the main loop (which only polls every 100 ms)
 * has had a chance to read the buffer - and would immediately overwrite
 * the just-received command with an empty string, making every correct
 * password look like "Access Denied". Treating '#' as the only
 * terminator, and simply discarding CR/LF bytes, avoids that. */
__irq void UART1_ISR(void)
{
    char ch;

    while (U1LSR & 0x01)   /* While Receiver Data Ready (RDR) bit is set */
    {
        ch = U1RBR;         /* Reading U1RBR also clears the RDR flag    */

        if (bt_rx_ready)
        {
            /* A completed command is still waiting for the main loop to
             * read it - discard anything further until it's consumed,
             * rather than letting it corrupt the pending command. */
            continue;
        }

        if (ch == '\r' || ch == '\n')
        {
            /* Ignore: not part of the password, not a terminator either -
             * prevents a trailing newline from clobbering a command that
             * is still waiting to be read by the main loop. */
        }
        else if (ch == '#')
        {
            bt_buffer[bt_index] = '\0';   /* Terminate the received string */
            bt_rx_ready = 1;              /* Tell the main loop it's ready */
            bt_index = 0;                 /* Reset for the next command    */
        }
        else
        {
            if (bt_index < (BT_BUF_SIZE - 1))
                bt_buffer[bt_index++] = ch;   /* Store the character (with overflow guard) */
        }
    }

    VICVectAddr = 0;   /* Acknowledge interrupt to the VIC (end-of-interrupt) */
}

/* Non-blocking check: returns non-zero once the ISR has assembled a
 * complete, terminated command. */
u8 bluetooth_available(void)
{
    return bt_rx_ready;
}

/* Copy the most recently received command out of the (volatile) internal
 * buffer into the caller's buffer, then clear the "ready" flag so a new
 * command can be received. */
void bluetooth_read_command(char *buf)
{
    u8 i;
    for (i = 0; i < BT_BUF_SIZE; i++)
    {
        buf[i] = bt_buffer[i];
        if (buf[i] == '\0')
            break;
    }
    buf[BT_BUF_SIZE - 1] = '\0';   /* Guarantee null-termination even on overflow */
    bt_rx_ready = 0;
}

/* Discard whatever has been received so far without processing it - used
 * by the main loop's stale-input watchdog (see bt_watchdog_poll() in
 * projectmain.c) to recover from a half-typed command that never got its
 * '#' terminator, instead of letting it silently sit forever waiting to
 * be appended onto by the next send. */
void bluetooth_clear(void)
{
    bt_index = 0;
    bt_buffer[0] = '\0';
    bt_rx_ready = 0;
}

/* Non-blocking check: returns non-zero if characters have started arriving
 * for a new command (bt_index > 0) but the '#' terminator hasn't shown up
 * yet - i.e. there's a half-typed command sitting in the buffer. */
u8 bluetooth_pending(void)
{
    return (bt_index > 0) ? 1U : 0U;
}

