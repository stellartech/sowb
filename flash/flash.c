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
#include "ssp0.h"
#include "gpio.h"
#include "flash.h"
#include "user.h"

extern bool sector_erase_in_progress;
extern bool page_write_in_progress;

char flash_buffer[2][FLASH_PAGE_SIZE];
int current_buffer;
unsigned int flash_address;

/** flash_init
 */
void flash_init(void) {
    
    /* Initialise the SSP0 to talk to the flash device. */    
    SSP0_init();
    
    DEBUG_INIT_START;
    
    /* Clear out the page buffers. */
    memset(flash_buffer, 0, 2 * FLASH_PAGE_SIZE);
    
    /* Default the buffer in use. */
    current_buffer = 0;
    
    /* Default pointer set-up. */
    flash_address = 0;
    
    /* Prime our buffers for expected future access. */
    flash_read_page(0, flash_buffer[0], true);
    flash_read_page(1, flash_buffer[1], true);
    
    /* Although not part of the flash system, SOWB includes
       a 25AA02E48 device from Microchip that holds a globally
       unique 48bit (6byte) MAC address. We use this for the
       SOWB "STL authentic product" serial number and in future
       may be used as the ethernet MAC address if we ever write 
       code to support Ethernet. This device is also connected
       to SSP1 and so we'll _init() it here now. */
    //_25AA02E48_init();
    
    DEBUG_INIT_END;
}

/** flash_process
 */
void flash_process(void) {
    /* Currently does nothing. */
}

/** flash_getc
 */
char flash_getc(bool peek) {
    char c;
    
    /* Flash being deleted, undefined memory. */
    if (sector_erase_in_progress) return 0xFF;
    
    /* Wait for any page loads to complete. */
    while (page_write_in_progress) user_call_process();
    
    /* Get the character from the internal buffer. */
    c = flash_buffer[current_buffer][flash_address];
    
    /* If this is just a peek then return the character without
       incrementing the memory pointers etc etc. */
    if (peek) return c;
    
    /* Inc the address pointer and load a new page if needed. Note,
       we load the page in background using DMA. */
    flash_address++;
    if ((flash_address & 0xFF) == 0) {
        flash_read_page(current_buffer >> 8, flash_buffer[current_buffer], false);
        current_buffer = current_buffer ? 0 : 1;        
    }
    
    return c;
}

/** flash_seek
 */
void flash_seek(unsigned int addr) {
    flash_read_page((addr >> 8) + 0, flash_buffer[0], true);
    flash_read_page((addr >> 8) + 1, flash_buffer[1], true);
    flash_address = addr;
}
