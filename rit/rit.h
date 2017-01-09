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
 
#ifndef RIT_H
#define RIT_H

#define RIT_TIMER_CB_GPS    0
#define RIT_TIMER_CB_WAIT   1
#define RIT_TIMER_NEXSTAR   2
#define RIT_ONESEC_NEXSTAR  3
#define RIT_100TH_NEXSTAR   4
#define FLASH_WRITE_CB      5
#define SDCARD_TIMER_CB     6
#define MAIN_TEST_CB        7

#include "sowb.h"

typedef struct _rit_timer {
    uint32_t    reload;         /* Value to reload for recurring mode. */
    uint32_t    counter;        /* The acutal counter that "down counts" to zero. */
    void      (*callback)(int); /* The function to call when zero is reached. */
} RIT_TIMER;

void rit_init(void);
void rit_timer_set_counter(int index, uint32_t value);
void rit_timer_set_reload(int index, uint32_t value);
void rit_timer_get_values(int *, uint32_t *, uint32_t *);
void rit_read_uptime(uint32_t *h, uint32_t *l);

#endif
