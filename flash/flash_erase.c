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
#include "flash.h"
#include "ssp0.h"
#include "dma.h"
#include "gpio.h"
#include "rit.h"
#include "user.h"
#include "utils.h"
#include "debug.h"

/* Flags to show what state we are in. */
bool sector_erase_in_progress = false;
extern bool page_write_in_progress;

bool flash_sector_erase_in_progress(void) {
    return sector_erase_in_progress;
}

/** flash_erase_sector
 */
int flash_erase_sector(int sector) {

    /* If a sector erase is in progress already
       we return zero rather than wait (block)
       because an erase can take so long to complete
       we don't want to hang around waiting. Let the
       caller reschedule it sometime later. */
    if (sector_erase_in_progress) {
        return 0;
    }

    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    sector_erase_in_progress = true;
    
    FLASH_CS_ASSERT;
    FLASH_SHORT_COMMAND(FLASH_WREN);
    FLASH_CS_DEASSERT;
    
    SSP0_FLUSH_RX_FIFO;
    
    /* Wait until the flash device has the WEL bit on. */
    while ((LPC_SSP0->DR & 0x2) == 0) {            
        FLASH_CS_ASSERT;
        FLASH_SHORT_COMMAND(FLASH_RDSR);
        SSP0_FLUSH_RX_FIFO;
        SSP0_WRITE_BYTE(0);
        while (SSP0_IS_BUSY);
        FLASH_CS_DEASSERT;
    }

    SSP0_FLUSH_RX_FIFO;
    
    FLASH_CS_ASSERT;
    FLASH_LONG_COMMAND(FLASH_SE, sector);
    FLASH_CS_DEASSERT;
    
    /* Note, a sector erase takes much longer than
       a page write (typical 600ms by the datasheet)
       so there's no point making the first timeout
       very short and producing a lot of uneeded 
       interrupts. So we set the first timeout to
       be 600 and then it'll switch to a much shorter
       time in the ISR. */
    rit_timer_set_counter(FLASH_WRITE_CB, 600);
    return 1;
}

/** flash_erase_bulk
 */
int flash_erase_bulk(void) {

    /* If a sector erase is in progress already
       we return zero rather than wait (block)
       because an erase can take so long to complete
       we don't want to hang around waiting. Let the
       caller reschedule it sometime later. */
   
    if (sector_erase_in_progress || page_write_in_progress) {
        return 0;
    }

    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    sector_erase_in_progress = true;
    
    FLASH_CS_ASSERT;
    FLASH_SHORT_COMMAND(FLASH_WREN);
    FLASH_CS_DEASSERT;
    
    SSP0_FLUSH_RX_FIFO;
    
    /* Wait until the flash device has the WEL bit on. */
    while ((LPC_SSP0->DR & 0x2) == 0) {            
        FLASH_CS_ASSERT;
        FLASH_SHORT_COMMAND(FLASH_RDSR);
        SSP0_FLUSH_RX_FIFO;
        SSP0_WRITE_BYTE(0);
        while (SSP0_IS_BUSY);
        FLASH_CS_DEASSERT;
    }

    FLASH_CS_ASSERT;
    FLASH_SHORT_COMMAND(FLASH_BE);
    FLASH_CS_DEASSERT;
    
    /* Note, a bulk erase takes much longer than
       a page write (typical 8s by the datasheet)
       so there's no point making the first timeout
       very short and producing a lot of uneeded 
       interrupts. So we set the first timeout to
       be 8000 and then it'll switch to a much shorter
       time in the ISR. */
    rit_timer_set_counter(FLASH_WRITE_CB, 8000);
    return 1;
}

