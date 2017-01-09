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

#include "rit.h" 
#include "gpio.h"
#include "user.h"
#include "nexstar.h"
#include "xbox360gamepad.h"
#include "debug.h"

#define STICK_DIVISOR   4096.0

void handle_stick_left(void);
void handle_stick_right(void);

typedef void (PROCESS_FUNC)();

extern PROCESS_FUNC *process_callbacks[];

USER_INPUT user_input;

/** user_get_button
 *
 * Used to get a "button press" from the user. This is basically
 * a read of the XBox360 gamepad interface via the USBEH system.
 * However, this function is provided to abstract that so that
 * other systems can "inject" button presses into the system.
 * The intention here is for a virtual console on a PC to 
 * control the SOWB system via the (to be done) pccomms.c module.
 * This is just a conveinent way to get user IO into the SOWB.
 *
 * Note, this fnction calls all the module _process() functions
 * in it's loop (ie while no user IO is occuring). That gives the
 * SOWB modules a chance to get work done outside of time critical
 * interrupts. _process() functions should NEVER block and should 
 * also be "fast" so as to avoid any user IO perseptive delay.
 *
 * Note, if arg peek is true, we return the next button press in
 * the buffer without actually removing that event from the buffer.
 * Allows us to look to see if an pending button press is within
 * the buffer awaiting being handled.
 *
 * @param bool peek
 * @return char The button pressed.
 */
char user_get_button(bool peek) {
    char c;
    
    if (peek) {
        /* Make a call to all _process()'s just in case we find 
           ourselves stuck in a tight loop waiting for an event. */
        for (c = 0; process_callbacks[c] != NULL; c++) (process_callbacks[c])();
        return xbox360gamepad_get_button_preview();
    }
    
    do {
        /* Call all module _process functions. */
        for (c = 0; process_callbacks[c] != NULL; c++) {
            KICK_WATCHDOG;
            (process_callbacks[c])();
        }
        
        handle_stick_left();
        handle_stick_right();
        
        /* Get a button press from the user or go round again. */
        c = xbox360gamepad_get_button();
    }
    while (c == 0);
    
    return c;
}

/* A flag to signify a timeout has occured. */
volatile int user_wait_ms_flag;

/** _user_wait_ms_cb
 *
 * Called by the RIT system to signal a timer timeout.
 *
 * @param int t_index The timer index value (handle).
 */
void _user_wait_ms_cb(int t_index) {
    user_wait_ms_flag = 0;
}

/** user_wait_ms
 *
 * Used to "wait" for a specified time interval. 
 * Note, while "waiting" for the timeout, it will
 * call all the modules _process() functions to 
 * allow modules to perform non-time critical work
 * outside of interrupts.
 *
 * @param uint32_t ms The delay amount.
 */
void user_wait_ms(uint32_t ms) {
    rit_timer_set_counter(RIT_TIMER_CB_WAIT, ms);
    user_wait_ms_flag = 1;
    while (user_wait_ms_flag) 
        for (int c = 0; process_callbacks[c] != NULL; c++) { 
            KICK_WATCHDOG;
            (process_callbacks[c])();
        }
}

/** user_wait_ms
 *
 * Used to "wait" for a specified time interval. 
 * Note, unlike the function above, NO _process()
 * functions are called. This really is a blocking
 * delay and should be used with care!
 *
 * @param uint32_t ms The delay amount.
 */
void user_wait_ms_blocking(uint32_t ms) {
    rit_timer_set_counter(RIT_TIMER_CB_WAIT, ms);
    user_wait_ms_flag = 1;
    while (user_wait_ms_flag) KICK_WATCHDOG;
}

/** user_call_process
 */
void user_call_process(void) {
    for (int c = 0; process_callbacks[c] != NULL; c++) {
        KICK_WATCHDOG; 
        (process_callbacks[c])();
    }
}

void handle_stick_left(void) {
    static XBOX360_STICK stick_left_previous;
    XBOX360_STICK *stick;
    int x, y;
    double rate;
    
    stick = xbox360gamepad_get_stick_left();
    if (stick->x/STICK_DIVISOR != stick_left_previous.x/STICK_DIVISOR || stick->y/STICK_DIVISOR != stick_left_previous.y/STICK_DIVISOR) {
        stick_left_previous.x = stick->x;
        stick_left_previous.y = stick->y;
        x = stick->x/STICK_DIVISOR;
        y = stick->y/STICK_DIVISOR;
        
        if (x > 2 || x < -2) {
            rate = 6.0 * (double)((double)stick->x/STICK_DIVISOR / 16.0);
            _nexstar_set_azmith_rate_coarse(rate);
        }
        else {
            _nexstar_set_azmith_rate_coarse(0.0);
        }
        
        if (y > 2 || y < -2) {
            rate = 6.0 * (double)((double)stick->y/STICK_DIVISOR / 16.0);
            _nexstar_set_elevation_rate_coarse(rate);
        }
        else {
            _nexstar_set_elevation_rate_coarse(0.0);
        }
    }    
}

void handle_stick_right(void) {
    static XBOX360_STICK stick_previous;
    XBOX360_STICK *stick;
    int x, y;
    double rate;
    
    stick = xbox360gamepad_get_stick_right();
    if (stick->x/STICK_DIVISOR != stick_previous.x/STICK_DIVISOR || stick->y/STICK_DIVISOR != stick_previous.y/STICK_DIVISOR) {
        stick_previous.x = stick->x;
        stick_previous.y = stick->y;
        x = stick->x/STICK_DIVISOR; 
        y = stick->y/STICK_DIVISOR;
        
        if (x > 2 || x < -2) {
            //debug_printf("New RIGHT stick position x = %d y = %d\r\n", stick->x/STICK_DIVISOR, stick->y/STICK_DIVISOR);
            rate = 0.4 * (double)((double)stick->x/STICK_DIVISOR / 16.0);
            _nexstar_set_azmith_rate_fine(rate);
        }
        else {
            _nexstar_set_azmith_rate_fine(0.0);
        }
        
        if (y > 2 || y < -2) {
            rate = 0.4 * (double)((double)stick->y/STICK_DIVISOR / 16.0);
            _nexstar_set_elevation_rate_fine(rate);
        }
        else {
            _nexstar_set_elevation_rate_fine(0.0);
        }
    }    
}



