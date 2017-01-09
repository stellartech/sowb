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
 #include "gpio.h"
 
 
 /** gpio_init
  */
 void gpio_init(void) {
 
    DEBUG_INIT_START;
    
    /* The following code could be condensed into a single set of
       and/or statements. However, for code clarity they are laid
       out on a "use case" way to see each pin being defined. Since
       this is "init" code and only called once at start-up, the
       extra overhead isn't worth the effort over cleaner to read
       code. */
       
    /* The MAX7456 module uses p0.23 (p15) for it's chip select
       output. Define it's setup here and use the macros in gpio.h
       to assert, deassert or read the pin. */
    //LPC_PINCON->PINSEL1 &= ~(3UL << 14);  /* Function GPIO. */
    //LPC_GPIO0->FIODIR   |=  (1UL << 23);  /* P0.23 as output. */
    
    /* The MAX7456 module uses p0.16 (p14) for it's chip select
       output. Define it's setup here and use the macros in gpio.h
       to assert, deassert or read the pin. */
    //LPC_PINCON->PINSEL1 &= ~(3UL << 14);  /* Function GPIO. */
    //LPC_GPIO0->FIODIR   |=  (1UL << 16);  /* P0.23 as output. */

    /* The MAX7456 module uses p0.6 (p8) for it's chip select
       output. Define it's setup here and use the macros in gpio.h
       to assert, deassert or read the pin. */
    LPC_PINCON->PINSEL0 &= ~(3UL << 12);  /* Function GPIO. */
    LPC_GPIO0->FIODIR   |=  (1UL <<  6);  /* P0.6 as output. */       

             
    /* The MAX7456 module uses p1.31 (p20) for it's reset output.
       Define it's setup here and use the macros in gpio.h to assert, 
       deassert or read the pin. */
    LPC_PINCON->PINSEL3 &= ~(3UL << 30);  /* Function GPIO. */
    LPC_GPIO1->FIODIR   |=  (1UL << 31);  /* P1.31 as output. */
    
    /* We use p0.25 (p17) for the SD Card detect. */
    LPC_PINCON->PINSEL1 &= ~(3UL << 18);  /* Function GPIO. */
    LPC_GPIO0->FIODIR   &= ~(1UL << 25);  /* P0.25 as Input. */
    
    /* We use p0.16 (p14) for the Flash device SSP0 CS signal. */
    LPC_PINCON->PINSEL1 &= ~(3UL << 14);  /* Function GPIO. */
    LPC_GPIO0->FIODIR   |=  (1UL << 16);  /* P0.23 as output. */

    /* We use p0.24 (p16) for the 25AA02E48 device SSP0 CS signal. */
    LPC_PINCON->PINSEL1 &= ~(3UL << 14);  /* Function GPIO. */
    LPC_GPIO0->FIODIR   |=  (1UL << 24);  /* P0.24 as output. */ 
    
    /* We use p1.30 (p19) for the MicroSD card device SSP0 CS signal. */
    LPC_PINCON->PINSEL3 &= ~(3UL << 28);  /* Function GPIO. */
    LPC_GPIO1->FIODIR   |=  (1UL << 30);  /* P1.30 as output. */    
    
    /* We use p2.5 (p21) for debugging. */
    LPC_PINCON->PINSEL4 &= ~(3UL << 10);  /* Function GPIO. */
    LPC_GPIO2->FIODIR   |=  (1UL << 5);  /* P2.5 as output. */
    
    /* We use p2.4 (p22) for debugging. */
    LPC_PINCON->PINSEL4 &= ~(3UL << 8);  /* Function GPIO. */
    LPC_GPIO2->FIODIR   |=  (1UL << 4);  /* P2.4 as output. */
    
    
#ifdef MBED_LEDS
    /* The MBED has four useful little blue LEDs that can be used. 
       Mbed examples use the DigitalOut led1(LED1) style. Mimic that
       using our system here. Here however, I will use shorthand ;)     
                                LED1           LED2           LED3           LED4     */
    LPC_PINCON->PINSEL3 &= ( ~(3UL <<  4) & ~(3UL <<  8) & ~(3UL << 10) & ~(3UL << 14) );  
    LPC_GPIO1->FIODIR   |= (  (1UL << 18) |  (1UL << 20) |  (1UL << 21) |  (1UL << 23) );
#endif
    
    
    SSP0_CS_DEASSERT;
    FLASH_CS_DEASSERT;
    SDCARD_CS_DEASSERT;
    AA02E48_CS_DEASSERT;
    MAX7456_CS_DEASSERT;
    
    DEBUG_INIT_END;
}
 
/** gpio_process
 */
void gpio_process(void) {
   /* Does nothing, no house keeping required. */
}

