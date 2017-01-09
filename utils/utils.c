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


#include "sowb.h"
#include "ctype.h"
#include "utils.h"
#include "gps.h"
#include "debug.h"

/** ascii2bin
 *
 * Converts an ascii char to binary nibble.
 *
 * @param char The character to convert [0-9][a-f][A-F]
 * @return char the bin value or -1 on invalid hex char.
 */
char ascii2bin(char c) {
    if (c >= '0' && c <= '9') return (c - '0') & 0xF;
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10) & 0xF;
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10) & 0xF;
    return (char)0xFF;
}

/** hex2bin
 *
 * Converts a hex ascii string to binary int.
 *
 * Note, no error checking, assume string is valid hex chars [0-9][a-f][A-F]
 *
 * @param char *s The string to convert.
 * @param int len The length of the string to convert.
 * @return uint32_t the converted value.
 */
uint32_t hex2bin(char *s, int len) {
    int i;
    uint32_t rval;
    
    for (rval = 0, i = 0; i < len; i++) rval = rval | (ascii2bin(*(s + i)) << ((len - i - 1) * 4));
    
    return rval;
}

/** bin2ascii
 *
 * Convert a nibble to an ASCII character
 *
 * @param char c The nibble to convert
 * @return char The character representation of the nibble.
 */
char bin2ascii(char c) {
    c &= 0xF;
    if (c < 0xA) return c + '0';
    return c + 'A' - 10;
}

/** bin2hex
 *
 * Convert a binary to a hex string representation.
 * The caller should allocate a buffer for *s before
 * calling this function. The allocation should be
 * len + 1 in length to hold the string and the 
 * terminating null character.
 *
 * @param uint32_t d The value to convert.
 * @param int len The string length.
 * @param char *s Where to put the string.
 * @return char * Returns *s passed in.
 */
 //  238E,238E
 //  O832,O832

char * bin2hex(uint32_t d, int len, char *s) {
    char c, i = 0;
    *(s + len) = '\0';
    while (len) {
        c = (d >> (4 * (len - 1))) & 0xF;
        *(s + i) = bin2ascii(c);
        len--; i++;        
    }
    return s;
}

/** dec2bin
 *
 * Converts a decimal ascii string to binary int.
 *
 * Note, no error checking, assume string is valid hex chars [0-9]
 *
 * @param char *s The string to convert.
 * @param int len The length of the string to convert.
 * @return uint32_t the converted value.
 */
uint32_t dec2bin(char *s, int len) {
    int i, mul;
    uint32_t rval = 0;
    
    for (mul = 1, i = len; i; i--, mul *= 10) rval += (ascii2bin(*(s + i - 1)) * mul);
    
    return rval;
}

/** strcsuml
 *
 * Return a two's compliment checksum char for the supplied string.
 *
 * @param char * s The string to sum
 * @param int len The length of the string.
 * @return The two's compliment char.
 */
char strcsuml(char *s, int len) {
    char sum = 0;
    while (len) {
        sum += *(s +len - 1);
    }
    return (~sum) + 1;
}

/** strcsum
 *
 * Return a two's compliment checksum char for the null terminated supplied string.
 *
 * @param char * s The string to sum
 * @return The two's compliment char.
 */
char strcsum(char *s) {
    return strcsuml(s, strlen(s));
}

/** strsuml
 *
 * Return the 8bit sum char for the supplied string.
 *
 * @param char * s The string to sum
 * @param int len The length of the string.
 * @return The sum
 */
char strsuml(char *s, int len) {
    char sum = 0;
    while (len) {
        sum += *(s +len - 1);
    }
    return sum;
}

/** strsum
 *
 * Return the 8bit sum of all the characters of the supplied string.
 *
 * @param char * s The string to sum
 * @return The sum
 */
char strsum(char *s) {
    return strsuml(s, strlen(s));
}

/* Used for the date_AsString function. */
const char month_abv[][4] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","Wtf" };

/** date_AsString
 *
 * Used to get the current date and return a formatted string.
 * Note, the caller must have a 12byte buffer in place, pointed
 * to by s to accept the string. 
 *
 * @param GPS_TIME *t A pointer to the time data struct to "print as"
 * @param char *s A pointer to a buffer to hold the formatted string.
 */
void date_AsString(GPS_TIME *t, char *s) {
    int month = t->month - 1; if (month > 11 || month < 0) month = 12; /* Ensure in range. */
    sprintf(s, "%4.4d/%s/%2.2d", t->year, month_abv[month], t->day);
}

/** time_AsString
 *
 * Used to get the current time and return a formatted string.
 * Note, the caller must have a 12byte buffer in place, pointed
 * to by s to accept the string. 
 *
 * @param GPS_TIME *t A pointer to the time data struct to "print as"
 * @param char *s A pointer to a buffer to hold the formatted string.
 */
void time_AsString(GPS_TIME *t, char *s) {    
    sprintf(s, "%2.2d:%2.2d:%2.2d.%1.1d%1.1d", t->hour, t->minute, t->second, t->tenth, t->hundreth);
}

/** double2dms
 *
 * Takes double and converts it to a printable display string of
 * degrees, minutes and seconds. 
 * The caller is responsible for allocating the buffer *s before
 * calling this function.
 *
 * @param char *s A pointer to the buffer to print to.
 * @param double d The value to print.
 */
void double2dms(char *s, double d) {
    int degrees, minutes;
    double seconds, t;
    
    degrees = (int)d; t = (d - (double)degrees) * 60.;
    minutes = (int)t;
    seconds = (t - (double)minutes) * 60.;
    
    sprintf(s, "%03d\xb0%02d\x27%02d\x22", degrees, minutes, (int)seconds);
}

/** printDouble
 *
 * Print a double to a string buffer with correct leading zero(s).
 * The caller is responsible for allocating the buffer *s before
 * calling this function.
 *
 * @param char *s A pointer to the buffer to print to.
 * @param double d The value to print.
 */
void printDouble(char *s, double d) {
    if (isnan(d))       sprintf(s, "---.----");
    else if (d > 100.)  sprintf(s, "%.4f", d);
    else if (d > 10.)   sprintf(s, "0%.4f", d);
    else                sprintf(s, "00%.4f", d);
}

/** printDouble_3_1
 *
 * Print a double to a string buffer with correct leading zero(s).
 * The caller is responsible for allocating the buffer *s before
 * calling this function.
 *
 * @param char *s A pointer to the buffer to print to.
 * @param double d The value to print.
 */
char * printDouble_3_1(char *s, double d) {
    char temp[16];
    if (isnan(d))       sprintf(temp, "---.-");
    else if (d > 100.)  sprintf(temp, "%.6f", d);
    else if (d > 10.)   sprintf(temp, "0%.6f", d);
    else                sprintf(temp, "00%.6f", d);
    memcpy(s, temp, 5);
    *(s+5) = '\0';
    return s;
}

/** printDouble_3_2
 *
 * Print a double to a string buffer with correct leading zero(s).
 * The caller is responsible for allocating the buffer *s before
 * calling this function.
 *
 * @param char *s A pointer to the buffer to print to.
 * @param double d The value to print.
 */
char * printDouble_3_2(char *s, double d) {
    char temp[16];
    if (isnan(d))       sprintf(temp, "---.--");
    else if (d > 100.)  sprintf(temp, "%.6f", d);
    else if (d > 10.)   sprintf(temp, "0%.6f", d);
    else                sprintf(temp, "00%.6f", d);
    memcpy(s, temp, 6);
    *(s+6) = '\0';
    return s;
}

void printBuffer(char *s, int len) {
    #ifdef DEBUG_ON
    for (int i = 0; i < len / 0x10; i++) {
        debug_printf("%02X: ", i);
        for (int j = 0; j < 0x10; j++) {
            debug_printf("%02X ", s[(i * 0x10) + j]);
            if (j == 7) debug_printf(" ");
        }
        for (int j = 0; j < 0x10; j++) {
            if (isprint(s[(i * 0x10) + j])) {
                debug_printf("%c", s[(i * 0x10) + j]);
            }
            else {
                debug_printf(".");
            }
            if (j == 7) debug_printf(" ");
        }
        debug_printf("\r\n");
    }
    #endif
}

inline void disable_irqs(void) {
    NVIC_DisableIRQ(EINT3_IRQn);
    NVIC_DisableIRQ(RIT_IRQn);
    NVIC_DisableIRQ(UART0_IRQn);
    NVIC_DisableIRQ(UART1_IRQn);
    NVIC_DisableIRQ(UART2_IRQn);
    NVIC_DisableIRQ(USB_IRQn);
}

inline void enable_irqs(void) {
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_EnableIRQ(EINT3_IRQn);
    NVIC_EnableIRQ(RIT_IRQn);
    NVIC_EnableIRQ(UART0_IRQn);
    NVIC_EnableIRQ(UART1_IRQn);
    NVIC_EnableIRQ(UART2_IRQn);
}
