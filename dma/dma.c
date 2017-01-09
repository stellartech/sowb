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
#include "dma.h"

uint32_t channel_in_use_flags = 0;

/* Declare callback functions here before placing 
   in the array below. */
int flash_read_dma0_irq(int);
int flash_read_dma1_irq(int);
int flash_write_dma0_irq(int);

/* Make sure each array definition below ends with 
   a NULL,NULL struct to mark the end of the array. */

const DMA_CALLBACKS   dma_channel0[] = {
    { flash_read_dma0_irq,      flash_read_dma0_irq },
    { flash_write_dma0_irq,     flash_write_dma0_irq },
    { NULL,                     NULL }
};

const DMA_CALLBACKS   dma_channel1[] = {
    { flash_read_dma1_irq,      flash_read_dma1_irq },
    { NULL,                     NULL }
};

const DMA_CALLBACKS   dma_channel2[] = {
    { NULL,                     NULL }
};

const DMA_CALLBACKS   dma_channel3[] = {
    { NULL,                     NULL }
};

const DMA_CALLBACKS   dma_channel4[] = {
    { NULL,                     NULL }
};

const DMA_CALLBACKS   dma_channel5[] = {
    { NULL,                     NULL }
};

const DMA_CALLBACKS   dma_channel6[] = {
    { NULL,                     NULL }
};

const DMA_CALLBACKS   dma_channel7[] = {
    { NULL,                     NULL }
};

/* Don't change anything below here. */

/* An array of pointers to the channel ISR handlers. */
const DMA_CALLBACKS   *dma_channels[8] = {
    dma_channel0, dma_channel1, dma_channel2, dma_channel3,
    dma_channel4, dma_channel5, dma_channel6, dma_channel7
};

/** DMA_IRQHandler
 */
extern "C" void DMA_IRQHandler(void) __irq {
    for (int channel_number = 0; channel_number < 8; channel_number++) {
        if (LPC_GPDMA->DMACIntStat & (1UL << channel_number)) {
            if (LPC_GPDMA->DMACIntTCStat & (1UL << channel_number)) {
                int irq_idx = 0;
                while (dma_channels[channel_number][irq_idx].TcCallback != NULL) {
                    if ((dma_channels[channel_number][irq_idx].TcCallback)(channel_number)) {
                        LPC_GPDMA->DMACIntTCClear = (1UL << channel_number);
                        break;
                    }
                    irq_idx++;
                }
            }
            if (LPC_GPDMA->DMACIntErrStat & (1UL << channel_number)) {
                int irq_idx = 0;
                while (dma_channels[channel_number][irq_idx].ErrCallback != NULL) {
                    if ((dma_channels[channel_number][irq_idx].ErrCallback)(channel_number)) {
                        LPC_GPDMA->DMACIntErrClr = (1UL << channel_number);
                        break;
                    }
                    irq_idx++;
                }
            }
        }
    }
    
    /* IRQ should be handled by now, check to make sure. */
    if (LPC_GPDMA->DMACIntStat) {
        LPC_GPDMA->DMACIntTCClear = (uint32_t)0xFF; /* If not, clear anyway! */
    }
    if (LPC_GPDMA->DMACIntErrStat) {
        LPC_GPDMA->DMACIntErrClr = (uint32_t)0xFF; /* If not, clear anyway! */
    }
}


/** DMA_init
 */
void DMA_init(void) {
    DEBUG_INIT_START;
    LPC_SC->PCONP |= (1UL << 29);
    LPC_GPDMA->DMACConfig = 1;
    NVIC_SetVector(DMA_IRQn, (uint32_t)DMA_IRQHandler);
    NVIC_EnableIRQ(DMA_IRQn);
    DEBUG_INIT_END;
}

/** DMA_process
 */
void DMA_process(void) {
    /* Nothing to do. */
}

/** dma_request_channel
 *
 * Used to request control of a DMA channel. Allows modules
 * to share channels if needed.
 *
 * @param int channel The channel being requested.
 * @return bool true if given control, false if another is already in control.
 */
bool DMA_request_channel(int channel) {
    if (!(channel_in_use_flags & (1UL << channel))) {
        channel_in_use_flags |= (1UL << channel);
        return true;
    }
    return false;
}

/** dma_release_channel
 *
 * Used to release a previously requested channel.
 *
 * @param int channel The channel to release.
 */
void DMA_release_channel(int channel) {
    channel_in_use_flags &= ~(1UL << channel);
}

