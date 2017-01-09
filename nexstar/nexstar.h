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

#ifndef NEXSTAR_H
#define NEXSTAR_H

#ifndef NEXSTAR_C
extern bool nexstar_aligned;
#endif

#define IS_NEXSTAR_ALIGNED nexstar_aligned

#define NEXSTAR_INIT    0
#define NEXSTAR_IDLE    1
#define NEXSTAR_CMD     2

/* Nexstar statuses. */
#define NEXSTAR_STATE_NOT_CONN  -1
#define NEXSTAR_STATE_IDLE      1
#define NEXSTAR_STATE_BUSY      2

/* Statuses for command packets. */
#define NEXSTAR_SET_TRACKING        1
#define NEXSTAR_GET_AZMALT          2
#define NEXSTAR_GET_RADEC           3
#define NEXSTAR_SET_ELEVATION_RATE  4
#define NEXSTAR_SET_AZMITH_RATE     5
#define NEXSTAR_GOTO                6
#define NEXSTAR_SET_APPROACH        7
#define NEXSTAR_GOTO_AZM_FAST       8
#define NEXSTAR_GOTO_ELE_FAST       9
#define NEXSTAR_IS_ALIGNED          10
#define NEXSTAR_SET_TIME            11
#define NEXSTAR_SET_LOCATION        12
#define NEXSTAR_SYNC                13

#define NEXSTAR_BUFFER_SIZE  16
#define NEXSTAR_SERIAL_TIMEOUT  350

/* API functions. */
void nexstar_get_elazm(double *el, double *azm);

/* Function prototypes. */
int _nexstar_set_tracking_mode(int mode);
int _nexstar_get_altazm(void);
int _nexstar_get_radec(void);
void _nexstar_set_elevation_rate_coarse(double rate);
void _nexstar_set_elevation_rate_fine(double rate);
void _nexstar_set_elevation_rate_auto(double rate);
void _nexstar_set_azmith_rate_coarse(double rate);
void _nexstar_set_azmith_rate_fine(double rate);
void _nexstar_set_azmith_rate_auto(double rate);

int _nexstar_set_elevation_rate(void);
int _nexstar_set_azmith_rate(void);
void _nexstar_set_azm_approach(int approach);

int _nexstar_goto(uint32_t, uint32_t);
int _nexstar_goto_azm_fast(uint32_t azmith);

#include "gps.h"
void _nexstar_set_time(GPS_TIME *t);
void _nexstar_set_location(GPS_LOCATION_AVERAGE *l);

#include "satapi.h"
void _nexstar_sync(RaDec *);

double nexstar_get_rate_azm(void);
double nexstar_get_rate_alt(void);

bool _nexstar_is_aligned(void);

/* Defined in nexstar_align.c */
void nexstar_force_align(void);

/* Macros */
            
/* Used to test the IIR register. Common across UARTs. */
#define UART_ISSET_THRE              0x0002
#define UART_ISSET_RDA               0x0004
#define UART_ISSET_CTI               0x000C
#define UART_ISSET_RLS               0x0006
#define UART_ISSET_FIFOLVL_RXFULL    0x0000000F
#define UART_ISSET_FIFOLVL_TXFULL    0x00000F00

/* UART2 peripheral control masks. */
#define UART2_ORMASK_PCONP      0x01000000
#define UART2_ORMASK_PCLKSEL1   0x00010000
#define UART2_ORMASK_PINMODE0   0x00A00000
#define UART2_ORMASK_PINSEL0    0x00500000

/* UART2 register configuration values. */
#define UART2_SET_LCR           0x00000003
#define UART2_SET_LCR_DLAB      0x00000083
#define UART2_SET_DLLSB         0x71
#define UART2_SET_DLMSB         0x02
#define UART2_SET_FCR           0x01
#define UART2_SET_FCR_CLEAR     0x07
#define UART2_SET_IER           0x01

/* Standard API functions. */
void nexstar_init(void);
void nexstar_process(void);

/* endif NEXSTAR_H */
#endif

