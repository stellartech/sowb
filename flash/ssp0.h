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

#ifndef SSP0_H
#define SSP0_H

#define FLASH_SSP_INIT_CPSR 2
#define _25AA02E48_SSP_INIT_CPSR 0x64


#define SSP0_TX_FIFO_SPACE_AVAILABLE  \
    LPC_SSP0->SR & (1UL << 1)

#define SSP0_TX_FIFO_NOT_EMPTY \
    (!(SSP0_TX_FIFO_SPACE_AVAILABLE))

#define SSP0_IS_BUSY \
    LPC_SSP0->SR & (1UL << 4)

#define SSP0_FLUSH_RX_FIFO  \
    while(LPC_SSP0->SR & (1UL << 2)) { \
        unsigned int dev_null = LPC_SSP0->DR; \
    }

#define SSP0_WRITE_BYTE(byte)         \
    while(!SSP0_TX_FIFO_SPACE_AVAILABLE);  \
    LPC_SSP0->DR=(uint32_t)(byte&0xFF)
    

void SSP0_init(void);
bool SSP0_request(void);
void SSP0_release(void);

#endif    
