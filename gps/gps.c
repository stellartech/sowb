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
    Implementation notes.
    The SOWB reads GPS data in on RDX1. We don't use TDX1 as we use it as part 
    of the SSP/SPI for the MAX7456/Flash card reader. However, we are not really
    interested in writing to the GPS module as the most crucial data is the time
    rather than the location. We do use the location data but we don't need any
    WAAS data correction, 10metres is enough accuracy. So we only use Uart1 Rx to 
    get data.
    
    Additionally, the GPS 1 pulse per second signal is connected to a P29 (P0.5)
    and the gpioirq.c module routes that interrupt back to us via the _gps_pps_irq()
    callback function.    
*/ 

#include "sowb.h"
#include "rit.h"
#include "gpio.h"
#include "gps.h"
#include "math.h"
#include "debug.h"

/* Module global variables. */
double              lat_average;
double              lon_average;
double              lat_history[GPS_HISTORY_SIZE];
double              lon_history[GPS_HISTORY_SIZE];
int                 history_in_index;
int                 history_complete;
GPS_TIME            the_time; 
GPS_LOCATION_RAW    the_location;

double              lat_acc_average;
double              lon_acc_average;
double              cnt_acc_average;

/* We maintain two serial input buffers so that the
   _process() function can be processing one buffer
   while the serial interrupt system can be capturing 
   to the other. */
char uart1_buffer[2][GPS_BUFFER_SIZE];

#define UART1_BUFFER_A  0
#define UART1_BUFFER_B  1

char active_buffer, active_buffer_in;
char passive_buffer_ready;

/* Update flag to signal new data in the active buffer. */
char have_new_location;

/* Set to non-zero by the updater interrupts. */
char time_updated;
char location_updated;

/* Internal function prototypes. */
void _gps_process_rmc(char passive_buffer);
void _gps_process_gga(char passive_buffer);
void _gps_time_inc(GPS_TIME *q);
void _gps_date_inc(GPS_TIME *q);
void _gps_timer_tick_cb(int);
void _gps_pps_alive_cb(int);
void Uart1_init(void);

/** gps_process
 */
void gps_process(void) {
    int i, j;
    uint32_t ier_copy;
    char passive_buffer;
    double lat_temp, lon_temp;
    
    if (passive_buffer_ready == 1) {
        /* Disable serial interrupts on UART1 */   
        ier_copy = LPC_UART1->IER;
        LPC_UART1->IER = 0x0;
    
        /* What index is the passive buffer, i.e., what is the opposite
           of the current active buffer? */
        passive_buffer = active_buffer == 0 ? 1 : 0;
        
        if (!strncmp(uart1_buffer[passive_buffer], "$GPRMC", 6)) {
            _gps_process_rmc(passive_buffer);
        }
        else if (!strncmp(uart1_buffer[passive_buffer], "$GPGGA", 6)) {
            _gps_process_gga(passive_buffer);
        }
        
        /* Flag we have completed processing the passive buffer. */
        passive_buffer_ready = 0;
        
        /* Enable serial interrupts on UART1 */   
        LPC_UART1->IER = ier_copy;
    }
    
    /* Update the latitude/longitude moving average filter. */
    if (the_location.is_valid && have_new_location != 0) {
        have_new_location = 0;
        lat_history[history_in_index] = gps_convert_coord(the_location.lat, GPS_LAT_STR);
        lon_history[history_in_index] = gps_convert_coord(the_location.lon, GPS_LON_STR);
        history_in_index++;
        if (history_in_index >= GPS_HISTORY_SIZE) {
            history_in_index = 0;
            history_complete = 1;
        }
        j = history_complete == 1 ? GPS_HISTORY_SIZE+1 : history_in_index;
        for(lat_temp = lon_temp = 0., i = 1; i < j; i++) {            
            lat_temp += lat_history[i - 1];
            lon_temp += lon_history[i - 1];        
        }
        if (i) {
            lat_average = lat_temp / (double)(i - 1);
            lon_average = lon_temp / (double)(i - 1);
            location_updated = 1;
        }
    }  
}

/** _gps_process_rmc
 *
 * Extract the NMEA data from an RMC packet. 
 * Sample:-
 * $GPRMC,132555.639,A,5611.5374,N,00302.0325,W,000.0,129.3,020910,,,A*75
 *
 * @param char passive_buffer Which buffer holds the RMC packet data.
 */
void _gps_process_rmc(char passive_buffer) {
    char *token;
    int  token_counter = 0;
    char *time   = (char *)NULL;
    char *date   = (char *)NULL;
    char *status = (char *)NULL;
    
    token = strtok(uart1_buffer[passive_buffer], ",");
    while (token) {
        switch (token_counter) {
            case 9: date   = token; break;
            case 1: time   = token; break;
            case 2: status = token; break;
        }
        token = strtok((char *)NULL, ",");
        token_counter++;
    }
    
    if (status && date && time) {
        the_time.hour       = (char)((time[0] - '0') * 10) + (time[1] - '0');
        the_time.minute     = (char)((time[2] - '0') * 10) + (time[3] - '0');
        the_time.second     = (char)((time[4] - '0') * 10) + (time[5] - '0');
        the_time.day        = (char)((date[0] - '0') * 10) + (date[1] - '0');
        the_time.month      = (char)((date[2] - '0') * 10) + (date[3] - '0');
        the_time.year       =  (int)((date[4] - '0') * 10) + (date[5] - '0') + 2000;
        the_time.is_valid   = status[0] == 'A' ? 1 : 0;
        the_time.prev_valid = 1;
    }
    else {
        the_time.is_valid = 0;
    }
}

/** _gps_process_gga
 *
 * Extract the NMEA data from a GGA packet. 
 * Sample:-
 * $GPGGA,132526.639,5611.5417,N,00302.0298,W,1,05,7.3,43.4,M,52.0,M,,0000*70
 *
 * @param char passive_buffer Which buffer holds the GGA packet data.
 */
void _gps_process_gga(char passive_buffer) {
    char *token;
    int  token_counter = 0;
    char *latitude  = (char *)NULL;
    char *longitude = (char *)NULL;
    char *lat_dir   = (char *)NULL;
    char *lon_dir   = (char *)NULL;
    char *qual      = (char *)NULL;
    char *altitude  = (char *)NULL;
    char *sats      = (char *)NULL;
    
    token = strtok(uart1_buffer[passive_buffer], ",");
    while (token) {
        switch (token_counter) {
                case 2:  latitude  = token; break;
                case 4:  longitude = token; break;
                case 3:  lat_dir   = token; break;    
                case 5:  lon_dir   = token; break;    
                case 6:  qual      = token; break;
                case 7:  sats      = token; break;
                case 9:  altitude  = token; break;
        }
        token = strtok((char *)NULL, ",");
        token_counter++;
    }

    /* If the fix quality is valid set our location information. */
    if (latitude && longitude && altitude && sats) { 
        memcpy(the_location.lat, latitude,  10); /* Fixed length string. */
        memcpy(the_location.lon, longitude, 10); /* Fixed length string. */
        memset(the_location.alt, 0, sizeof(the_location.alt)); /* Clean string out first. */
        strncpy(the_location.alt, altitude, 10); /* Variable length string. */
        strncpy(the_location.sats, sats, 3);     /* Variable length string. */
        the_location.north_south = lat_dir[0];
        the_location.east_west   = lon_dir[0];
        the_location.is_valid    = qual[0];
        the_location.updated++;
        have_new_location = 1;
    }
    else {
        the_location.is_valid = qual[0];
    }    
}

/** gps_convert_coord
 *
 * Given a string and a type convert the string into a double.
 *
 * @param char *s A pointer to the string to convert.
 * @param int type The conversion type required (lat or lon)
 * @return double the converted string.
 */
double gps_convert_coord(char *s, int type) {
    int deg, min, sec;
    double fsec, val;
    
    if (type == GPS_LAT_STR) {
        deg  = ( (s[0] - '0') * 10) + s[1] - '0';
        min  = ( (s[2] - '0') * 10) + s[3] - '0';
        sec  = ( ((s[5] - '0') * 1000) + ((s[6] - '0') * 100) + ((s[7] - '0') * 10) + (s[8] - '0'));
        fsec = (double)((double)sec /10000.0);
        val  = (double)deg + ((double)((double)min/60.0)) + (fsec/60.0);
        return val;
    }
    else {
        deg  = ( (s[0] - '0') * 100) + ((s[1] - '0') * 10) + (s[2] - '0');
        min  = ( (s[3] - '0') * 10) + s[4] - '0';
        sec  = ( ((s[6] - '0') * 1000) + ((s[7] - '0') * 100) + ((s[8] - '0') * 10) + (s[9] - '0'));
        fsec = (double)((double)sec /10000.0);
        val  = (double)deg + ((double)((double)min/60.0)) + (fsec/60.0);
        return val;
    }
}

/** gps_init
 */
void gps_init(void) {
    int i;
    
    DEBUG_INIT_START;
    
    /* Set the time to a known starting point in the past. */
    the_time.year       = 2000;
    the_time.month      = 1;
    the_time.day        = 1;
    the_time.hour       = 0;
    the_time.minute     = 0;
    the_time.second     = 0;
    the_time.tenth      = 0;
    the_time.hundreth   = 0;
    the_time.is_valid   = 0;
    the_time.prev_valid = 0;
    
    memset(&the_location, 0, sizeof(GPS_LOCATION_RAW));
    
    /* Initial condition. */
    time_updated = 0;
    
    /* Zero out the moving average filter. */
    lat_average = 0.;
    lon_average = 0.;
    history_in_index = 0;
    history_complete = 0;
    for(i = 0; i < GPS_HISTORY_SIZE; i++) {
        lat_history[i] = 0.;
        lon_history[i] = 0.;
    }

    lat_acc_average = 0.;
    lon_acc_average = 0.;
    cnt_acc_average = 0.;

    /* Init the active buffer and the serial in pointer. */
    active_buffer = active_buffer_in = 0;
    
    /* Flag the passive buffer is not ready. The passive buffer
       is the opposite of the active buffer. When the serial irq
       system detects the end of a NEMA message it automatically 
       switches buffers and sets passive_buffer_ready non-zero to
       indicate it's just finished dumping data into it. */
    passive_buffer_ready = 0;
    
    /* Updated to non-zero after a new location packet is received. */
    have_new_location = 0;
    
    /* Setup the 0.01second timer. */
    rit_timer_set_reload(RIT_TIMER_CB_GPS, 10);  /* Recurring reload. */
    rit_timer_set_counter(RIT_TIMER_CB_GPS, 10); /* Start timer. */
    
    DEBUG_INIT_END;
    
    Uart1_init();    
}

/** gps_get_time
 * 
 * Copies our internal time data structure to a buffer supplied by the caller.
 *
 * Note, the update flag is set to non-zero by the interrupt routines when an
 * update occurs. The loop is to test if an update occured while the copy was
 * in progress. If it did, do the copy again as it's most likely corrupted the
 * orginal copy operation.
 *
 * @param GPS_TIME *q A pointer to the GPS_TIME data structure to copy to.
 * @return GPS_TIME * The supplied pointer.
 */
GPS_TIME *gps_get_time(GPS_TIME *q) {

    do {
        time_updated = 0;
        memcpy(q, &the_time, sizeof(GPS_TIME));
    } while (time_updated != 0);
    
    return q;
}

/** gps_get_location_raw
 * 
 * Copies our internal location data structure to a buffer supplied by the caller.
 *
 * Note, the update flag is set to non-zero by the interrupt routines when an
 * update occurs. The loop is to test if an update occured while the copy was
 * in progress. If it did, do the copy again as it's most likely corrupted the
 * orginal copy operation.
 *
 * @param GPS_LOCATION_RAW *q A pointer to the GPS_LOCATION_RAW data structure to copy to.
 * @return GPS_LOCATION_RAW * The supplied pointer.
 */
GPS_LOCATION_RAW *gps_get_location_raw(GPS_LOCATION_RAW *q) {

    do {
        location_updated = 0;
        memcpy(q, &the_location, sizeof(GPS_LOCATION_RAW));
    } while (location_updated != 0);
    
    return q;
}

/** gps_get_location_average
 *
 * Places the current average location into the supplied struct buffer.
 * The caller is responsible for allocating the buffer storage space.
 *
 * Note, the update flag is set to non-zero by the interrupt routines when an
 * update occurs. The loop is to test if an update occured while the copy was
 * in progress. If it did, do the copy again as it's most likely corrupted the
 * orginal copy operation.
 *
 * @param GPS_LOCATION_AVERAGE *q A pointer to the struct buffer to write data to.
 * @return GPS_LOCATION_AVERAGE *  The supplied pointer returned.
 */
GPS_LOCATION_AVERAGE *gps_get_location_average(GPS_LOCATION_AVERAGE *q) {
    char **p = NULL;

    do {
        location_updated = 0;
        q->north_south  = the_location.north_south;
        q->latitude     = lat_average;
        q->east_west    = the_location.east_west;
        q->longitude    = lon_average;
        q->height       = strtod(the_location.alt, p); 
        q->sats         = the_location.sats;   
        q->is_valid     = the_location.is_valid;
    } while (location_updated != 0);

    /* Test the values to ensure the data is valid. */
    if (isnan(q->latitude) || isnan(q->longitude) || isnan(q->height)) {
        q->is_valid = 0;
    }
    
    return q;
}

/** gps_julian_day_number
 *
 * Gets the Julian Day Number from the supplied time reference passed.
 * http://en.wikipedia.org/wiki/Julian_day#Converting_Gregorian_calendar_date_to_Julian_Day_Number
 *
 * @param GPS_TIME *t A pointer to a time data structure.
 * @return double The Julian Day Number.
 */
double gps_julian_day_number(GPS_TIME *t) {
    double wikipedia_jdn = (double)(1461 * ((int)t->year + 4800 + ((int)t->month - 14) / 12)) / 4 + (367 * ((int)t->month - 2 - 12 * (((int)t->month - 14) / 12))) / 12 - (3 * (((int)t->year + 4900 + ((int)t->month - 14) / 12 ) / 100)) / 4 + (int)t->day - 32075;
    
    /* Not sure why yet but the calculation on the Wikipedia site returns a value that
       is 0.5 too big. */
    return wikipedia_jdn;
}

/** gps_julian_date
 * 
 * Find the Julian Date based on the supplied args.
 *
 * @param GPS_TIME *t A pointer to a time data structure.
 * @return double The Julian Date.
 */
double gps_julian_date(GPS_TIME *t) {
    double hour, minute, second, jd;
    hour   = (double)t->hour;
    minute = (double)t->minute;
    second = (double)t->second + ((double)t->tenth / 10.) + ((double)t->hundreth / 100.);
                                   /* Wiki fix, see above. */  
    jd = gps_julian_day_number(t) -           0.5 +
         ((hour - 12.) / 24.) +
         (minute / 1440.) + 
         (second / 86400.);        
        
    return jd;
}

/** gps_siderealDegrees_by_jd
 *
 * Calculate the sidereal degree angle based on the 
 * Julian Date supplied.
 *
 * @param double jd Julian Date.
 * @return The sidereal angle in degrees.
 */
double gps_siderealDegrees_by_jd(double jd) {
    double sidereal, gmst, lmst, mul;
    double T  = jd - 2451545.0;
    double T1 = T / 36525.0;
    double T2 = T1 * T1;
    double T3 = T2 * T1;
     
    /* Calculate gmst angle. */
    sidereal = 280.46061837 + (360.98564736629 * T) + (0.000387933 * T2) - (T3 / 38710000.0);
     
    /* Convert to degrees. */
    sidereal = fmod(sidereal, 360.0);
    if (sidereal < 0.0) sidereal += 360.0;
 
    mul = (the_location.east_west == 'W') ? -1.0 : 1.0;
    
    gmst = sidereal;
    lmst = gmst + (lon_average * mul);
    return lmst;
}

/** gps_siderealDegrees_by_time
 *
 * Calculate the sidereal degree angle based on the 
 * time data structure supplied.
 *
 * @param GPS_TIME *t A pointer to the time structure.
 * @return The sidereal angle in degrees.
 */
double gps_siderealDegrees_by_time(GPS_TIME *t) {
    GPS_TIME temp;
    if (t == (GPS_TIME *)NULL) {
        t = &temp;
        gps_get_time(t);        
    }
    return gps_siderealDegrees_by_jd(gps_julian_date(t));
}

/** gps_siderealHA_by_jd
 *
 * Calculate the HA (hour angle) based on the supplied Julian Date.
 *
 * @param double jd The Julian Date.
 * @return double The Hour Angle.
 */
double gps_siderealHA_by_jd(double jd) {
    double lmst = gps_siderealDegrees_by_jd(jd);
    return lmst / 360.0 * 24.0;
}

/** gps_siderealHA_by_time
 *
 * Calculate the HA (hour angle) based on the supplied time data structure.
 *
 * @param GPS_TIME *t The time data structure.
 * @return double The Hour Angle.
 */
double gps_siderealHA_by_time(GPS_TIME *t) {
    double lmst = gps_siderealDegrees_by_time(t);
    return lmst / 360.0 * 24.0;
}

/** _gps_inc_time
 *
 * Used to increment the time structure by 0.01sec.
 * Called by the RIT timer callback.
 *
 * @see gpioirq.c
 * @param int t_index The timer's index number (handle).
 */
void _gps_timer_tick_cb(int t_index) {
    
    /* We use x so that the_time.hundreth can never be 10 as I have noticed
       occasionally between the ++ and the == 10 test an interrupt can occur
       and then the_time.hundreth contains an invalid value. So using x to do
       the ++ and ==10 test means the_time.hundreth can never itself be 10. 
       We reuse x on the tenths for a similar reason. */
    char x = the_time.hundreth;
    x++;
    
    if (x == 10) {
        the_time.hundreth = 0;
        x = the_time.tenth + 1;
        if (x < 10) {  
            the_time.tenth = x;
            time_updated = 1;
        }
    }
    else {
        the_time.hundreth = x;
        time_updated = 1;
    }
}

/** _gps_pps_alive_cb
 *
 * Timeout that goes off if the GPS 1PPS signal doesn't
 * fire within 1.5seconds telling us that the GPS isn't
 * connected. Not currently used.
 */
void _gps_pps_alive_cb(int index) {
    the_time.is_valid = 0;
}
 
/** gps_pps_fall
 *
 * Increments the seconds. Called by the GPS 1PP callback interrupt.
 *
 * Note, some GPS modules, including the one used in this design, 
 * provide a 1PPS signal. However, it's almost always positive logic
 * and it doesn't interface directly to an Mbed pin/interrupt. So we 
 * have a simple FET that buffers the signal and in so doing it becomes
 * an active low signal. Hence why this is a falling edge interrupt.
 *
 * @see gpioirq.c
 */
void gps_pps_fall(void) {
    the_time.hundreth = 0;
    the_time.tenth    = 0;
    _gps_time_inc(&the_time);           
}

/** gps_date_inc
 *
 * Increment the time.
 *
 * @param GPS_TIME *q Pointer the data struct holding the time.
 */
void _gps_time_inc(GPS_TIME *q) {
    
    time_updated = 1;
    
    q->second++;
    if (q->second == 60) {
        q->second = 0;
        q->minute++;
        if (q->minute == 60) {
            q->minute = 0;
            q->hour++;
            if (q->hour == 24) {
                q->hour = 0;
                _gps_date_inc(q);
            }
        }
    }
}

/** _gps_date_inc
 *
 * Increment the date.
 *
 * @param GPS_TIME *q Pointer the data struct holding the time.
 */
void _gps_date_inc(GPS_TIME *q) {
    const int days[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 } ;

    /* Handle February leap year. */    
    int leap_year = ((the_time.year % 4 == 0 && the_time.year % 100 != 0) || the_time.year % 400 == 0) ? 1 : 0;
    int days_this_month = days[q->month - 1];
    if (q->month == 2 && leap_year) days_this_month++;
    
    q->day++;
    if (q->day > days_this_month) {
        q->day = 1;
        q->month++;
        if (q->month == 13) {
            q->year++;
        }
    }
}

/* UART1 functions. */

extern "C" void UART1_IRQHandler(void) __irq {
    volatile uint32_t iir;
    volatile char c;
    
    iir = LPC_UART1->IIR;
    
    if (iir & 0x1) return;
    
    /* Do we have a serial character(s) in the fifo? */
    if (UART_RX_INTERRUPT) {
        while (UART1_FIFO_NOT_EMPTY) {
            c = UART1_GETC;
            uart1_buffer[active_buffer][active_buffer_in++] = c;
            active_buffer_in &= (GPS_BUFFER_SIZE - 1);
            if (c == '\n') {
                /* Swap buffers and clean it out. */
                active_buffer = active_buffer == 0 ? 1 : 0;
                memset(uart1_buffer[active_buffer], 0, GPS_BUFFER_SIZE);
                active_buffer_in = 0;
                passive_buffer_ready = 1;
            }
        } 
    }
}

/** Uart1_init
 */
void Uart1_init(void) {
    
    DEBUG_INIT_START;
    
    LPC_SC->PCONP       |=  (1UL << 4);
    LPC_SC->PCLKSEL0    &= ~(3UL << 8);
    LPC_SC->PCLKSEL0    |=  (1UL << 8);
    LPC_PINCON->PINSEL4 &= ~(3UL << 2); /* TXD1 not used. See SSP0_init() in max7456.c */
    LPC_PINCON->PINSEL4 |=  (2UL << 2); /* TXD1 not used. See SSP0_init() in max7456.c */
    LPC_UART1->LCR       = 0x83;
    LPC_UART1->DLL       = 0x71;
    LPC_UART1->DLM       = 0x02;
    LPC_UART1->LCR       = 0x03;
    LPC_UART1->FCR       = 0x07;
    
    NVIC_SetVector(UART1_IRQn, (uint32_t)UART1_IRQHandler);
    NVIC_EnableIRQ(UART1_IRQn);    
    
    /* Enable the RDA interrupt. */
    LPC_UART1->IER = 0x01; 
    
    DEBUG_INIT_END;
}

