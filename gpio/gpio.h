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
 
#ifndef GPIO_H
#define GPIO_H

/* Switch on our macros to use the 4 LEDs on the Mbed. */
#define MBED_LEDS
 
/* Used by the MAX7456 module. 
   The MAX7456 !cs signal is connected to Mbed pin 8
   which is internally connected to Port0.6 */
#define SET_P0_06       (LPC_GPIO0->FIOSET = (1UL << 6))
#define CLR_P0_06       (LPC_GPIO0->FIOCLR = (1UL << 6))
#define VAL_P0_06       (LPC_GPIO0->FIOPIN & (1UL << 6))
#define MAX7456_CS_ASSERT      CLR_P0_06
#define MAX7456_CS_DEASSERT    SET_P0_06
#define MAX7456_CS_VALUE       VAL_P0_06
#define MAX7456_CS_TOGGLE      MAX7456_CS_VALUE ? MAX7456_CS_DEASSERT : MAX7456_CS_ASSERT

/* Used by the MAX7456 module. 
   The MAX7456 !rst signal is connected to Mbed pin 20
   which is internally connected to Port1.31 */
#define SET_P1_31      (LPC_GPIO1->FIOSET = (1UL << 31))
#define CLR_P1_31      (LPC_GPIO1->FIOCLR = (1UL << 31))
#define VAL_P1_31      (LPC_GPIO1->FIOPIN & (1UL << 31))
#define MAX7456_RST_ASSERT     CLR_P1_31
#define MAX7456_RST_DEASSERT   SET_P1_31
#define MAX7456_RST_VALUE      VAL_P1_31
#define MAX7456_RST_TOGGLE     MAX7456_RST_VALUE ? MAX7456_RST_DEASSERT : MAX7456_RST_ASSERT
#define SSP0_CS_ASSERT         CLR_P1_31
#define SSP0_CS_DEASSERT       SET_P1_31
#define SSP0_CS_VALUE          VAL_P1_31
#define SSP0_CS_TOGGLE         MAX7456_RST_VALUE ? MAX7456_RST_DEASSERT : MAX7456_RST_ASSERT

/* Used by the flash module. 
   The flash !cs signal is connected to Mbed pin 14
   which is internally connected to Port0.16 */
#define SET_P0_16      (LPC_GPIO0->FIOSET = (1UL << 16))
#define CLR_P0_16      (LPC_GPIO0->FIOCLR = (1UL << 16))
#define VAL_P0_16      (LPC_GPIO0->FIOPIN & (1UL << 16))
#define FLASH_CS_ASSERT         CLR_P0_16
#define FLASH_CS_DEASSERT       SET_P0_16
#define FLASH_CS_VALUE          VAL_P0_16
#define FLASH_CS_TOGGLE         FLASH_CS_VALUE ? FLASH_CS_DEASSERT : FLASH_CS_ASSERT

/* Used by the 25AA02E48 module. 
   The device !cs signal is connected to Mbed pin 16
   which is internally connected to Port0.24 */
#define SET_P0_24      (LPC_GPIO0->FIOSET = (1UL << 24))
#define CLR_P0_24      (LPC_GPIO0->FIOCLR = (1UL << 24))
#define VAL_P0_24      (LPC_GPIO0->FIOPIN & (1UL << 24))
#define AA02E48_CS_ASSERT       CLR_P0_24
#define AA02E48_CS_DEASSERT     SET_P0_24
#define AA02E48_CS_VALUE        VAL_P0_24
#define AA02E48_CS_TOGGLE       AA02E48_CS_VALUE ? AA02E48_CS_DEASSERT : AA02E48_CS_ASSERT

/* Used by the SD card. 
   The device !cs signal is connected to Mbed pin 19
   which is internally connected to Port1.30 */
#define SET_P1_30      (LPC_GPIO1->FIOSET = (1UL << 30))
#define CLR_P1_30      (LPC_GPIO1->FIOCLR = (1UL << 30))
#define VAL_P1_30      (LPC_GPIO1->FIOPIN & (1UL << 30))
#define SDCARD_CS_ASSERT       CLR_P1_30
#define SDCARD_CS_DEASSERT     SET_P1_30
#define SDCARD_CS_VALUE        VAL_P1_30
#define SDCARD_CS_TOGGLE       SDCARD_CS_VALUE ? SDCARD_CS_DEASSERT : SDCARD_CS_ASSERT

/* Used for reading the SD Card detect pin. */
#define SDCARD_DETECT   (LPC_GPIO0->FIOPIN & (1UL << 25))       

/* For debugging. 
   Mbed pin 21 which is internally connected to Port2.5 */
#define SET_P2_05      (LPC_GPIO2->FIOSET = (1UL << 5))
#define CLR_P2_05      (LPC_GPIO2->FIOCLR = (1UL << 5))
#define VAL_P2_05      (LPC_GPIO2->FIOPIN & (1UL << 5))
#define P21_ASSERT     SET_P2_05
#define P21_DEASSERT   CLR_P2_05
#define P21_VALUE      VAL_P2_05
#define P21_TOGGLE     P21_VALUE ? P21_DEASSERT : P21_ASSERT

/* For debugging. 
   Mbed pin 22 which is internally connected to Port2.4 */
#define SET_P2_04      (LPC_GPIO2->FIOSET = (1UL << 4))
#define CLR_P2_04      (LPC_GPIO2->FIOCLR = (1UL << 4))
#define VAL_P2_04      (LPC_GPIO2->FIOPIN & (1UL << 4))
#define P22_ASSERT     SET_P2_04
#define P22_DEASSERT   CLR_P2_04
#define P22_VALUE      VAL_P2_04
#define P22_TOGGLE     P22_VALUE ? P22_DEASSERT : P22_ASSERT

#ifdef MBED_LEDS
#define LED1_ON        (LPC_GPIO1->FIOSET = (1UL << 18))
#define LED1_OFF       (LPC_GPIO1->FIOCLR = (1UL << 18))
#define LED1_IS_ON     (LPC_GPIO1->FIOPIN & (1UL << 18))
#define LED1_TOGGLE    LED1_IS_ON ? LED1_OFF : LED1_ON
#define LED2_ON        (LPC_GPIO1->FIOSET = (1UL << 20))
#define LED2_OFF       (LPC_GPIO1->FIOCLR = (1UL << 20))
#define LED2_IS_ON     (LPC_GPIO1->FIOPIN & (1UL << 20))
#define LED2_TOGGLE    LED2_IS_ON ? LED2_OFF : LED2_ON
#define LED3_ON        (LPC_GPIO1->FIOSET = (1UL << 21))
#define LED3_OFF       (LPC_GPIO1->FIOCLR = (1UL << 21))
#define LED3_IS_ON     (LPC_GPIO1->FIOPIN & (1UL << 21))
#define LED3_TOGGLE    LED3_IS_ON ? LED3_OFF : LED3_ON
#define LED4_ON        (LPC_GPIO1->FIOSET = (1UL << 23))
#define LED4_OFF       (LPC_GPIO1->FIOCLR = (1UL << 23))
#define LED4_IS_ON     (LPC_GPIO1->FIOPIN & (1UL << 23))
#define LED4_TOGGLE    LED4_IS_ON ? LED4_OFF : LED4_ON
#endif
 
/* Function prototypes. */
void gpio_init(void);
void gpio_process(void);
 
#endif
 