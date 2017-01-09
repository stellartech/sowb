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

#ifndef GPS_H
#define GPS_H

#define GPS_LAT_STR 0
#define GPS_LON_STR 1

#define GPS_HISTORY_SIZE  60

typedef struct _gps_time {
    int     year;
    char    month;
    char    day;
    char    hour;
    char    minute;
    char    second;
    char    tenth;
    char    hundreth;
    char    is_valid;
    char    prev_valid;
} GPS_TIME;

typedef struct _gps_location_raw {
    char        north_south;
    char        east_west;
    char        lat[16];
    char        lon[16];
    char        alt[16];
    char        sats[8];
    char        is_valid;
    uint32_t    updated;
} GPS_LOCATION_RAW;

typedef struct _gps_location_average {
    char    north_south;
    double  latitude;
    char    east_west;
    double  longitude;
    double  height;
    char    *sats;
    char    is_valid;
} GPS_LOCATION_AVERAGE;

/* GPS module API function prototypes. */
void                 gps_init(void);
void                 gps_process(void);
double               gps_convert_coord(char *s, int type);
double               gps_julian_day_number(GPS_TIME *t);
double               gps_julian_date(GPS_TIME *t);
double               gps_siderealDegrees_by_jd(double jd);
double               gps_siderealDegrees_by_time(GPS_TIME *t);
double               gps_siderealHA_by_jd(double jd);
double               gps_siderealHA_by_time(GPS_TIME *t);
GPS_TIME             *gps_get_time(GPS_TIME *q);
GPS_LOCATION_RAW     *gps_get_location_raw(GPS_LOCATION_RAW *q);
GPS_LOCATION_AVERAGE *gps_get_location_average(GPS_LOCATION_AVERAGE *q);

/* Used by other modules to make callbacks. */
void gps_pps_fall(void);    /* gpioirq.c needs this to know what to callback to. */

#define GPS_BUFFER_SIZE     128

/* Used to test the IIR register. Common across UARTs. */
#define UART_ISSET_THRE              0x0002
#define UART_ISSET_RDA               0x0004
#define UART_ISSET_CTI               0x000C
#define UART_ISSET_RLS               0x0006
#define UART_ISSET_FIFOLVL_RXFULL    0x0000000F
#define UART_ISSET_FIFOLVL_TXFULL    0x00000F00

#define UART_RX_INTERRUPT       iir & UART_ISSET_RDA


/* UART1 register configuration values. */
#define UART1_SET_LCR           0x00000003
#define UART1_SET_LCR_DLAB      0x00000083
#define UART1_SET_DLLSB         0x71
#define UART1_SET_DLMSB         0x02
#define UART1_SET_FCR           0x01
#define UART1_SET_FCR_CLEAR     0x07
#define UART1_SET_IER           0x07

/* Macros. */
#define UART1_FIFO_NOT_EMPTY    LPC_UART1->LSR & 0x1
#define UART1_GETC              (char)LPC_UART1->RBR

#endif

