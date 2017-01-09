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

/*
   Update: This whole module requires refactoring into something much more
   sane. Basically, I "organically" grew the code in this module with trial
   and error using the hardware since getting simple things done proved hard.
   I did design (see below) what I thought reasonable system but the Nexstar
   kept throwing up problems. So I really need to come back to this code and
   sort it out properly.
   
   Frustrations: I originally implemented an "intelligent" start-up routine
   that sent echo requests to check the Nexstar was there, then requested 
   isAligned and, once aligned, would finally send the Tracking Off command
   and enter operational state. However, that's all out the window.
   
   Basically, I have found that if you send anything down the serial cable 
   while the Nexstar is powering up it goes dumb and won't responsd to 
   anything you send it. How useless, someone needs to educate their 
   software engineers to designing robust serial protocols that can 
   handle battery/power failure. Really very silly because I now know
   that my software MUST assume the end user has powered on their Nexstar
   before I start sending serial commands! Rubbish design, I like smart
   designs that don't rely on the end user having to do "special things",
   they should just work!
   
   Additionally, I have found that, based on documents from TheSky program
   you cannot ask the Nexstar for it's pointing position faster than once
   per second! WTF? Asking too fast can crash the Nexstar hand controller.
   Doh! Since we really need the position at a higher frequency then at the
   subsecond level, the pointing position is calculated from the last position
   known +/- whatever variable speed rates have been sent since then.
   
   For the purposes of trying to track a fast moving object (an artificial
   satellite in this case) we end up having to do most of the work ourselves
   and hope the Nexstar tracks it with a sub-standard pointing system.
   Hopefully I'll get it right!
   
   Means the serial protocol I develop here is more basic than I would have 
   prefered. For example, rather than sending periodic echo requests to 
   detect the Nexstar, I'll send just one. If no reply I'll have to prompt
   the user to "Power on the Nexstar and press a key to continue". Doh.
*/

#define NEXSTAR_C

#include "sowb.h"
#include "nexstar.h"
#include "utils.h"
#include "rit.h"
#include "user.h"
#include "osd.h"
#include "debug.h"
#include "gpio.h"
#include "main.h"

/* Module global variables. */
int  nexstar_status;
int  nexstar_command;
int  nexstar_command_status;
char rx_buffer[NEXSTAR_BUFFER_SIZE];
int  rx_buffer_in;

bool nexstar_aligned;

double last_elevation;
double last_azmith;
double virtual_elevation;
double virtual_azmith;

double elevation_rate;
double elevation_rate_coarse;
double elevation_rate_fine;
double elevation_rate_auto;

double azmith_rate;
double azmith_rate_coarse;
double azmith_rate_fine;
double azmith_rate_auto;

bool nexstar_goto_in_progress;
char nexstar_goto_elevation[5];
char nexstar_goto_azmith[5];

int virtual_update_counter;

/* Local function prototypes. */
static void Uart2_init(void);
static inline void Uart2_putc(char c);
static inline void Uart2_puts(char *s, int len);

/** nexstar_process
 *
 * Our system _process function.
 */
void nexstar_process(void) {
        
    if (nexstar_command_status != 0) {
        switch (nexstar_command) {
            case NEXSTAR_IS_ALIGNED:
                nexstar_aligned = rx_buffer[0] == 1 ? true : false;
                nexstar_command = 0;
                nexstar_command_status = 0;
                break;
            case NEXSTAR_GET_AZMALT:
                last_azmith    = virtual_azmith    = 360.0 * (((double)hex2bin(&rx_buffer[0], 4)) / 65536.0);                
                last_elevation = virtual_elevation = 360.0 * (((double)hex2bin(&rx_buffer[5], 4)) / 65536.0);
                nexstar_command = 0;
                nexstar_command_status = 0;
                virtual_update_counter = 0;    
                if (nexstar_goto_in_progress) {
                    if (!memcmp(&rx_buffer[0], nexstar_goto_azmith, 4) && !memcmp(&rx_buffer[5], nexstar_goto_elevation, 4)) {
                        nexstar_goto_in_progress = false;
                        osd_clear_line(2);
                        osd_clear_line(3);
                        _nexstar_set_tracking_mode(0);
                    }
                }            
                break;
            case NEXSTAR_GET_RADEC:
            
                break;
            case NEXSTAR_SET_ELEVATION_RATE:
            case NEXSTAR_SET_AZMITH_RATE:
                nexstar_command = 0;
                nexstar_command_status = 0;
                break;
            case NEXSTAR_GOTO:
                nexstar_command = 0;
                nexstar_command_status = 0;
                break;
            case NEXSTAR_GOTO_AZM_FAST:
                nexstar_command = 0;
                nexstar_command_status = 0;
                break;
            case NEXSTAR_SET_APPROACH:
                nexstar_command = 0;
                nexstar_command_status = 0;
                break;
            case NEXSTAR_SET_TIME:
                nexstar_command = 0;
                nexstar_command_status = 0;
                break;
            case NEXSTAR_SET_LOCATION:
                nexstar_command = 0;
                nexstar_command_status = 0;
                break;    
            case NEXSTAR_SYNC:
                nexstar_command = 0;
                nexstar_command_status = 0;
                break;    
        }
    }    
}

/** nexstar_timeout_callback
 *
 * The generic timer callback handler.
 *
 * @param int index The timer index (handle).
 */
void _nexstar_timeout_callback(int index) {
    nexstar_status = NEXSTAR_STATE_NOT_CONN;
}

void _nexstar_one_second_timer(int index) {
    _nexstar_get_altazm();
    //_nexstar_get_radec();
    rit_timer_set_counter(RIT_ONESEC_NEXSTAR, 200);
}

double nexstar_get_rate_azm(void) {
    if (nexstar_goto_in_progress) return 0.0;
    return azmith_rate_coarse + azmith_rate_fine + azmith_rate_auto;
}

double nexstar_get_rate_alt(void) {
    if (nexstar_goto_in_progress) return 0.0;
    return elevation_rate_coarse + elevation_rate_fine + elevation_rate_auto;
}

void _nexstar_100th_timer(int index) {
    rit_timer_set_counter(RIT_100TH_NEXSTAR, 100);
}

/** nexstar_init
 *
 * Used to initialise the system.
 */
void nexstar_init(void) {

    DEBUG_INIT_START;
    
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, 0);
    rit_timer_set_counter(RIT_ONESEC_NEXSTAR, 250);
    rit_timer_set_counter(RIT_100TH_NEXSTAR, 100);
    nexstar_status = NEXSTAR_STATE_IDLE;
    nexstar_command = 0;
    nexstar_command_status = 0;
    last_elevation = 0.0;
    last_azmith = 0.0;
    virtual_elevation = 0.0;
    virtual_azmith = 0.0;
    
    elevation_rate = elevation_rate_coarse = elevation_rate_fine = elevation_rate_auto = 0.0;
    azmith_rate = azmith_rate_coarse = azmith_rate_fine = azmith_rate_auto = 0.0;
    
    nexstar_goto_in_progress = false;
    
    nexstar_aligned = false;
    
    rx_buffer_in = 0;
    virtual_update_counter = 0;
    
    DEBUG_INIT_END;
    
    Uart2_init();
}

/* External API functions. */
void nexstar_get_elazm(double *el, double *azm) {
    *(el)  = last_elevation;
    *(azm) = last_azmith;
}

/* Internal API functions. */
int _nexstar_set_tracking_mode(int mode) {
    if (nexstar_status == NEXSTAR_STATE_IDLE) {
        nexstar_status = NEXSTAR_STATE_BUSY;
        rx_buffer_in = 0;
        nexstar_command_status = 0;
        nexstar_command = NEXSTAR_SET_TRACKING;
        LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
        Uart2_putc('T');
        Uart2_putc((char)mode);
        rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);
        return 1;
    }
    return 0;
}

int _nexstar_get_altazm(void) {
    if (nexstar_status == NEXSTAR_STATE_IDLE) {
        nexstar_status = NEXSTAR_STATE_BUSY;
        rx_buffer_in = 0;
        nexstar_command_status = 0;
        nexstar_command = NEXSTAR_GET_AZMALT;
        LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
        Uart2_putc('Z');
        rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);
        return 1;
    }
    return 0;
}

int _nexstar_get_radec(void) {
    if (nexstar_status == NEXSTAR_STATE_IDLE) {
        nexstar_status = NEXSTAR_STATE_BUSY;
        rx_buffer_in = 0;
        nexstar_command_status = 0;
        nexstar_command = NEXSTAR_GET_RADEC;
        LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
        Uart2_putc('E');
        rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);
        return 1;
    }
    return 0;
}

void _nexstar_set_elevation_rate_coarse(double rate) {
    elevation_rate_coarse = rate;
    _nexstar_set_elevation_rate();
}

void _nexstar_set_elevation_rate_fine(double rate) {
    elevation_rate_fine = rate;
    _nexstar_set_elevation_rate();
}

void _nexstar_set_elevation_rate_auto(double rate) {
    elevation_rate_auto = rate;
    _nexstar_set_elevation_rate();
}

int _nexstar_set_elevation_rate(void) {
    char dir, high, low, cmd[32];
    double rate;

    rate = elevation_rate_coarse + elevation_rate_fine + elevation_rate_auto;
    
    if (elevation_rate == rate) return 0;
    
    if (rate != 0.0) {
        if (nexstar_goto_in_progress || nexstar_status != NEXSTAR_STATE_IDLE) return 0;
    }
        
    while (nexstar_status != NEXSTAR_STATE_IDLE) {
        if (nexstar_status == NEXSTAR_STATE_NOT_CONN) return 0;
        user_wait_ms(1);
    };
    
    //if (rate >= 0.0) osd_string_xyl(0, 14, cmd, sprintf(cmd, " ALT >> %c%.1f", '+', rate)); 
    //else             osd_string_xyl(0, 14, cmd, sprintf(cmd, " ALT >> %+.1f", rate)); 
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    nexstar_command_status = 0;
    nexstar_command = NEXSTAR_SET_ELEVATION_RATE;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    dir = 6;
    if (rate < 0.0) {
        dir = 7;
        rate *= -1;
    }
    high = ((int)(3600.0 * rate * 4.0) / 256) & 0xFF;
    low  = ((int)(3600.0 * rate * 4.0) % 256) & 0xFF;            
    Uart2_puts(cmd, sprintf(cmd, "P%c%c%c%c%c%c%c", 3, 17, dir, high, low, 0, 0));
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);
    elevation_rate = rate;
    return 1;    
}

void _nexstar_set_azmith_rate_coarse(double rate) {
    azmith_rate_coarse = rate;
    _nexstar_set_azmith_rate();
}

void _nexstar_set_azmith_rate_fine(double rate) {
    azmith_rate_fine = rate;
    _nexstar_set_azmith_rate();
}

void _nexstar_set_azmith_rate_auto(double rate) {
    azmith_rate_auto = rate;
    _nexstar_set_azmith_rate();
}

int _nexstar_set_azmith_rate(void) {
    char dir, high, low, cmd[32];
    double rate;
    
    rate = azmith_rate_coarse + azmith_rate_fine + azmith_rate_auto;
    
    if (azmith_rate == rate)  return 0;
    
    if (rate != 0.0) {
        if (nexstar_goto_in_progress || nexstar_status != NEXSTAR_STATE_IDLE) return 0;
    }
    
    while (nexstar_status != NEXSTAR_STATE_IDLE) {
        if (nexstar_status == NEXSTAR_STATE_NOT_CONN) return 0;
        user_wait_ms(1);
    };
        
    //if (rate >= 0.0) osd_string_xyl(14, 14, cmd, sprintf(cmd, " AZM >> %c%.1f", '+', rate)); 
    //else             osd_string_xyl(14, 14, cmd, sprintf(cmd, " AZM >> %+.1f", rate)); 
    nexstar_status = NEXSTAR_STATE_BUSY;
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    nexstar_command_status = 0;
    nexstar_command = NEXSTAR_SET_AZMITH_RATE;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    dir = 6;
    if (rate < 0.0) {
        dir = 7;
        rate *= -1;
    }
    high = ((int)(3600.0 * rate * 4.0) / 256) & 0xFF;
    low  = ((int)(3600.0 * rate * 4.0) % 256) & 0xFF;
    Uart2_puts(cmd, sprintf(cmd, "P%c%c%c%c%c%c%c", 3, 16, dir, high, low, 0, 0));            
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);
    azmith_rate = rate;
    return 1;    
}

int _nexstar_goto(uint32_t elevation, uint32_t azmith) {
    char cmd[32], buf1[6], buf2[6];
    double azm_target, azm_current;

    if (nexstar_goto_in_progress) return 0;
    
    printDouble_3_2(buf1, 360.0 * ((double)elevation / 65536));
    osd_stringl(2, cmd, sprintf(cmd, "     GOTO > %s < ALT", buf1)); 
    printDouble_3_2(buf2, 360.0 * ((double)azmith / 65536));
    osd_stringl(3, cmd, sprintf(cmd, "          > %s < AZM", buf2));
    osd_clear_line(14); 
       
    /* Adjust the GOTO approach based on where we are pointing now
       comapred to where we want to go. */
    azm_target = 360.0 * ((double)((double)azmith / 65536.0));
    azm_current = last_azmith - azm_target;

    if (azm_current > 180.) _nexstar_set_azm_approach(1);        
    else                   _nexstar_set_azm_approach(-1);            

    while(nexstar_status != NEXSTAR_STATE_IDLE) {
        if(nexstar_status == NEXSTAR_STATE_NOT_CONN) return 0;
        user_wait_ms(1);
    };
            
    nexstar_goto_in_progress = true;
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    nexstar_command_status = 0;
    nexstar_command = NEXSTAR_GOTO;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    sprintf(nexstar_goto_azmith, "%04X", azmith);
    sprintf(nexstar_goto_elevation, "%04X", elevation);
    Uart2_puts(cmd, sprintf(cmd, "B%s,%s", nexstar_goto_azmith, nexstar_goto_elevation));
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, 60000);
    return 1;    
}

int _nexstar_goto_azm_fast(uint32_t azmith) {
    char cmd[16];
    
    while(nexstar_status != NEXSTAR_STATE_IDLE) {
        if(nexstar_status == NEXSTAR_STATE_NOT_CONN) return 0;
        user_wait_ms(1);
    };
            
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    nexstar_command_status = 0;
    nexstar_command = NEXSTAR_GOTO_AZM_FAST;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    Uart2_puts(cmd, sprintf(cmd, "P%c%c%c%c%c%c%c", 3, 16, 2, (azmith >> 8) & 0xFF, azmith & 0xFF, 0, 0));            
    
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, 60000);
    return 1;    
}

void _nexstar_set_azm_approach(int approach) {
    char cmd[16];
    
    while(nexstar_status != NEXSTAR_STATE_IDLE) {
        if(nexstar_status == NEXSTAR_STATE_NOT_CONN) return;
        user_wait_ms(1);
    };
    
    nexstar_command = NEXSTAR_SET_APPROACH;    
    nexstar_command_status = 0;
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    Uart2_puts(cmd, sprintf(cmd, "P%c%c%c%c%c%c%c", 3, 16, 0xFD, approach < 0 ? 1 : 0, 0, 0, 0));
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);
}

bool _nexstar_is_aligned(void) {
    
    while(nexstar_status != NEXSTAR_STATE_IDLE) {
        if(nexstar_status == NEXSTAR_STATE_NOT_CONN) return false;
        user_wait_ms(1);
    };
    
    nexstar_command = NEXSTAR_IS_ALIGNED;    
    nexstar_command_status = 0;
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    Uart2_putc('J');
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);
    
    /* This command blocks while we wait for an answer. */
    while(nexstar_status != NEXSTAR_STATE_IDLE) {
        user_wait_ms(10);
    };
    
    return nexstar_aligned;
}

void _nexstar_sync(RaDec *radec) {
    char cmd[32];
    uint16_t ra, dec;
    
    while(nexstar_status != NEXSTAR_STATE_IDLE) {
        user_wait_ms(1);
    };
    
    ra  = (uint16_t)((radec->ra  / 360.0) * 65536);
    dec = (uint16_t)((radec->dec / 360.0) * 65536);
    
    nexstar_command = NEXSTAR_SYNC;    
    nexstar_command_status = 0;
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    Uart2_puts(cmd, sprintf(cmd, "S%02X,%02X", ra, dec));
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);   

}

void _nexstar_set_time(GPS_TIME *t) {
    char cmd[32];
    GPS_TIME rt;
    
    while(nexstar_status != NEXSTAR_STATE_IDLE) {
        user_wait_ms(1);
    };
    
    if (t == (GPS_TIME *)NULL) {
        t = &rt;
        gps_get_time(t);        
    }
    
    nexstar_command = NEXSTAR_SET_TIME;    
    nexstar_command_status = 0;
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    Uart2_puts(cmd, sprintf(cmd, "H%c%c%c%c%c%c%c%c", t->hour, t->minute, t->second, t->month, t->day, t->year - 2000, 0, 0));
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);   
}

void _nexstar_set_location(GPS_LOCATION_AVERAGE *l) {
    char cmd[32];
    GPS_LOCATION_AVERAGE rl;
    double d, t;
    char lat_degrees, lat_minutes, lat_seconds, lon_degrees, lon_minutes, lon_seconds;
    
    while(nexstar_status != NEXSTAR_STATE_IDLE) {
        user_wait_ms(1);
    };
    
    if (l == (GPS_LOCATION_AVERAGE *)NULL) {
        l = &rl;
        gps_get_location_average(l);        
    }
    
    d = l->latitude;
    lat_degrees = (char)d; t = (d - (double)lat_degrees) * 60.0;
    lat_minutes = (char)t;
    lat_seconds = (t - (double)lat_minutes) * 60.0;
    d = l->longitude;
    lon_degrees = (int)d; t = (d - (double)lon_degrees) * 60.0;
    lon_minutes = (int)t;
    lon_seconds = (t - (double)lon_minutes) * 60.0;
    
    nexstar_command = NEXSTAR_SET_LOCATION;    
    nexstar_command_status = 0;
    nexstar_status = NEXSTAR_STATE_BUSY;
    rx_buffer_in = 0;
    LPC_UART2->FCR = UART2_SET_FCR_CLEAR;
    Uart2_puts(cmd, sprintf(cmd, "W%c%c%c%c%c%c%c%c", lat_degrees, lat_minutes, lat_seconds, l->north_south == 'N' ? 0 : 1, lon_degrees, lon_minutes, lon_seconds, l->east_west == 'E' ? 0 : 1));
    rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);   
}

/** UART2_IRQHandler(void)
 *
 * The interrupt service routine for the UART2.
 */
extern "C" void UART2_IRQHandler(void) __irq {
    volatile uint32_t iir; 
    char c;

    /* Get the interrupt identification register which also resets IIR. */
    iir = LPC_UART2->IIR;
       
    if (iir & 0x1) return;
    
    iir = (iir >> 1) & 0x3;
    
    /* Handle the incoming character. */
    if (iir == 2) {
        c = LPC_UART2->RBR;
        //Uart0_putc(c);
        rit_timer_set_counter(RIT_TIMER_NEXSTAR, NEXSTAR_SERIAL_TIMEOUT);
        rx_buffer[rx_buffer_in] = c;
        rx_buffer_in++;
        if (rx_buffer_in >= NEXSTAR_BUFFER_SIZE) {
            rx_buffer_in = 0;
        }
        if (c == '#') {
            rit_timer_set_counter(RIT_TIMER_NEXSTAR, 0);
            nexstar_command_status = 1;
            nexstar_status = NEXSTAR_STATE_IDLE;
        }
    }
}

/** Uart2_init
 *
 * Initialise UART2 to our requirements for Nexstar.
 */
static void Uart2_init (void) {
    volatile char c __attribute__((unused));
    
    DEBUG_INIT_START;
    
    LPC_SC->PCONP       |=  (1UL << 24);
    LPC_SC->PCLKSEL1    &= ~(3UL << 16);
    LPC_SC->PCLKSEL1    |=  (1UL << 16);
    LPC_PINCON->PINSEL0 |= ((1UL << 20) | (1UL << 22));
    LPC_UART2->LCR       = 0x80;
    LPC_UART2->DLM       = 0x2;  // 115200 baud, for 9600 use 0x2;
    LPC_UART2->DLL       = 0x71; // 115200 baud, for 9600 use 0x71;
    LPC_UART2->LCR       = 0x3;
    LPC_UART2->FCR       = 0x1;

    /* Ensure the FIFO is empty. */
    while (LPC_UART2->LSR & 0x1) c = (char)LPC_UART2->RBR;
    
    NVIC_SetVector(UART2_IRQn, (uint32_t)UART2_IRQHandler);
    NVIC_EnableIRQ(UART2_IRQn);
    
    /* Enable UART0 RX interrupt only. */
    LPC_UART2->IER = 0x1; 
    
    DEBUG_INIT_END;  
}

static inline void Uart2_putc(char c) {
    //Uart0_putc(c);
    LPC_UART2->THR = (uint32_t)(c & 0xFF);
}

static inline void Uart2_puts(char *s, int len) {
    int i;
    if (len > 0) for (i = 0; len; i++, len--) Uart2_putc(s[i]);
}

