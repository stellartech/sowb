/****************************************************************************
 *    Copyright 2010 Andy Kirkham, Stellar Technologies Ltd
 *    
 *    This file is part of the Satellite Observers Workbench (SOWB).
 *
 *    SOWB is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    SOWB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SOWB.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    $Id: main.cpp 5 2010-07-12 20:51:11Z ajk $
 *    
 ***************************************************************************/

/*
    Design notes. 
    The MAX7456 is connected to SSP1 on the Mbed. Additionally, the vertical sync
    is connected to Pxx, horizontal sync to Pxx, LOS to Pxx, RST to Pxx. These IO
    pins are setup and macros made available via gpio.c and the interrupts (vsync)
    via gpioirq.c so for information regarding those see those modules.
*/

#include "sowb.h"
#include "gpio.h"
#include "gpioirq.h"
#include "utils.h"
#include "user.h"
#include "MAX7456.h"
#include "MAX7456_chars.h"
#include "debug.h"

/* Forward local function prototypes. */
static void SSP1_init(void);

/* Declare the custom character map (CM) definitions.
   See MAX7456_chars.c for more details. */
extern MAX7456_CUSTOM_CHAR custom_chars[];

/* Map ASCII table to the MAX7456 character map.
   Note, the MAX7456 in-built character map is no where near the ascii
   table mapping and very few characters are abailable to map. Where 
   possible we create new characters for those we need that are missing
   from the MAX7456 that we want to use and also we create some special
   characters of our own that are not ASCII chars (crosshair for example).
   These additional character definitions are listed below the table. 
   Character maps we have create can be found in MAX7456_chars.c */
const unsigned char MAX7456_ascii[256] = {

    /* Regular ASCII table. */
    /*         00    01    02    03    04    05    06    07     08    09    0A    0B    0C    0D    0E    0F  */  
    /* 00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* 10 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* 20 */ 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x46,  0x3F, 0x40, 0x00, 0x4d, 0x45, 0x49, 0x41, 0x47, 
    /* 30 */ 0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  0x08, 0x09, 0x44, 0x43, 0x4A, 0x00, 0x4B, 0x42,
    /* 40 */ 0x4C, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,  0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 
    /* 50 */ 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0X1F, 0x20, 0x21,  0x22, 0x23, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* 60 */ 0x46, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,  0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33,
    /* 70 */ 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,  0x3C, 0x3D, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 
    
    /* Extended ASCII table. */
    /*         00    01    02    03    04    05    06    07     08    09    0A    0B    0C    0D    0E    0F  */  
    /* 80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* 90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* A0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* B0 */ 0xB0, 0x00, 0x00, 0xB3, 0xB4, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBF,   
    /* C0 */ 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* D0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0xD9, 0xDA, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* E0 */ 0xe0, 0xe1, 0xe2, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* F0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00     
};

/** MAX7456_map_char
 *
 * Convert the supplied ASCII character to a MAX7456 character.
 * Note, the MAX7456 does not support all ASCII chars. Those it
 * cannot convert are converted to spaces. Proceedure is a simple
 * table look-up.
 *
 * @See const unsigned char MAX7456_ascii
 *
 * @param unsigned char c The character to convert.
 * @return unsigned char The converted character.
 */
unsigned char MAX7456_map_char(unsigned char c) {
    return MAX7456_ascii[c];
}

/** MAX7456_init
 */
void MAX7456_init(void) {
    
    /* Setup SSP1 to interface to the MAX7456 chip. */
    SSP1_init();

    DEBUG_INIT_START;
    P21_ASSERT;
    
    /* Reset the MAX7456 device. */    
    MAX7456_RST_ASSERT;
    user_wait_ms_blocking(100);    
    MAX7456_RST_DEASSERT;
    user_wait_ms_blocking(100);

    /* Write the custom CM map. */
    MAX7456_write_byte(0, MAX7456_read_byte(0) & 0xF7);
    for (int index = 0; custom_chars[index].ascii != 0; index++) {
        MAX7456_write_char_map(custom_chars[index].ascii, custom_chars[index].map);
    }
    user_wait_ms_blocking(100);

    /* Change the vertical offset. */
    MAX7456_write_byte(0x3, 0x16);
    
    /* Enable display of OSD image. */
    MAX7456_write_byte(0x0, 0x48);
    
    P21_DEASSERT;
    
    DEBUG_INIT_END;
}

/** MAX7456_vsync_fall
 *
 * Interrupt handler for the vertical sync signal from the MAX7456 chip.
 * Note, this is called from gpioirq.c module which handles GPIO interrupts.
 */
void osd_vsync(void);
void MAX7456_vsync_fall(void) {
    osd_vsync();
}

/** MAX7456_vsync_rise
 *
 * Interrupt handler for the vertical sync signal from the MAX7456 chip.
 * Note, this is called from gpioirq.c module which handles GPIO interrupts.
 */
void MAX7456_vsync_rise(void) {
    // Does nothing.
}

/** MAX7456_write_byte
 *
 * Used to write a byte to a specific address.
 * This function also doubles up to allow a byte
 * to be written without an address (less than 0x80)
 * and this is used to make 16bit transfer from two
 * 8bit transfers or to construct continuous 8bit
 * transfer where the MAX7456 auto-increments an 
 * internal location pointer on each write operation.
 *
 * Note, this function effectivly "blocks". However, the
 * speed of the SSP serial bus is pretty high to say the
 * least and experimentation has show it, so far, to have
 * had no adverse effects.
 *
 * @param unsigned char address The register address to write to.
 * @param unsigned char byte The byte to write to teh register.
 */
void MAX7456_write_byte(unsigned char address, unsigned char byte) {
    volatile int dev_null __attribute__((unused));
    
    MAX7456_CS_ASSERT;
    
    /* MAX7456 addresses are always less than 0x80 so if the
       address is > 0x7F then the caller is requesting an direct
       8bit data transfer. */
    if (address < 0x80) {
        LPC_SSP1->DR = (uint32_t)(address & 0xFF);
        while(LPC_SSP1->SR & 0x10);     
        dev_null = LPC_SSP1->DR;            
    }
    
    LPC_SSP1->DR = (uint32_t)byte & 0xFF;
    while(LPC_SSP1->SR & 0x10); 
    dev_null = LPC_SSP1->DR;
            
    MAX7456_CS_DEASSERT;
}

/** MAX7456_read_byte
 *
 * Read a byte from a specific address.
 *
 * Note, this function effectivly "blocks". However, the
 * speed of the SSP serial bus is pretty high to say the
 * least and experimentation has show it, so far, to have
 * had no adverse effects.
 *
 * @param unsigned char address The address of the register to read.
 * @return int data The value of the register addressed.
 */
int MAX7456_read_byte(unsigned char address) {
    int data;

    MAX7456_CS_ASSERT;
    
    LPC_SSP1->DR = (uint32_t)address & 0xFF;
    while(LPC_SSP1->SR & 0x10);
    data = LPC_SSP1->DR; /* Discarded. */
    
    LPC_SSP1->DR = 0;
    while(LPC_SSP1->SR & 0x10);
    data = LPC_SSP1->DR & 0xFF;
    
    MAX7456_CS_DEASSERT;
    
    return data;
}

/** MAX7456_cursor
 *
 * Move the MAX7456 "cursor" (next display memory write)
 * to the specified position.
 *
 * @param int x The X position.
 * @param int y The Y position.
 */
void MAX7456_cursor(int x, int y) {
    int pos = (y * 30) + x;
    MAX7456_write_byte(0x05, (unsigned char)((pos >> 8) & 0xFF));
    MAX7456_write_byte(0x06, (unsigned char)(pos & 0xFF));
}

/** MAX7456_convert_string
 *
 * Convert the NULL terminated raw string to MAX7456 character set compat bytes.
 * Note, alters the string passed, doesn't make a copy of the string so cannot 
 * be a const value.
 *
 * @param unisgned char *s A pointer to the string to convert.
 */
void MAX7456_convert_string(unsigned char *s) {
    while(*(s)) {
        *(s) = MAX7456_ascii[*(s)];
        s++;
    }
}

/** MAX7456_string
 *
 * Send the NULL terminated ASCII string to the display memory.
 *
 * @param unisgned char *s A pointer to the ASCII string to write.
 */
void MAX7456_string(unsigned char *s) {
    MAX7456_write_byte(0x04, 0x01);  /* Enable 8bit write */
    while(*(s)) {
        MAX7456_write_byte(0x80, MAX7456_map_char(*s++));
    }
    MAX7456_write_byte(0x80, 0xFF);
}

/** MAX7456_stringl
 *
 * Send the ASCII string to the display memory. A null will terminate the write.
 *
 * @param int x The X position to write the string to.
 * @param int y The Y position to write the string to.
 * @param unisgned char *s A pointer to the string to write.
 * @param int len The length of the string to send.
 */
void MAX7456_stringl(int x, int y, unsigned char *s, int len) {
    MAX7456_cursor(x, y);            
    MAX7456_write_byte(0x04, 0x01);  /* Enable 8bit write */
    while(len--) {
        if (*s == '\0') break;
        MAX7456_write_byte(0x80, MAX7456_map_char(*s++));
    }
    MAX7456_write_byte(0x80, 0xFF);
}

/** MAX7456_read_char_map
 *
 * Reads the 54byte character make-up bytes and stores them into a buffer.
 *
 */
void MAX7456_read_char_map(unsigned char address, unsigned char *data54) {
    MAX7456_write_byte(0x9, address);
    MAX7456_write_byte(0x8, 0x50);
    user_wait_ms_blocking(100);
    for (int index = 0; index < 54; index++) {
        MAX7456_write_byte(0xA, index);
        user_wait_ms_blocking(1);
        *(data54 + index) = MAX7456_read_byte(0xC0);
    }
}

/** MAX7456_write_char_map
 *
 * Used to write the 54bytes that make up a character into
 * the MAX7456 CM non-volatile memory. Note, it tests the
 * current non-v memory against the supplied buffer and only
 * writes the buffer out if they don't match. Used to ensure
 * we don't keep writing the same data into non-v memory which
 * has a "lifetime" associated with it.
 *
 * @param unsigned char address The character address we are writing.
 * @param unsigned char *data54 An array that contains the 54bytes of CM data.
 */
void MAX7456_write_char_map(unsigned char address, const unsigned char *data54) {
    unsigned char index, c, match = 1;
    
    MAX7456_write_byte(0x9, address);
    MAX7456_write_byte(0x8, 0x50);
    user_wait_ms_blocking(20);
    for (index = 0; index < 54; index++) {
        MAX7456_write_byte(0xA, index);
        c = MAX7456_read_byte(0xC0);
        if (c != data54[index]) {
            match = 0;            
            break;
        }
    }
    
    if (!match) {   
        MAX7456_write_byte(0x9, address);
        for (index = 0; index < 0x36; index++) {
            MAX7456_write_byte(0x0A, index);
            MAX7456_write_byte(0x0B, data54[index]);
        }
        MAX7456_write_byte(0x08, 0xA0);
        user_wait_ms_blocking(20);
        while ((MAX7456_read_byte(0xA0) & 0x20) != 0x00);    
    }
}

/** SSP1_init
 */
static void SSP1_init(void) {

    DEBUG_INIT_START;
    
    /* The MAX7456 device is connected to SSP1 via the Mbed pins.
       So this init is about configuring just the SSP1, other
       MAX7456 signals (vsync, etc) are setup elsewhere although
       make call backs to this module as the "clearing house" for
       MAX7456 signals. */
    
    /* Enable the SSP1 peripheral. */
    LPC_SC->PCONP |= (1UL << 10);

    /* Select the clock required for SSP1. */    
    LPC_SC->PCLKSEL0  &= ~(3UL << 20);
    LPC_SC->PCLKSEL0  |=  (3UL << 20);
    
    /* Select the GPIO pins for the SSP1 functions. */
    /* SCK1 */
    LPC_PINCON->PINSEL0  &= ~(3UL << 14);
    LPC_PINCON->PINSEL0  |=  (2UL << 14);
    /* MISO1 */
    LPC_PINCON->PINSEL0  &= ~(3UL << 16);
    LPC_PINCON->PINSEL0  |=  (2UL << 16);
    /* MOSI1 */
    LPC_PINCON->PINSEL0  &= ~(3UL << 18);
    LPC_PINCON->PINSEL0  |=  (2UL << 18);
    
    /* Note, we don't use SSEL1 in our design, we just use a standard GPIO
       because a) the MAX7456 didn't really like the speed! and b) writing
       16bit data is somewhat simpler than reconfiguring the SSP each time
       when all we need to do is hold CS low across 2 8bit operations. */
    
    /* Setup the control registers for SSP1 */
    LPC_SSP1->CR0  = 0x7;
    LPC_SSP1->CPSR = 0x2;
    LPC_SSP1->CR1  = 0x2;
    
    DEBUG_INIT_END;
}

