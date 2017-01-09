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
#include "user.h"
#include "debug.h"
#include "rit.h"
#include "gps.h"
#include "gpio.h"
#include "ff.h"

FATFS MyFileSystem;
bool card_detected_A;
bool card_detected_B;
bool card_mounted;
bool card_bad;
uint32_t sdcard_detect_shift_register;

/** sdcard_is_mounted
 *
 * Tell the caller if an SD card is mounted.
 *
 * @return bool True if a card is mounted, false otherwise.
 */
bool sdcard_is_mounted(void) {
    return card_mounted;
}

/** sdcard_init()
 */
void sdcard_init(void) {
    DEBUG_INIT_START;
    card_bad        = false;
    card_mounted    = false;
    card_detected_A = false;
    card_detected_B = false;
    sdcard_detect_shift_register = 0;
    rit_timer_set_counter(SDCARD_TIMER_CB, 10);
    DEBUG_INIT_END;
}

/** sdcard_process()
 */             
void sdcard_process(void) {
    int retry;
    int f_return;
    DIR root_directory;
    
    /* card_detected_A is set by the RIT timer callback below which performs
       a rough debounce of the switch. So, if here we need to do a rising
       edge detect so we only flag a card insertion once. */
    if (card_detected_A && !card_detected_B && !card_mounted && !card_bad) {
        card_detected_B = true;        
        debug_printf("Card insertion detected.\r\n");
        /* Have found that some cards require a "kick" to bring them to
           life. So we retry 3 times to init the card. If after 3 tries
           it fails we mark the card as "bad". */
        for(retry = 0; retry < 3; retry++) {
            KICK_WATCHDOG;
            f_return = f_mount(0,&MyFileSystem);
            if (f_return == FR_OK) {
                f_return = f_opendir(&root_directory, "/");
                if (f_return == FR_OK) {
                    card_mounted = true;
                    debug_printf("SD card mounted.\r\n");
                    retry = 3; /* Break loop. */
                }
            }    
        }
        
        if (!card_mounted) {
            card_bad = true;
            debug_printf("Failed to mount SD Card\r\n");
        }
    }
}

/** _sdcard_timer_callback
 *
 * RIT timer callback.
 * @see rit.c
 */
void _sdcard_timer_callback(int index) {
    
    /* Shift left 1bit... */
    sdcard_detect_shift_register = sdcard_detect_shift_register << 1;
    
    /* And then mask in bit0 position. */
    if (SDCARD_DETECT != 0) {  
        sdcard_detect_shift_register |= 1;
    }

    /* 16 consecutive 1s mean card detected. Rough debouncing of the
       mechanical "card detect" switch in the SD card holder. */
    if ((sdcard_detect_shift_register & 0x0000FFFFUL) == 0x0000FFFFUL) { 
        card_detected_A = true; 
    }
    else {
        card_bad        = false;
        card_mounted    = false;
        card_detected_A = false;
        card_detected_B = false;
    }
    
    /* Restart timer. */
    rit_timer_set_counter(SDCARD_TIMER_CB, 10);
}
