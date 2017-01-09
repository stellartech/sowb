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
#include "rit.h"
#include "gps.h"
#include "gpio.h"

volatile uint32_t uptimeL;
volatile uint32_t uptimeH;

/* Declare "timer at zero" callback function prototypes. */
void _gps_timer_tick_cb(int);
void _user_wait_ms_cb(int);
void _nexstar_timeout_callback(int);
void _nexstar_one_second_timer(int index);
void _nexstar_100th_timer(int index);
void _flash_write_timer_callback(int index);
void _sdcard_timer_callback(int index);

/* Define an array of timers that the ISR should handle. */
volatile RIT_TIMER timers[] = {
    { 10, 10, _gps_timer_tick_cb            },      /* Index 0 */
    {  0,  0, _user_wait_ms_cb              },      /* Index 1 */
    {  0,  0, _nexstar_timeout_callback     },      /* Index 2 */
    {  0,  0, _nexstar_one_second_timer     },      /* Index 3 */
    {  0,  0, _nexstar_100th_timer          },      /* Index 4 */
    {  0,  0, _flash_write_timer_callback   },      /* Index 5 */
    {  0,  0, _sdcard_timer_callback        },      /* Index 6 */
//    {  0,  0, _main_test_callback           },      /* Index 7 */
    {  0,  0, NULL                          }       /* Always the last entry. */
};

/** RIT_IRQHandler
 *
 * The ISR for the RIT.
 */
extern "C" static void RIT_IRQHandler(void) __irq {
    if (++uptimeL == 0) uptimeH++;
    for (int index = 0; timers[index].callback != NULL; index++) {
        if (timers[index].counter > 0) {
            timers[index].counter--;
            if (timers[index].counter == 0) {
                (timers[index].callback)(index);
                if (timers[index].reload > 0) {
                    timers[index].counter = timers[index].reload;
                }
            }
        }
    }
    LPC_RIT->RICTRL |= 0x1; /* Dismiss the IRQ. */
}

/** rit_index_check
 *
 * Check that an index exists. Ensures that index into array is valid.
 *
 * @param int inex The index to check.
 * @return int zero on non-existent index, non-zero otherwise.
 */
inline static bool rit_index_check(int index) {
    for (int i = 0; timers[i].callback != NULL; i++) if (i == index) return true;
    return false;
}

/** rit_timer_set_counter
 */
void rit_timer_set_counter(int index, uint32_t value) {
    if (rit_index_check(index)) timers[index].counter = value;
}

/** rit_timer_set_reload
 */
void rit_timer_set_reload(int index, uint32_t value) {
    if (rit_index_check(index)) timers[index].reload = value;
}

/** rit_timer_get_values
 *
 * Get the current counter/reload values of the specified timer.
 *
 * If the supplied "index" value is out of range the index value pointed to by
 * the caller is set to -1 to signify an error has occured (subscript/index out
 * of range).
 *
 * @param int * index A pointer to an int for the index value of the array to return.
 * @param uint32_t * counter A pointer to variable to place the counter value into.
 * @param uint32_t * counter A pointer to variable to place the reload value into.
 */
void rit_timer_get_values(int *index, uint32_t *counter, uint32_t *reload) {
    if (rit_index_check(*(index))) {
        *(counter) = timers[*(index)].counter;
        *(reload)  = timers[*(index)].reload;
    }
    else *(index) = -1;
}

/** rit_get_uptime
 */
void rit_read_uptime(uint32_t *h, uint32_t *l) {
    *(l) = uptimeL;
    *(h) = uptimeH;
}

/** rit_init
 */
void rit_init(void) {
    
    DEBUG_INIT_START;
    
    uptimeL = uptimeH = 0;
    
    /* Start by switching power on to the peripheral */
    LPC_SC->PCONP |= (1UL << 16); 
    
    /* The compare value depends on the PCLK frequency.
       The following are based on a CCLK of 96Mhz to
       achieve a 1ms timeout value. */
    switch ((LPC_SC->PCLKSEL1 >> 26) & 0x3) {
        case 0: LPC_RIT->RICOMPVAL = (96000000L / 4 / 1000); break; /* CCLK / 4 */
        case 1: LPC_RIT->RICOMPVAL = (96000000L / 1 / 1000); break; /* CCLK / 1 */
        case 2: LPC_RIT->RICOMPVAL = (96000000L / 2 / 1000); break; /* CCLK / 2 */
        case 3: LPC_RIT->RICOMPVAL = (96000000L / 8 / 1000); break; /* CCLK / 8 */
    }
    
    /* Set the ISR vector and enable interrupts for the RIT. */
    NVIC_SetVector(RIT_IRQn, (uint32_t)RIT_IRQHandler);
    NVIC_EnableIRQ(RIT_IRQn);
    
    /* Enable the RIT. */
    LPC_RIT->RICTRL     = 0x0000000A;
    
    DEBUG_INIT_END;
}
