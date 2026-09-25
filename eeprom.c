/*=============================================================================
 * File        : eeprom.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Byte-level read/write driver for an AT24C256 serial EEPROM
 *               (32 KB, 2-byte internal addressing) connected over I2C0.
 *               Used to store the Level-1/Level-2 passwords persistently
 *               so they survive a power cycle.
 *
 * Wiring:
 *   SCL0 -> P0.2   (I2C0 clock)
 *   SDA0 -> P0.3   (I2C0 data)
 *   EEPROM A0/A1/A2 -> GND (address pins tied low, giving device ID 0xA0)
 *   EEPROM VCC -> 3V3, GND -> GND, WP -> GND (write protect disabled)
 *
 * EEPROM_ID (0xA0) is the AT24Cxx 7-bit I2C slave address (1010 000)
 * already shifted left by one bit, with bit 0 used for read(1)/write(0).
 *===========================================================================*/
#include <lpc214x.h>
#include "eeprom.h"
#include "delay.h"

#define EEPROM_ID   0xA0   /* AT24C256 I2C address, all address pins = 0 */

/* Configure P0.2/P0.3 for their I2C0 alternate function and set up the
 * I2C0 peripheral (clock rate, control flags) for master mode. */
void i2c_init(void)
{
    PINSEL0 &= ~((3UL << 4) | (3UL << 6));  /* Clear P0.2/P0.3 function bits */
    PINSEL0 |=  ((1UL << 4) | (1UL << 6));  /* Select SCL0 (P0.2) / SDA0 (P0.3) */

    I2C0CONCLR = 0x6C;   /* Clear STA, STO, SI and I2EN before reconfiguring */
    I2C0SCLH = 75;        /* High-period duty cycle count for SCL             */
    I2C0SCLL = 75;        /* Low-period duty cycle count for SCL (~100 kHz with PCLK=15MHz) */
    I2C0CONSET = 0x40;   /* Enable the I2C0 interface (I2EN)                  */
}

/* Issue an I2C START condition and wait for it to be signalled complete. */
static void i2c_start(void)
{
    I2C0CONSET = 0x20;   /* Set STA (request START)     */
    I2C0CONCLR = 0x08;   /* Clear SI (interrupt flag) before waiting */
    while (!(I2C0CONSET & 0x08));   /* Wait for SI to be set (START sent)   */
}

/* Issue an I2C STOP condition, releasing the bus. */
static void i2c_stop(void)
{
    I2C0CONSET = 0x10;   /* Set STO (request STOP) */
    I2C0CONCLR = 0x08;   /* Clear SI */
    delay_us(20);         /* Small guard time before the bus can be reused */
}

/* Write one byte onto the I2C bus (address byte or data byte) and wait
 * for the transfer to complete. */
static void i2c_write(u8 data)
{
    I2C0DAT = data;
    I2C0CONCLR = 0x28;   /* Clear STA and SI                     */
    while (!(I2C0CONSET & 0x08));   /* Wait for SI (byte transferred) */
}

/* Read one byte from the I2C bus.
 * ack = 1 -> send ACK  (more bytes to follow)
 * ack = 0 -> send NACK (this is the last byte being read) */
static u8 i2c_read(u8 ack)
{
    if (ack) I2C0CONSET = 0x04;    /* AA = 1 -> ACK after this byte  */
    else     I2C0CONCLR = 0x04;    /* AA = 0 -> NACK after this byte */

    I2C0CONCLR = 0x08;              /* Clear SI                        */
    while (!(I2C0CONSET & 0x08));  /* Wait for the byte to arrive      */
    return I2C0DAT;
}

/* Write a single byte to a given 16-bit EEPROM address.
 * Sequence: START, device address (write), address high byte,
 * address low byte, data byte, STOP. A 10 ms delay follows to allow the
 * EEPROM's internal write cycle to complete before any further access. */
void eeprom_byte_write(u16 addr, u8 data)
{
    i2c_start();
    i2c_write(EEPROM_ID);          /* Device address + write bit (0) */
    i2c_write((addr >> 8) & 0xFF); /* EEPROM address high byte        */
    i2c_write(addr & 0xFF);        /* EEPROM address low byte         */
    i2c_write(data);               /* The byte to store                */
    i2c_stop();
    delay_ms(10);                  /* Wait for the internal write cycle */
}

/* Read a single byte from a given 16-bit EEPROM address.
 * Sequence: START, device address (write), address high/low bytes,
 * repeated-START, device address (read), read one byte with NACK, STOP. */
u8 eeprom_byte_read(u16 addr)
{
    u8 data;

    i2c_start();
    i2c_write(EEPROM_ID);           /* Device address + write bit, to set the address pointer */
    i2c_write((addr >> 8) & 0xFF);
    i2c_write(addr & 0xFF);

    i2c_start();                    /* Repeated START to switch to read mode */
    i2c_write(EEPROM_ID | 0x01);    /* Device address + read bit (1)          */
    data = i2c_read(0);             /* Read the byte, NACK (last/only byte)   */
    i2c_stop();

    return data;
}

/* Write 'len' consecutive bytes starting at 'addr' (simple byte-at-a-time
 * loop; sufficient for the short 4-digit passwords used in this project). */
void eeprom_write_str(u16 addr, const char *str, u8 len)
{
    u8 i;
    for (i = 0; i < len; i++)
        eeprom_byte_write(addr + i, str[i]);
}

/* Read 'len' consecutive bytes starting at 'addr' into buf, then
 * null-terminate the result so it can be used as a C string. */
void eeprom_read_str(u16 addr, char *buf, u8 len)
{
    u8 i;
    for (i = 0; i < len; i++)
        buf[i] = eeprom_byte_read(addr + i);
    buf[len] = '\0';
}

