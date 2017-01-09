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
#include "gpioirq.h"

/* Declare the external interrupt callback functions. */
extern void MAX7456_vsync_rise(void);
extern void MAX7456_vsync_fall(void);
//extern void max7456_los_rise(void);
//extern void max7456_los_fall(void);
extern void gps_pps_rise(void);
extern void gps_pps_fall(void);

/** EINT3_IRQHandler
 */
extern "C" void GPIOIRQ_IRQHandler (void) {
    
    /* Test for IRQ on Port0. */
    if (LPC_GPIOINT->IntStatus & 0x1) {
        
        /* GPS PPS is connected to MBED P29 (Port0.5) */
        //if (LPC_GPIOINT->IO0IntStatR & (1 << 5))  gps_pps_rise();      

        /* GPS PPS is connected to MBED P29 (Port0.5) */
        if (LPC_GPIOINT->IO0IntStatF & (1 << 5))  gps_pps_fall();

        /* MAX7456 Vertical Sync is connected to MBED P15 (Port0.23) */
        if (LPC_GPIOINT->IO0IntStatF & (1 << 23)) MAX7456_vsync_fall();
        
        /* MAX7456 LOS is connected to P17 (Port0.25) */
        //if (LPC_GPIOINT->IO0IntStatR & (1 << 25)) max7456_los_rise();  

        LPC_GPIOINT->IO0IntClr = (LPC_GPIOINT->IO0IntStatR | LPC_GPIOINT->IO0IntStatF);
    }

    /* Test for IRQ on Port2. */
    if (LPC_GPIOINT->IntStatus & 0x4) {
                
        LPC_GPIOINT->IO2IntClr = (LPC_GPIOINT->IO2IntStatR | LPC_GPIOINT->IO2IntStatF);
    }
}
 
/** gpioirq_init
 */
void gpioirq_init(void) {

    DEBUG_INIT_START;
    
    /* Enable the interrupts for connected signals. 
       For bit definitions see the ISR function above. */
    LPC_GPIOINT->IO0IntEnR |= ( (1UL << 5) | (1UL << 25) | (1UL << 23) );
    LPC_GPIOINT->IO0IntEnF |= ( (1UL << 5) | (1UL << 25) | (1UL << 23) );
    //LPC_GPIOINT->IO2IntEnR |= ( (1UL << 5) );
    //LPC_GPIOINT->IO2IntEnF |= ( (1UL << 5) );
    
    NVIC_SetVector(EINT3_IRQn, (uint32_t)GPIOIRQ_IRQHandler);
    NVIC_EnableIRQ(EINT3_IRQn);
    
    DEBUG_INIT_END;
}

void gpioirq_process(void) {
    /* Does nothing, no periodic house keeping required. */
}

 
 