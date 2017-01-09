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

bool ssp0_in_use;

bool SSP0_request(void) {
    if (!ssp0_in_use) {
        ssp0_in_use = true;
        return true;
    }
    return false;
}

void SSP0_release(void) {
    ssp0_in_use = false;
}

/* Declare ISR callbacks. */
extern int flash_read_ssp0_irq(void);
extern int flash_write_ssp0_irq(void);

/* Function pointer type for the following table. */
typedef int (*SSP0_callback)(void);

/* Define an array of callbacks to make. */
const SSP0_callback ssp0_callbacks[] = {
    flash_read_ssp0_irq,
    flash_write_ssp0_irq,
    NULL
};

/** SSP0_IRQHandler
 */
extern "C" void SSP0_IRQHandler(void) __irq {
    for (int i = 0; ssp0_callbacks[i] != NULL; i++) {
        if ((ssp0_callbacks[i])() != 0) {
            break;
        }
    }
}

/** SSP0_init
 */
void SSP0_init(void) {

    DEBUG_INIT_START;
    
    ssp0_in_use = false;
    
    /* The flash device is connected to SSP1 via the Mbed pins.
       So this init is about configuring just the SSP1. */
    
    /* Enable the SSP1 peripheral. */
    //LPC_SC->PCONP |= (1UL << 10);
    LPC_SC->PCONP |= (1UL << 21);

    /* Select the clock required for SSP1. */
    LPC_SC->PCLKSEL1  &= ~(3UL << 10);
    LPC_SC->PCLKSEL1  |=  (3UL << 10);
    //LPC_SC->PCLKSEL0  &= ~(3UL << 20);
    //LPC_SC->PCLKSEL0  |=  (3UL << 20);
    
    /* Select the GPIO pins for the SSP0 functions. */
    /* SCK0 */
    LPC_PINCON->PINSEL0  &= ~(3UL << 30);
    LPC_PINCON->PINSEL0  |=  (2UL << 30);
    /* MISO0 */
    LPC_PINCON->PINSEL1  &= ~(3UL << 2);
    LPC_PINCON->PINSEL1  |=  (2UL << 2);
    /* MISI0 */
    LPC_PINCON->PINSEL1  &= ~(3UL << 4);
    LPC_PINCON->PINSEL1  |=  (2UL << 4);

    /* Select the GPIO pins for the SSP1 functions. */
    /* SCK1 */
    //LPC_PINCON->PINSEL0  &= ~(3UL << 14);
    //LPC_PINCON->PINSEL0  |=  (2UL << 14);
    /* MISO1 */
    //LPC_PINCON->PINSEL0  &= ~(3UL << 16);
    //LPC_PINCON->PINSEL0  |=  (2UL << 16);
    /* MOSI1 */
    //LPC_PINCON->PINSEL0  &= ~(3UL << 18);
    //LPC_PINCON->PINSEL0  |=  (2UL << 18);
    
    /* Note, we don't use SSEL1 in our design, we just use a standard GPIO
       because writing multi-byte data is simpler. */
    
    /* Setup the interrupt system. Note however, the SSP1 interrupt
       is only actually activated within the DMA ISR and is self disabling. */
    NVIC_SetVector(SSP0_IRQn, (uint32_t)SSP0_IRQHandler);
    NVIC_EnableIRQ(SSP0_IRQn);
    
    /* Setup the control registers for SSP0 */
    LPC_SSP0->IMSC = 0;
    LPC_SSP0->CR0  = 0x7;
    LPC_SSP0->CPSR = FLASH_SSP_INIT_CPSR;
    LPC_SSP0->CR1  = 0x2;
    
    DEBUG_INIT_END;
}

