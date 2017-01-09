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
#include "user.h"
#include "flash.h"
#include "ssp0.h"
#include "gpio.h"

char mac_addr[6];

/** _25AA02E48_mac_addr
 *
 * Get a copy of the MAC address with a null terminator.
 *
 * @param char *s a buffer, 7 bytes long, to hold the MAC+null
 */
void _25AA02E48_mac_addr(char *s) {
    memcpy(s, mac_addr, 6);
    s[6] = '\0';
}

/** _25AA02E48_mac_addr_printable
 *
 * Create a string that represents the MAC addr as a
 * printable ASCII string. The caller is responsible 
 * for allocating enough space in the buffer pointed
 * to by s to hold the string.
 *
 * @param char *s a buffer, 18 bytes long, to hold the MAC
 * @param char divider A character to divide the bytes or 0
 */
void _25AA02E48_mac_addr_printable(char *s, char divider) {
    if (divider != 0) {
        sprintf(s, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X", 
            mac_addr[0], divider, mac_addr[1], divider, mac_addr[2], divider, 
            mac_addr[3], divider, mac_addr[4], divider, mac_addr[5]);
    }
    else {
        sprintf(s, "%02X%02X%02X%02X%02X%02X", 
            mac_addr[0], mac_addr[1], mac_addr[2], 
            mac_addr[3], mac_addr[4], mac_addr[5]);    
    }
}

/** _25AA02E48_init
 */
void _25AA02E48_init(void) {

    /* Assumes SSP0 is already _init() */
    
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    LPC_SSP0->CPSR = _25AA02E48_SSP_INIT_CPSR;    
    AA02E48_CS_ASSERT;
    FLASH_SHORT_COMMAND(FLASH_READ);
    SSP0_WRITE_BYTE(0xFA);
    SSP0_FLUSH_RX_FIFO;
    
    for (int i = 0; i < 6; i++) {
        SSP0_WRITE_BYTE(0x00);
        while(SSP0_IS_BUSY || (LPC_SSP0->SR & (1UL << 2)) == 0);
        mac_addr[i] = (char)LPC_SSP0->DR;        
    }
    
    AA02E48_CS_DEASSERT;
    LPC_SSP0->CPSR = FLASH_SSP_INIT_CPSR;
    SSP0_release();
}
