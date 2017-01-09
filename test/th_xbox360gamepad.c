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
 
#ifndef TH_XBOX360GAMEPAD_C
#define TH_XBOX360GAMEPAD_C
#endif

#include "mbed.h"
#include "usbeh.h"
#include "usbeh_endpoint.h"
#include "usbeh_device.h"
#include "usbeh_controller.h"
#include "usbeh_api.h"
#include "xbox360gamepad.h"
#include "th_xbox360gamepad.h"

#include "main.h"
#include "debug.h"


const char *button_text[] = { "","LS","RS","XBOX","Unused","A","B","X","Y","DPAD UP","DPAD DOWN","DPAD LEFT","DPAG RIGHT","START","BACK","LEFT HAT","RIGHT HAT" };

/* Define globals to hold Xbox360 stick data. */
XBOX360_STICK *stick;
XBOX360_STICK stick_left_previous;
XBOX360_STICK stick_right_previous;   
unsigned char trigger_left = 0, trigger_left_last = 0;
unsigned char trigger_right = 0, trigger_right_last = 0;

void th_xbox360gamepad_init(void) {
    stick = xbox360gamepad_get_stick_left();
    stick_left_previous.x = stick->x;
    stick_left_previous.y = stick->y;
    stick = xbox360gamepad_get_stick_right();
    stick_right_previous.x = stick->x;
    stick_right_previous.y = stick->y;    
}
        
void th_xbox360gamepad(void) {
    unsigned char button;
    if ((button = xbox360gamepad_get_button()) != 0) {
        if (button > 0) {
            debug_printf("Button ");
            if (button > (BUTT_RIGHT_HAT_PRESS + 16)) {
                debug_printf("%s held\r\n", button_text[button - 32]);
            }
            else if (button > BUTT_RIGHT_HAT_PRESS) { 
                debug_printf("%s released\r\n", button_text[button - 16]);
            }
            else {
                debug_printf("%s pressed\r\n", button_text[button]);
                switch (button) {
                    case BUTT_A_PRESS:
                        xbox360gamepad_led(LED_1_FLASH_THEN_ON);
                        break;
                    case BUTT_B_PRESS:
                        xbox360gamepad_led(LED_2_FLASH_THEN_ON);
                        break;
                    case BUTT_X_PRESS:
                        xbox360gamepad_led(LED_3_FLASH_THEN_ON);
                        break;
                    case BUTT_Y_PRESS:
                        xbox360gamepad_led(LED_4_FLASH_THEN_ON);
                        break;
                }
            }
        }
    }

    if ((trigger_left = xbox360gamepad_get_trigger_left()) != trigger_left_last) {
        debug_printf("Left trigger: %d\r\n", trigger_left);
        trigger_left_last = trigger_left;
    }

    if ((trigger_right = xbox360gamepad_get_trigger_right()) != trigger_right_last) {
        debug_printf("Right trigger: %d\r\n", trigger_right);
        trigger_right_last = trigger_right;
    }
        
    unsigned char xbox360gamepad_get_trigger_right(void);
      
    stick = xbox360gamepad_get_stick_left();
    if (stick->x/STICK_DIVISOR != stick_left_previous.x/STICK_DIVISOR || stick->y/STICK_DIVISOR != stick_left_previous.y/STICK_DIVISOR) {
        stick_left_previous.x = stick->x;
        stick_left_previous.y = stick->y;
        // Don't bother printing for now, the sticks are too sensitive!
        int x = stick->x/STICK_DIVISOR, y = stick->y/STICK_DIVISOR;
        if (1 || (x > 10 || x < -10) || (y > 10 || y < -10) ) {
            debug_printf("New LEFT stick position x = %d y = %d\r\n", stick->x/STICK_DIVISOR, stick->y/STICK_DIVISOR);
        }
    }

    stick = xbox360gamepad_get_stick_right();
    if (stick->x/STICK_DIVISOR != stick_right_previous.x/STICK_DIVISOR || stick->y/STICK_DIVISOR != stick_right_previous.y/STICK_DIVISOR) {
        stick_right_previous.x = stick->x;
        stick_right_previous.y = stick->y;
        // Don't bother printing for now, the sticks are too sensitive!
        int x = stick->x/STICK_DIVISOR, y = stick->y/STICK_DIVISOR;
        if (1 || (x > 10 || x < -10) || (y > 10 || y < -10) ) {
            debug_printf("New RIGHT stick position x = %d y = %d\r\n", stick->x/STICK_DIVISOR, stick->y/STICK_DIVISOR);
        }
    }    
}

