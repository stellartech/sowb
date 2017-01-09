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
#include "dma.h"
#include "ssp0.h"
#include "gpio.h"
#include "user.h"
#include "debug.h"

bool page_read_in_progress = false;

/* Local function prototypes. */
static int _flash_read_page(unsigned int page_address, char *buffer);

void flash_read_page(unsigned int page_address, char *buffer, bool block) {
    _flash_read_page(page_address, buffer);
    
    if (block) {
        while(page_read_in_progress) {
            WHILE_WAITING_DO_PROCESS_FUNCTIONS;
        }
    }
}

/** flash_read_in_progress
 */
bool flash_read_in_progress(void) {
    return page_read_in_progress;
}

/** flash_read_page
 * 
 * Load the given flash page into the supplied buffer.
 *
 * @param unsigned int the page to load, between 0 and 4095
 * @param char * buffer The RAM buffer to load the page to.
 */
static int _flash_read_page(unsigned int page_address, char *buffer) {

    /* We can't read a page while a write or erase is in progress. */    
    if (flash_write_in_progress() || flash_sector_erase_in_progress()) {
        return 0;
    }
    
    /* Wait for any previous read operation to complete. */
    while (page_read_in_progress) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    /* Mark a read is in operation. */
    page_read_in_progress = true; 
    
    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    /* Ensure the SSP1 RX FIFO is empty. */
    SSP0_FLUSH_RX_FIFO;
    
    /* Send the command and page read to the flash device. */
    FLASH_CS_ASSERT;
    FLASH_LONG_COMMAND(FLASH_READ, page_address);
    
    /* We use two DMA channels to achieve the required results.
       The higher priority channel0 is used to drive the SSP0
       SCLK0 pin with "don't care" bytes. We do this to flush 
       the bytes out of the flash device. We then use Channel1 
       to transfer the incoming bytes to RAM. */

    while(!DMA_request_channel(0)) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    while(!DMA_request_channel(1)) WHILE_WAITING_DO_PROCESS_FUNCTIONS;

    LPC_GPDMA->DMACIntTCClear = 0x3;
    LPC_GPDMA->DMACSoftSReq   = 0xC;
    
    /* Prep Channel1 to receive the incoming byte stream. */
    LPC_GPDMACH1->DMACCSrcAddr  = (uint32_t)&LPC_SSP0->DR;
    LPC_GPDMACH1->DMACCDestAddr = (uint32_t)buffer;
    LPC_GPDMACH1->DMACCLLI      = 0;
    LPC_GPDMACH1->DMACCControl  = DMA_CHANNEL_TCIE | DMA_CHANNEL_DST_INC | (uint32_t)FLASH_PAGE_SIZE;
    
    /* Prep Channel0 to send "don't care" bytes in order to clock out the data from the flash device. */
    LPC_GPDMACH0->DMACCSrcAddr  = (uint32_t)buffer; /* don't care data. */
    LPC_GPDMACH0->DMACCDestAddr = (uint32_t)&LPC_SSP0->DR;
    LPC_GPDMACH0->DMACCLLI      = 0;
    LPC_GPDMACH0->DMACCControl  = DMA_CHANNEL_TCIE | (uint32_t)FLASH_PAGE_SIZE;
    
    /* Enable SSP0 DMA. */
    LPC_SSP0->DMACR = 0x3;

    /* Enable Channel0 */
    LPC_GPDMACH0->DMACCConfig = DMA_CHANNEL_ENABLE | 
                                DMA_CHANNEL_DST_PERIPHERAL_SSP0_TX | 
                                DMA_TRANSFER_TYPE_M2P |
                                DMA_MASK_IE |
                                DMA_MASK_ITC;
    
    /* Wait until at least one byte has arrived into the RX FIFO
       and then start-up the Channel1 DMA to begin transferring them. */
    while((LPC_SSP0->SR & (1UL << 2)) == 0);
    
    /* Enable Channel1 */
    LPC_GPDMACH1->DMACCConfig = DMA_CHANNEL_ENABLE | 
                                DMA_CHANNEL_SRC_PERIPHERAL_SSP0_RX | 
                                DMA_TRANSFER_TYPE_P2M |
                                DMA_MASK_IE |
                                DMA_MASK_ITC;
                                
    /* SSP0 CS line and "page_read_in_progress" flag are now 
       under DMA/SSP0 interrupt control. See the DMA ISR handlers 
       and SSP0 ISR handlers for more information. */
       
    return 1;   
}

/** flash_read_ssp0_irq
 * 
 * Called by the SSP0 ISR handler.
 */
int flash_read_ssp0_irq(void) {
    if (page_read_in_progress) {
        if (LPC_SSP0->MIS & (1UL << 3)) {
            LPC_SSP0->IMSC &= ~(1UL << 3); 
            while(SSP0_IS_BUSY);
            FLASH_CS_DEASSERT;
            SSP0_release();
            page_read_in_progress = false; 
            return 1;
        }
    }
    return 0;
}

/* The following two functions are the DMA ISR handlers. They are
   called from dma.c so see that module for more details. */
   
/** flash_read_dma0_irq
 */
int flash_read_dma0_irq(int channel_number) {
    if (page_read_in_progress) {
        LPC_GPDMACH0->DMACCConfig = 0;
        DMA_release_channel(0);        
        return 1;        
    }
    return 0;
}

/** flash_read_dma1_irq
 */
int flash_read_dma1_irq(int channel_number) {
    if (page_read_in_progress) {
        LPC_GPDMACH1->DMACCConfig = 0;
        DMA_release_channel(1);
        LPC_SSP0->IMSC = (1UL << 3);
        return 1;        
    }
    return 0;
}

