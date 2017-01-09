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

#ifndef PCCOMMS_H
#define PCCOMMS_H
 
#include "sowb.h"

#define PCCOMMS_STATE_WAITING           0
#define PCCOMMS_BASE_PACKET_READ        1
#define PCCOMMS_PACKET_READ             2
#define PCCOMMS_BASE_PACKET_BAD_CSUM    3

#define PCCOMMS_PACKET_ACCEPTED         1
#define PCCOMMS_PACKET_REJECTED         2

typedef struct _base_packet_bin {
    uint16_t    mode;
    uint16_t    length;
} BASE_PACKET_B;

#define BASE_PACKET_B_LEN   sizeof(BASE_PACKET_B)

typedef struct _base_packet_ascii {
    char        lead_char;
    char        mode[4];
    char        length[4];
    char        csum;
    char        flag;
    char        trail_char;
} BASE_PACKET_A;

#define BASE_PACKET_A_LEN   sizeof(BASE_PACKET_A)

#define BASE_PACKET_WRAP    if(header_packet_in>=BASE_PACKET_A_LEN)header_packet_in=0

void pccomms_init(void);
void pccomms_process(void);

void Uart3_init(void);
void Uart3_putc(char c);
int Uart3_getc(int block);

void base_packet_a2b(BASE_PACKET_B *b, BASE_PACKET_A *a);
void base_packet_b2a(BASE_PACKET_A *a, BASE_PACKET_B *b);

/* Buffer sizes MUST be a aligned 2^, for example 8, 16, 32, 64, 128, 256, 512, 1024, etc 
   Do NOT use any other value or the buffer wrapping firmware won't work. */ 
#define UART3_TX_BUFFER_SIZE   512
#define UART3_RX_BUFFER_SIZE   512


#endif
