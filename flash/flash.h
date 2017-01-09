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

#ifndef FLASH_H
#define FLASH_H

#define FLASH_PAGE_SIZE 256

#define FLASH_WIP_TEST_TIME 2

#define FLASH_WREN      0x06
#define FLASH_WRDI      0x04
#define FLASH_RDID      0x9F
#define FLASH_RDSR      0x05
#define FLASH_WRSR      0x01
#define FLASH_READ      0x03
#define FLASH_FAST_READ 0x0B
#define FLASH_PP        0x02
#define FLASH_SE        0xD8
#define FLASH_BE        0xC7

#define DMA_CHANNEL_ENABLE  1
//#define DMA_CHANNEL_SRC_PERIPHERAL_SSP1_RX  (3UL << 1)
//#define DMA_CHANNEL_SRC_PERIPHERAL_SSP1_TX  (2UL << 1)
//#define DMA_CHANNEL_DST_PERIPHERAL_SSP1_RX  (3UL << 6)
//#define DMA_CHANNEL_DST_PERIPHERAL_SSP1_TX  (2UL << 6)
#define DMA_CHANNEL_SRC_PERIPHERAL_SSP0_RX  (1UL << 1)
#define DMA_CHANNEL_SRC_PERIPHERAL_SSP0_TX  (0UL << 1)
#define DMA_CHANNEL_DST_PERIPHERAL_SSP0_RX  (1UL << 6)
#define DMA_CHANNEL_DST_PERIPHERAL_SSP0_TX  (0UL << 6)
#define DMA_CHANNEL_SRC_INC                 (1UL << 26)
#define DMA_CHANNEL_DST_INC                 (1UL << 27)
#define DMA_CHANNEL_TCIE                    (1UL << 31)
#define DMA_TRANSFER_TYPE_M2M               (0UL << 11)
#define DMA_TRANSFER_TYPE_M2P               (1UL << 11)
#define DMA_TRANSFER_TYPE_P2M               (2UL << 11)
#define DMA_TRANSFER_TYPE_P2P               (3UL << 11)
#define DMA_MASK_IE                         (1UL << 14)
#define DMA_MASK_ITC                        (1UL << 15)
#define DMA_LOCK                            (1UL << 16)
#define DMA_ACTIVE                          (1UL << 17)
#define DMA_HALT                            (1UL << 18)

#define FLASH_SHORT_COMMAND(x)              \
    SSP0_WRITE_BYTE(x);                     \
    while(SSP0_IS_BUSY);                    \
    SSP0_FLUSH_RX_FIFO;

#define FLASH_LONG_COMMAND(x,y)             \
    SSP0_WRITE_BYTE(x);                     \
    SSP0_WRITE_BYTE((y >> 8) & (0xFF));     \
    SSP0_WRITE_BYTE(y & 0xFF);              \
    SSP0_WRITE_BYTE(0);                     \
    while(SSP0_IS_BUSY);                    \
    SSP0_FLUSH_RX_FIFO;
    
#define FLASH_WAIT_WHILE_WIP                \
    SSP0_CS_ASSERT;                         \
    FLASH_SHORT_COMMAND(FLASH_RDSR);        \
    SSP0_FLUSH_RX_FIFO;                     \
    do {                                    \
        SSP0_WRITE_BYTE(0);                 \
    }                                       \
    while (LPC_SSP0->DR & 0x1);             \
    SSP0_CS_DEASSERT;
    

/* Defined in flash.c */
void flash_init(void);
void flash_process(void);
char flash_getc(bool peek);
void flash_seek(unsigned int addr);

/* Defined in flash_read.c */
bool flash_read_in_progress(void);
void  flash_read_page(unsigned int page_address, char *buffer, bool block);

/* Defined in flash_write.c */
int  flash_erase_sector(int sector);
bool flash_write_in_progress(void);
bool flash_sector_erase_in_progress(void);
int  flash_page_write(int page, char *buffer);

/* Defined in flash_erase.c */
bool flash_sector_erase_in_progress(void);
int flash_erase_sector(int sector);
int flash_erase_bulk(void);

/* Defined in 25AA02E44.c */
void _25AA02E48_init(void);
void _25AA02E48_mac_addr(char *);
void _25AA02E48_mac_addr_printable(char *, char);

#endif
