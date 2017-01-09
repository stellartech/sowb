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
MODE 1 Packet handler.

This mode is used by the Host to send a file containing all the TLEs that
SOWB needs to aquire satellites. The base packet payload itself conatins
a sub packet as defined below. Note, we assign a number of flash pages to
the TLE file. Each flash page is 256bytes in size. So, the host sends the
file in 256byte blocks. The sub-packet header defines the block being sent.
We add to this block the flash base page and write the 256byte sub-packet
payload into the flash device. Note, TLE files are ascii characters so the
sub-packet payload is ASCII itself and doesn't require any conversions.

Subp-acket format   <BBBBCF>.........
Where:-
    < is the "start of packet header" character.
    BBBB is a hex string representation of the uint16_t 256 char block.
    C is a two's complement checksum of the header from < to > inclusive.
    F is a char flag. Normally zero.
    > is the "end of packet header" character.
    .... is the packet payload, 256 bytes in length.

*/

#include "sowb.h"
#include "user.h"
#include "utils.h"
#include "flash.h"
#include "pccomms.h"

#define FLASH_BASE_TLE_FILE 10

/* Error flags to send back on failure. */
#define FLAG_BAD_CHECKSUM   '1'
#define FLAG_INVALID_LENGTH '2'

/* Base packet payload to sub-packet header definition. */
typedef struct _pccomms_mode1_header {
    char        lead_char;
    char        block[4];
    char        csum;
    char        flag;
    char        trail_char;
} PCCOMMS_MODE1_HEADER;

/* Base packet payload to sub-packet overall definition. */
typedef struct _pccomms_mode1 {
    PCCOMMS_MODE1_HEADER    header;
    char                    payload[256];
} PCCOMMS_MODE1;


/** pccomms_mode1_failed
 *
 * Used to send back a failed packet with a flag "reason for failure".
 *
 * @param BASE_PACKET_A *a The original ASCII version of the packet header.
 * @param char flag The char flag to insert. 
 */
void pccomms_mode1_failed(BASE_PACKET_A *a, char flag) {
    BASE_PACKET_A temp;
    char *p;
    int i;
    
    memcpy((char *)&temp, (char *)a, sizeof(BASE_PACKET_A));
    temp.flag = flag;
    temp.csum = 0;
    temp.csum = strcsuml((char *)&temp, sizeof(BASE_PACKET_A));
    for (p = (char *)&temp, i = 0; i < sizeof(BASE_PACKET_A); i++) {
        Uart3_putc((char)p[i]);
    }
}

/** pccomms_mode1_handler
 *
 * The packet handler for MODE 1.
 *
 * @param BASE_PACKET_B *b
 * @param BASE_PACKET_A *a
 * @return int 
 */
int pccomms_mode1_handler(BASE_PACKET_B *b, BASE_PACKET_A *a) {
    PCCOMMS_MODE1 data;
    char *p;
    int page, c, i;
        
    /* Sanity check, should never fail, should really assert() */    
    if (b->mode != 1) return PCCOMMS_PACKET_REJECTED;
    
    /* Sanity check, should never fail, should really assert() */    
    if (b->length != sizeof(PCCOMMS_MODE1)) return PCCOMMS_PACKET_REJECTED;
     
    /* Copy the base packet payload into our internal data structure. */
    for(p = (char *)&data, i = 0; i < sizeof(PCCOMMS_MODE1); i++) {
        c = Uart3_getc(0);
        if (c != -1) p[i] = (char)c;
        else {
            /* This shouldn't happen as once the IRQ system has detected the
               end of the packet it no longer inserts serial chars into the 
               buffer. So another sanity check really. Unless the Host did
               in fact screw up somewhere. */
            pccomms_mode1_failed(a, FLAG_INVALID_LENGTH);
            return PCCOMMS_PACKET_REJECTED;
        }
    }

    /* Run a checksum check. On failure, send back the Host's packet with "flag" set to "bad checksum". */
    if (strsuml((char *)&data, sizeof(PCCOMMS_MODE1)) != 0) {
        pccomms_mode1_failed(a, FLAG_BAD_CHECKSUM);
        return PCCOMMS_PACKET_REJECTED;
    }
    
    /* Get the block header and add the flash page offset. */
    page = (int)hex2bin(data.header.block, 4) + FLASH_BASE_TLE_FILE;    
    
    /* Assuming we programmed the flash device ok, return packet accepted. */    
    flash_page_write(page, data.payload);
    while(flash_write_in_progress()) user_call_process();
    return PCCOMMS_PACKET_ACCEPTED;
}

