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
#include "gpioirq.h"
#include "gpio.h"
#include "rit.h"
#include "usbeh.h"
#include "usbeh_endpoint.h"
#include "usbeh_device.h"
#include "usbeh_controller.h"
#include "usbeh_api.h"
#include "xbox360gamepad.h"
#include "th_xbox360gamepad.h"
#include "gps.h"
#include "MAX7456.h"
#include "osd.h"
#include "nexstar.h"
#include "utils.h"
#include "user.h"
#include "dma.h"
#include "flash.h"
#include "sdcard.h"
#include "config.h"
#include "ff.h"
#include "sgp4sdp4.h"
#include "satapi.h"
#include "star.h"

#include "main.h"
#include "debug.h"
#include "predict_th.h"

/* Create an array of _process function pointers
   to call while user_io is waiting. */
typedef void (PROCESS_FUNC)();

PROCESS_FUNC *process_callbacks[] = {
    usbeh_api_process,
    xbox360gamepad_process,
    gps_process,
    gpioirq_process,
    nexstar_process,
    sdcard_process,
    config_process,
    NULL
};

void SOWBinit(void) {
    GPS_LOCATION_RAW location;
    
    /* Carry out module start-up _init() functions. 
       Note, the order is important, do not change. */
    debug_init();
    gpio_init();
    rit_init();
    xbox360gamepad_init();
    usbeh_api_init();
    MAX7456_init();
    osd_init();
    gps_init();
    gpioirq_init();
    
    /* We use raw MAX7456 calls to display the splash screen because
       at this point not all interrupts are active and the OSD system
       will not yet be fully operational even though we've _init() it. */
    MAX7456_cursor(0, 5);  MAX7456_string((unsigned char *)"     Satellite Observers  ");
    MAX7456_cursor(0, 6);  MAX7456_string((unsigned char *)"       Workbench V0.1     ");
    MAX7456_cursor(0, 7);  MAX7456_string((unsigned char *)"     (c) Copyright 2010   ");
    MAX7456_cursor(0, 8);  MAX7456_string((unsigned char *)"  Stellar Technologies Ltd");
    if (LPC_WDT->WDMOD & 0x4) { MAX7456_cursor(0, 14);  MAX7456_string((unsigned char *)" WDT Error detected"); }
    user_wait_ms_blocking(2000); /* Simple splash screen delay. */
    MAX7456_cursor(0, 11);  MAX7456_string((unsigned char *)"    Press A to continue");
    //while (user_get_button(false) != BUTT_A_PRESS)   ;
    //while (user_get_button(false) != BUTT_A_RELEASE) ;
    
    /* Complete the module _init() stage. */
    nexstar_init();
    DMA_init();
    flash_init();
    sdcard_init();
    config_init();
    th_xbox360gamepad_init();
    
    if (!_nexstar_is_aligned()) {
        debug_printf("Nexstar not aligned, forcing user to align.\r\n");
        nexstar_force_align();
    }
    
    MAX7456_cursor(0, 11);  MAX7456_string((unsigned char *)"    Waiting for GPS....");
    do { 
        gps_get_location_raw(&location);
        WHILE_WAITING_DO_PROCESS_FUNCTIONS; 
    } while (location.is_valid == '0');
    osd_clear(); osd_set_mode_l01(L01_MODE_A);
    
    /* Tell the Nexstar the real time and place. */
    _nexstar_set_time(NULL);
    _nexstar_set_location(NULL);

}