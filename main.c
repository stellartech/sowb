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
 
#ifndef MAIN_CPP
#define MAIN_CPP
#endif

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
#include "init.h"

#include "predict_th.h"

int test_flash_page;

int main() {
    int counter = 0;
    char test_buffer[256];
    DIR fDir;
    FILINFO fInfo;
    int f_return;
    GPS_LOCATION_RAW location;
    
    SOWBinit();
        
    /* Init the watchdog and then go into the main loop. */
    LPC_SC->PCLKSEL0 |= 0x3;
    LPC_WDT->WDCLKSEL = 1;
    LPC_WDT->WDTC = 6000000;
    LPC_WDT->WDMOD = 3;
    KICK_WATCHDOG;
    
    while(1) {
                
        char c = user_get_button(false);
        switch (c) {
            case BUTT_START_PRESS:
                if (!sdcard_is_mounted()) {
                    osd_string_xy(1, 14, "No SD card inserted");
                }
                break;
            
            case BUTT_XBOX_PRESS:
                //SAT_POS_DATA q;
                //satapi_aos(&q, true);
                break;
                    
            case BUTT_LS_PRESS:
                osd_l01_next_mode();
                break;
            case BUTT_RS_PRESS:
                osd_crosshair_toggle();
                break;    
            case BUTT_B_PRESS:
                //_nexstar_goto_azm_fast(0x238F);
                //_nexstar_goto(0x238F, 0x238F);
                _nexstar_goto(0x0, 0x0);
                break;
            case BUTT_X_PRESS:
                /*
                _nexstar_set_elevation_rate_auto(1.0);
                main_test_flag = 1;
                rit_timer_set_counter(MAIN_TEST_CB, 5000);
                P22_ASSERT;
                while (main_test_flag == 1) {
                    user_call_process();
                }
                _nexstar_set_elevation_rate_auto(0);
                P22_DEASSERT;
                */
                
                for (int i = 0; i < 256; i++) {
                    test_buffer[i] = 255 - i;
                }
                debug_printf("Test buffer before:-\r\n");
                printBuffer(test_buffer, 256);
                flash_page_write(1, test_buffer);
                LED1_ON;
                while(flash_write_in_progress());
                LED1_OFF; 
                flash_read_page(1, test_buffer, true);
                debug_printf("Test buffer after:-\r\n");
                printBuffer(test_buffer, 256);
                break;
                
            case BUTT_Y_PRESS:
                memset(test_buffer, 0xAA, 256);
                flash_read_page(0, test_buffer, true);
                debug_printf("Page 0\r\n");
                printBuffer(test_buffer, 256);
                memset(test_buffer, 0xAA, 256);
                flash_read_page(1, test_buffer, true);
                debug_printf("Page 1\r\n");
                printBuffer(test_buffer, 256);
                memset(test_buffer, 0xAA, 256);
                flash_read_page(2, test_buffer, true);
                debug_printf("Page 2\r\n");
                printBuffer(test_buffer, 256);
                memset(test_buffer, 0xAA, 256);
                flash_read_page(3, test_buffer, true);
                debug_printf("Page 3\r\n");
                printBuffer(test_buffer, 256);
                break;
            case BUTT_DPAD_DOWN_PRESS:
                flash_erase_sector(0);
                break;
            case BUTT_DPAD_UP_PRESS:
                flash_erase_bulk();
                break;
            case BUTT_DPAD_LEFT_PRESS:
                {
                    AltAz y;
                    RaDec x;
                    GPS_LOCATION_AVERAGE loc;
                    GPS_TIME t;
                    memset(&t, 0, sizeof(GPS_TIME));
                    t.year = 2010; t.month = 10; t.day = 7; t.hour = 21; t.minute = 15; t.second = 21;
                
                    double jd = gps_julian_date(&t);
                    sprintf(test_buffer, "\n\n\rJD = %f\r\n", jd);
                    debug_printf(test_buffer);
                    
                    gps_get_location_average(&loc);
                    double siderealDegrees = gps_siderealDegrees_by_time(&t);
                    sprintf(test_buffer, "SR by time = %f\r\n", siderealDegrees);
                    debug_printf(test_buffer);
                    
                    x.dec = 28.026111;
                    x.ra  = 116.32875;
                    
                    sprintf(test_buffer, "Staring with Dec = %f, RA = %f\r\n", x.dec, x.ra);
                    debug_printf(test_buffer);
                    
                    radec2altaz(siderealDegrees, &loc, &x, &y);
                    sprintf(test_buffer, "Alt = %f, Azm = %f\r\n", y.alt, y.azm);
                    debug_printf(test_buffer);
                                        
                    altaz2radec(siderealDegrees, &loc, &y, &x);
                    sprintf(test_buffer, "Dec = %f, RA = %f\r\n", x.dec, x.ra);
                    debug_printf(test_buffer);
                    
                    // Lets look for the brightest star near Sirius.
                    // It should return HR2491, Sirius it's self.
                    // RA 6 45 8.9      101.2606833
                    // Dec -16 42, 58   -16.716111
                    x.ra = 101.2606833; x.dec = -16.716111;
                    char test_buffer2[32];
                    basicStarData star, *p;
                    p = star_closest(&x, &star);
                    if (!p) {
                        debug_printf("No star found\r\n");
                    }
                    else {
                        sprintf(test_buffer, "HR%d %f %f %f\r\n", star.hr, star.ra, star.dec, star.mag);
                        debug_printf(test_buffer);
                    }
                    
                    
                }
    
    
                break;
        }
        
        //sgp4sdp4_th_init(); 
        
   /* 
        for(int i = 0; process_callbacks[i] != NULL; i++) {
            (process_callbacks[i])();
        }
        
        th_xbox360gamepad();
    */
    }    
}

