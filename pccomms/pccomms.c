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
Packet format   <MMMMLLLLCF>.........
Where:-
    < is the "start of packet header" character.
    MMMM is a hex string representation of the uint16_t mode.
    LLLL is a hex string representation of the uint16_t length (payload length).
    C is a two's complement checksum of the header from < to > inclusive.
    F is a char flag. Normally zero.
    > is the "end of packet header" character.
    .... is the packet payload, a series of bytes, the number defined by length LLLL
    
Note, the C checksum should be added by the sender. This is the two's complement of
all the ascii chars from < to > inclusive. Once inserted by the sender, the header
packet can be checked by summing all the charachers, from < to > inclusive and the
result should be zero.
    
Once a complete header is received the IRQ is told how many bytes to read into the
RX buffer before changing the state to PCCOMMS_PACKET_READ. After this any future
characters received will be sent to dev/null until the current packet is handled.

The handling is done by the _process() function. Basically, once we have a packet
the MMMM mode decides what to do with it. The _process() function will call the list
of handler functions passing a pointer to the packet header. Each handler can
accept the header and then empty the payload RX buffer. Handlers can get the payload
by simply calling Uart1_getc(), which will eventually return -1 when the RX buffer
is empty (which should match the LLLL length of the payload.

If a handler returns PCCOMMS_PACKET_ACCEPTED then we clear out the header and reset
the serial system to begin listening for a new packet. Note, if a handler needs to
keep a copy of any data (header or payload) it should do so before it returns 
PCCOMMS_PACKET_ACCEPTED because the engine here will ensure all buffers get reset
before going back into reception mode. 

If no handler accepts the packet then the packet is dropped, the serial engine is
reset and basically everything is reset to begin looking for a new packet header.  */

#include "sowb.h"
#include "debug.h"
#include "pccomms.h"
#include "utils.h"

/* Globals used for the packet reception engine. */
int            pccomms_state;
BASE_PACKET_A  header_packet_ascii;
unsigned char  header_packet_in;
BASE_PACKET_B  header_packet;
uint16_t       packet_char_counter;

/* Payload buffers. */
volatile char uart3txBuffer[UART3_TX_BUFFER_SIZE];
volatile char uart3rxBuffer[UART3_RX_BUFFER_SIZE];
volatile unsigned char uart3txBufferIn, uart3txBufferOut;
volatile unsigned char uart3rxBufferIn, uart3rxBufferOut;
volatile bool uart3txBufferFull, uart3rxBufferFull, uart3rxBufferOverflow;

/* Used to reset the PC COMMS system. */
static void pccomms_reset(void) {
    pccomms_state = PCCOMMS_STATE_WAITING;
    header_packet_in = 0;
    uart3txBufferIn = uart3txBufferOut = 0;
    memset((char *)uart3txBuffer, 0, UART3_TX_BUFFER_SIZE);
    uart3rxBufferIn = uart3rxBufferOut = 0;
    memset((char *)uart3txBuffer, 0, UART3_RX_BUFFER_SIZE);
    uart3txBufferFull = uart3rxBufferFull = false;
    uart3rxBufferOverflow = false;
}

/** pccomms_init
 */
void pccomms_init(void) {
    DEBUG_INIT_START;
    pccomms_reset();
    Uart3_init();
    DEBUG_INIT_END;
}

/* We declare the packet handler functions prototypes here. */
int pccomms_mode1_handler(BASE_PACKET_B *b, BASE_PACKET_A *a);

/** pccomms_process
 */
void pccomms_process(void) {
    
    /* If the IRQ system has flagged a packet reception complete handle it. */
    if (pccomms_state == PCCOMMS_PACKET_READ) {
        switch(header_packet.mode) {
            case 1: 
                pccomms_mode1_handler(&header_packet, &header_packet_ascii); 
                break;
        }
        
        pccomms_reset();
    }
}

/** base_packet_a2b
 *
 * Convert a header packet in ASCII format to the internal
 * binary form.
 *
 * @param BASE_PACKET_B *b The dst of the conversion
 * @param BASE_PACKET_A *a The src of the conversion
 */
void base_packet_a2b(BASE_PACKET_B *b, BASE_PACKET_A *a) {
    b->mode = (uint16_t)hex2bin(a->mode, 4);
    b->length = (uint16_t)hex2bin(a->length, 4);
}

/** base_packet_b2a
 *
 * Convert an internal header packet in binary format to the external
 * ASCII form.
 *
 * @param BASE_PACKET_A *a The dst of the conversion
 * @param BASE_PACKET_B *b The src of the conversion
 */
void base_packet_b2a(BASE_PACKET_A *a, BASE_PACKET_B *b) {
   bin2hex((uint32_t)b->mode, 4, a->mode);
   bin2hex((uint32_t)b->length, 4, a->length);
   a->lead_char = '<';
   a->trail_char = '>';
   a->csum = '\0';
   a->csum = strcsuml((char *)a, BASE_PACKET_A_LEN);
}

/** Uart3_putc
 *
 * Put a character out the UART1 serial port. 
 * Note, if the THR register is not empty AND the output buffer is not empty
 * then place the character into the output buffer and enable interrupt to
 * flush the buffer.
 *
 * Additionally, if the TX buffer is full this function will BLOCK!
 * Be aware of this blocking!
 *
 * @param char c The character to send out of UART1.
 */
void Uart3_putc(char c) {
    if ((LPC_UART3->LSR & 0x20) && (uart3txBufferIn == uart3txBufferOut && !uart3txBufferFull)) {
        LPC_UART3->THR = (uint8_t)c;
    }
    else {  
        while (uart3txBufferFull) ; /* Blocks!!! */    
        uart3txBuffer[uart3txBufferIn++] = c;
        uart3txBufferIn &= (UART3_TX_BUFFER_SIZE - 1);
        if (uart3txBufferIn == uart3txBufferOut && !uart3txBufferFull) uart3txBufferFull = true;
        LPC_UART3->IER = 0x3;
    }
}

/** Uart3_getc
 *
 * Used to get a character from Uart1. If the passed arg "block" is non-zero
 * then this function will block (wait) for user input. Otherwise if a char
 * is available return it, otherwise return -1 to show buffer was empty.
 *
 * @param int block Should we block?
 * @return int Cast char to int for char or -1 if non-blocking and no char.
 */
int Uart3_getc(int block) {
    char c;
    
    if (block) while (uart3rxBufferOut == uart3rxBufferIn && !uart3rxBufferFull) ; /* Blocks! */    
    else if (uart3rxBufferIn == uart3rxBufferOut && !uart3rxBufferFull) return -1;
    
    c = uart3rxBuffer[uart3rxBufferOut++];
    uart3rxBufferOut &= (UART3_RX_BUFFER_SIZE - 1);
    if (uart3rxBufferFull) uart3rxBufferFull = false;     
    return (int)c;   
}

/** UART3_IRQHandler
 */
extern "C" void UART3_IRQHandler(void) __irq {
    uint32_t iir;
    char c, *p;
    
    iir = LPC_UART3->IIR;
    
    if (iir & 1) return;
    
    iir = (iir >> 1) & 0x3;
    
    if (iir == 2) {
        c = (char)LPC_UART3->RBR;
        if (pccomms_state == PCCOMMS_STATE_WAITING) {
            p = (char *)&header_packet_ascii;
            if (c == '<') {
                header_packet_in = 0;
                p[header_packet_in++] = c;
                BASE_PACKET_WRAP;
            }
            else if (c == '>') {
                p[header_packet_in++] = c;
                BASE_PACKET_WRAP;
                if (strsuml(p, BASE_PACKET_A_LEN) == 0) { 
                    base_packet_a2b(&header_packet, &header_packet_ascii);
                    packet_char_counter = header_packet.length;
                    pccomms_state = PCCOMMS_BASE_PACKET_READ;
                }
                else {
                    pccomms_state = PCCOMMS_BASE_PACKET_BAD_CSUM;
                    header_packet_in = 0;
                }
            }
            else {
                p[header_packet_in++] = c;
                BASE_PACKET_WRAP;
            }
        }
        else if (pccomms_state == PCCOMMS_BASE_PACKET_READ) {
            if (uart3rxBufferIn == uart3rxBufferOut && uart3rxBufferFull) {            
                uart3rxBufferOverflow = true;
            }
            else {
                uart3rxBuffer[uart3rxBufferIn++] = c;
                uart3rxBufferIn &= (UART3_RX_BUFFER_SIZE - 1);
                if (uart3rxBufferIn == uart3rxBufferOut) uart3rxBufferFull = true;
                if (packet_char_counter) packet_char_counter--;
                if (packet_char_counter == 0) {
                    pccomms_state = PCCOMMS_PACKET_READ;
                }
            }
            
        }
        else {
            /* Unknown state, send char to /dev/null */          
        }
    }
    
    if (iir == 1) {
        if (uart3txBufferIn != uart3txBufferOut || uart3txBufferFull) {
            LPC_UART3->THR = (uint8_t)(uart3txBuffer[uart3txBufferOut++]);
            uart3txBufferOut &= (UART3_TX_BUFFER_SIZE - 1);
            uart3txBufferFull = false;
        }
        else {
            LPC_UART3->IER = 0x1;
        }
    } 
}

/** Uart3_init
 */
void Uart3_init(void) {
    
    LPC_SC->PCONP       |=  (1UL << 25);
    LPC_SC->PCLKSEL1    &= ~(3UL << 18);
    LPC_SC->PCLKSEL1    |=  (1UL << 18);
    LPC_PINCON->PINSEL0 &= ~((2UL << 0) | (2UL << 2));
    LPC_PINCON->PINSEL0 |=  ((2UL << 0) | (2UL << 2));
    LPC_UART1->LCR       = 0x80;
    LPC_UART1->DLM       = 0x0;  // 0x00 for 115200 baud, for 9600 use 0x2;
    LPC_UART1->DLL       = 0x34; // 0x34 for 115200 baud, for 9600 use 0x71;
    LPC_UART1->LCR       = 0x3;
    LPC_UART1->FCR       = 0x7;
    
    NVIC_SetVector(UART3_IRQn, (uint32_t)UART3_IRQHandler);
    NVIC_EnableIRQ(UART3_IRQn);
    
    /* Enable UART1 RX and TX interrupt. */
    LPC_UART3->IER = 0x3;
}
