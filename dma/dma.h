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

#ifndef DMA_H
#define DMA_H

#define DMA_CHANNEL_0   (1UL << 0)
#define DMA_CHANNEL_1   (1UL << 1)
#define DMA_CHANNEL_2   (1UL << 2)
#define DMA_CHANNEL_3   (1UL << 3)
#define DMA_CHANNEL_4   (1UL << 4)
#define DMA_CHANNEL_5   (1UL << 5)
#define DMA_CHANNEL_6   (1UL << 6)
#define DMA_CHANNEL_7   (1UL << 7)

typedef int (*DMA_callback)(int channel);

typedef struct _dma_callbacks {
    DMA_callback    TcCallback;
    DMA_callback    ErrCallback; 
} DMA_CALLBACKS;

void DMA_init(void);
void DMA_process(void);
bool DMA_request_channel(int channel);
void DMA_release_channel(int channel);

#endif
