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
 
#ifndef DEBUG_H
#define DEBUG_H

/* Comment out the following to totally disable debugging output on UART0. 
   This is the global kill switch for debug output. */
#define DEBUG_ON


/* These are finer grained debug enable switches. */
#ifdef DEBUG_ON
#define DEBUG_USE_UART0
#endif

/* The following is used "interally" by the debug.c and various other modules. */

void debug_init(void);

/* Buffer sizes MUST be a aligned 2^, for example 8, 16, 32, 64, 128, 256, 512, 1024, etc 
   Do NOT use any other value or the buffer wrapping firmware won't work. */ 
#ifdef DEBUG_USE_UART0 
#define UART0_TX_BUFFER_SIZE   8192
#define UART0_RX_BUFFER_SIZE   16
#else
#define UART0_TX_BUFFER_SIZE   4
#define UART0_RX_BUFFER_SIZE   4
#endif


#ifdef DEBUG_USE_UART0 
/* If debugging is on declare the real function prototypes. */
int  debug_printf(const char *format, ...);
int  debug_sprintf(char *out, const char *format, ...);
void debug_string(char *s);
void debug_stringl(char *s, int length);

#else

/* If no debugging, replace debug functions with simple empty macros. */
#define debug_printf(x, ...)
#define debug_sprintf(x, y, ...)
#define debug_string(x)
#define debug_stringl(x, y)

/* End #ifdef DEBUG_USE_UART0 */
#endif

#define DEBUG_INIT_START debug_printf("INIT: %s()... ", __FUNCTION__)
#define DEBUG_INIT_END   debug_printf("complete.\r\n")

/* These function prototypes are always declared. */
void Uart0_putc(char c);
int  Uart0_getc(int block);


/* End #ifndef DEBUG_H */ 
#endif

