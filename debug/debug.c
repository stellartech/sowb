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
#include "debug.h"

/* Design note. We do not use a txBufferOverflow flag like we
   do with the RX buffer as all calls to Uart0_putc() will 
   block if there isn't room to send. It's up to the designer
   to either ensure they don't flood the TX buffer with too
   many debug strings or alternatively increase the buffer
   size to handle the higher amounts of traffic. */

volatile char txBuffer[UART0_TX_BUFFER_SIZE];
volatile char rxBuffer[UART0_RX_BUFFER_SIZE];
volatile unsigned char txBufferIn, txBufferOut;
volatile unsigned char rxBufferIn, rxBufferOut;
volatile bool txBufferFull, rxBufferFull, rxBufferOverflow;

/** UART0_IRQHandler
 */
extern "C" void UART0_IRQHandler(void) __irq {
    uint32_t iir;
    
    iir = LPC_UART0->IIR;
    
    if (iir & 1) return;
    
    iir = (iir >> 1) & 0x3;
    
    if (iir == 2) {
        if (rxBufferIn == rxBufferOut && rxBufferFull) {            
            char c __attribute__((unused)); /* oh dear, no room, send to /dev/null */
            c = LPC_UART0->RBR;
            rxBufferOverflow = true;
        }
        else {
            rxBuffer[rxBufferIn++] = LPC_UART0->RBR;
            rxBufferIn &= (UART0_RX_BUFFER_SIZE - 1);
            if (rxBufferIn == rxBufferOut) rxBufferFull = true;
        }
    }
    
    if (iir == 1) {
        if (txBufferIn != txBufferOut || txBufferFull) {
            LPC_UART0->THR = (uint8_t)(txBuffer[txBufferOut++]);
            txBufferOut &= (UART0_TX_BUFFER_SIZE - 1);
            txBufferFull = false;
        }
        else {
            LPC_UART0->IER = 0x1;
        }
    } 
}

/** Uart0_init
 */
void Uart0_init(void) {
    volatile char c __attribute__((unused));
    
    LPC_SC->PCONP       |=  (1UL << 3);
    LPC_SC->PCLKSEL0    &= ~(3UL << 6);
    LPC_SC->PCLKSEL0    |=  (1UL << 6);
    LPC_PINCON->PINSEL0 &= ~((1UL << 4) | (1UL << 6));
    LPC_PINCON->PINSEL0 |=  ((1UL << 4) | (1UL << 6));
    LPC_UART0->LCR       = 0x80;
    LPC_UART0->DLM       = 0x0;  // 0x00 for 115200 baud, for 9600 use 0x2;
    LPC_UART0->DLL       = 0x34; // 0x34 for 115200 baud, for 9600 use 0x71;
    LPC_UART0->LCR       = 0x3;
    LPC_UART0->FCR       = 0x7;
    
    NVIC_SetVector(UART0_IRQn, (uint32_t)UART0_IRQHandler);
    NVIC_EnableIRQ(UART0_IRQn);
    
    /* Enable UART0 RX interrupt only. */
    LPC_UART0->IER = 0x1;
}

/** debug_init
 */
void debug_init(void) {
    txBufferIn = txBufferOut = 0;
    memset((char *)txBuffer, 0, UART0_TX_BUFFER_SIZE);
    rxBufferIn = rxBufferOut = 0;
    memset((char *)txBuffer, 0, UART0_RX_BUFFER_SIZE);
    txBufferFull = rxBufferFull = false;
    rxBufferOverflow = false;
    Uart0_init();
}

#ifdef DEBUG_USE_UART0

/** debug_string
 *
 * Print a null termimnated string to UART0.
 *
 * @param char *s A pointer to the null terminated string. 
 */
void debug_string(char *s) {
    while (*(s)) {
        Uart0_putc(*s++);
    }
}

/** debug_stringl
 *
 * Print a string of specified length to UART0.
 *
 * @param char *s A pointer to the null terminated string. 
 * @param int length The length of the string to print.
 */
void debug_stringl(char *s, int length) {
    while (length--) {
        Uart0_putc(*s++);
    }
}

/* Local function prototype for _init(). */
void Uart0_init(void);

/** Uart0_putc
 *
 * Put a character out the UART0 serial port. 
 * Note, if the THR register is not empty AND the output buffer is not empty
 * then place the character into the output buffer and enable interrupt to
 * flush the buffer.
 *
 * Additionally, if the TX buffer is full this function will BLOCK!
 * If you have lots of debugging messages to spit out then it may be
 * wise to either "slimeline" what you need to print out or alternatively
 * increase the size of the txBuffer to cope with the increased traffic.
 * Be aware of this blocking!
 *
 * @param char c The character to send out of UART0.
 */
void Uart0_putc(char c) {
    if ((LPC_UART0->LSR & 0x20) && (txBufferIn == txBufferOut && !txBufferFull)) {
        LPC_UART0->THR = (uint8_t)c;
    }
    else {  
        while (txBufferFull) ; /* Blocks!!! */    
        txBuffer[txBufferIn++] = c;
        txBufferIn &= (UART0_TX_BUFFER_SIZE - 1);
        if (txBufferIn == txBufferOut && !txBufferFull) txBufferFull = true;
        LPC_UART0->IER = 0x3;
    }
}

/** Uart0_getc
 *
 * Used to get a character from Uart0. If the passed arg "block" is non-zero
 * then this function will block (wait) for user input. Otherwise if a char
 * is available return it, otherwise return -1 to show buffer was empty.
 *
 * @param int block Should we block?
 * @return int Cast char to int for char or -1 if non-blocking and no char.
 */
int Uart0_getc(int block) {
    char c;
    
    if (block) while (rxBufferOut == rxBufferIn && !rxBufferFull) ; /* Blocks! */    
    else if (rxBufferIn == rxBufferOut && !rxBufferFull) return -1;
    
    c = rxBuffer[rxBufferOut++];
    rxBufferOut &= (UART0_RX_BUFFER_SIZE - 1);
    if (rxBufferFull) rxBufferFull = false;     
    return (int)c;   
}

/* end of #ifdef DEBUG_USE_UART0 */
#endif



