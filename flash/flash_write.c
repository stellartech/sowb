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
bool page_write_in_progress = false;
bool page_write_buffer_in_use = false;

/* Flag used by the flash_erase.c file. */
extern bool sector_erase_in_progress;

/* Buffer used to hold a copy of the page to write. Used
   to ensure the DMA has a valid buffer to copy. */
char flash_page_write_buffer[FLASH_PAGE_SIZE];

/** flash_write_in_progress
 */
bool flash_write_in_progress(void) {
    return page_write_in_progress;
}

/** flash_page_write
 */
int flash_page_write(int page, char *buffer) {
    
    /* Wait for the write page buffer to be released by
       the DMA ISR handler, if in use, then make a copy
       of the source buffer for the DMA. */
    while (page_write_buffer_in_use) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    memcpy(flash_page_write_buffer, buffer, FLASH_PAGE_SIZE);
    page_write_buffer_in_use = true;

    
    /* Below this we check for conditions that should stall 
       us before continuing. However, sector erase is different,
       it can take quite some time to complete. If this is the
       case, rather than block (wait), we'll return zero (not
       done) and allow the caller to schedule a write later. */
    if (sector_erase_in_progress) return 0;
    
    /* Do not start a page write while another page write
       is in progress. This flag is released by the RIT
       timer callback when the WIP flag shows a previous
       write has completed. */    
    while(page_write_in_progress) WHILE_WAITING_DO_PROCESS_FUNCTIONS;

    /* Do not start a page write while a page read is in operation. */
    while (flash_read_in_progress()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;

    /* Request DMA channel0. */
    while(!DMA_request_channel(0)) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    /* Request the use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    /* Flag a write is in progress. */    
    page_write_in_progress = true;
    
    /* Switch on WIP/WEL. */
    FLASH_CS_ASSERT;       
    FLASH_SHORT_COMMAND(FLASH_WREN);        
    FLASH_CS_DEASSERT;

    /* Originally I dropped into a do { ... } while(); here but
       found the time between the CS deassert and reassert inside
       the loop was *very* short and the flash device wasn't to
       keen on this. So, I switched to a "flush rx fifo" and the
       use a while() { ... } loop, just puts some delay between
       the CS manipulation. */
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
    FLASH_LONG_COMMAND(FLASH_PP, page); 

    LPC_GPDMA->DMACIntTCClear = 0x1;
    LPC_GPDMA->DMACSoftSReq   = 0xC;
        
    /* Prep Channel0 to send the buffer to the flash device. */
    LPC_GPDMACH0->DMACCSrcAddr  = (uint32_t)flash_page_write_buffer;
    LPC_GPDMACH0->DMACCDestAddr = (uint32_t)&LPC_SSP0->DR;
    LPC_GPDMACH0->DMACCLLI      = 0;
    LPC_GPDMACH0->DMACCControl  = DMA_CHANNEL_TCIE | DMA_CHANNEL_SRC_INC | (uint32_t)FLASH_PAGE_SIZE;
    
    /* Enable SSP0 DMA. */
    LPC_SSP0->DMACR = 0x2;

    /* Enable Channel0 */
    LPC_GPDMACH0->DMACCConfig = DMA_CHANNEL_ENABLE | 
                                DMA_CHANNEL_DST_PERIPHERAL_SSP0_TX | 
                                DMA_TRANSFER_TYPE_M2P |
                                DMA_MASK_IE |
                                DMA_MASK_ITC;
    
                                
    /* SSP0 CS line and "page_write_in_progress" flag are now 
       under DMA/SSP0 interrupt control. See ISR handlers for 
       more information. */
       
    return 1;
}

/** _flash_write_timer_callback
 *
 * RIT timer callback.
 *
 * After the write operation is complete this callback
 * is used to examine the WIP flag in the flash status
 * register. If it's still set then we reset the timer
 * and try again in the future. If it's clear then we
 * can mark the process as complete.
 *
 * @param int index The index of the timer the RIT modulr used.
 */
void _flash_write_timer_callback(int index) {
    uint32_t sr = 1;
    
    /* Read the WIP flag from the flash device status
       register and if the write cycle is complete mark
       the operation as complete. Otherwise, reset the
       timer to test again in the future. */
    FLASH_CS_ASSERT;
    FLASH_SHORT_COMMAND(FLASH_RDSR);
    SSP0_WRITE_BYTE(0);
    while (SSP0_IS_BUSY);
    FLASH_CS_DEASSERT;
    while(LPC_SSP0->SR & (1UL << 2)) {
        /* This loop ensures we read the last byte in the
           RX FIFO and test that. */
        sr = LPC_SSP0->DR;
    }
    if (sr & 0x1) {
        if (sector_erase_in_progress) rit_timer_set_counter(index, 100);
        else rit_timer_set_counter(index, FLASH_WIP_TEST_TIME);
    }
    else {
        FLASH_CS_ASSERT;
        FLASH_SHORT_COMMAND(FLASH_WRDI);
        SSP0_FLUSH_RX_FIFO;
        FLASH_CS_DEASSERT;
        if (sector_erase_in_progress) sector_erase_in_progress = false;
        if (page_write_in_progress)   page_write_in_progress = false;
        SSP0_release();
    }
}

/** flash_write_dma0_irq
 *
 * DMA transfer irq callback. 
 */
int flash_write_dma0_irq(int channel) {
    int rval = 0;
    
    /* If we were using DMA to transfer our buffer to the flash
       device then mark the buffer as "released" and no longer 
       in use, release the DMA channel and start the "detect WIP
       indicates complete" timer. */
    if (page_write_buffer_in_use) {
        page_write_buffer_in_use = false;
        LPC_GPDMACH0->DMACCConfig = 0;
        DMA_release_channel(0);
        LPC_SSP0->IMSC = (1UL << 3);
        rit_timer_set_counter(FLASH_WRITE_CB, FLASH_WIP_TEST_TIME);
        rval = 1;
    }
        
    return rval;
}

/** flash_read_ssp0_irq
 * 
 * Called by the SSP0 ISR handler.
 */
int flash_write_ssp0_irq(void) {
    if (page_write_in_progress) {
        if (LPC_SSP0->MIS & (1UL << 3)) {
            LPC_SSP0->IMSC &= ~(1UL << 3); 
            while(SSP0_IS_BUSY);            
            FLASH_CS_DEASSERT;            
            SSP0_release();
            return 1;
        }
    }
    return 0;
}


