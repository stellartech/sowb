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
 
#ifndef SATAPI_H
#define SATAPI_H

#include "gps.h"
#include "sgp4sdp4.h"

#define JD_SECOND   (1. / 86400.)
#define JD_MINUTE   (JD_SECOND * 60.)
#define JD_HOUR     (JD_MINUTE * 60.)
#define JD_DAY      (JD_HOUR   * 24.)

typedef struct _altaz {
    double  alt;
    double  azm;
} AltAz;

typedef struct _radec {
    double  ra;
    double  dec;
} RaDec;

typedef struct _eci {
    double x;
    double y;
    double z;
    double xdot;
    double ydot;
    double zdot;
    double atTime;
} Eci;

typedef struct _sat_pos_data {

    /* Inputs. */
    char                    elements[3][80];
    GPS_TIME                time;
    GPS_LOCATION_AVERAGE    location;    
    
    /* Semi-intermediates. 
       Set to zero for JD_UTC - SAT EPOCH.
       If you want to know the satellite position say 5 seconds 
       into the furture set this to 5.0  The "real" tsince is
       calculated by JD_UTC + (tsince * (1 / 86400.)) - JD_SAT_EPOCH */
    double      tsince;
    
    /* Intermediates. */
    tle_t       tle;        /* Constructed from the elements arrays. */
    geodetic_t  observer;   /* Derived from input observer's location. */
    double      jd_epoch;   /* Computed from the TLE epoch time. */
    double      jd_utc;     /* Computed from the GPS_TIME t */
    double      phase;
    vector_t    vel;
    vector_t    pos;
    vector_t    obs_set;
    geodetic_t  sat_geodetic;
    
    /* Outputs. */
    double azimuth;
    double elevation;
    double range;
    double rangeRate;
    double height;
    double velocity;
    
} SAT_POS_DATA;

double satapi_aos(char *, char *, char *, SAT_POS_DATA *, bool);
int satallite_calculate(SAT_POS_DATA *q);
SAT_POS_DATA * observer_now(SAT_POS_DATA *q);

AltAz * radec2altaz(double siderealDegrees, GPS_LOCATION_AVERAGE *location, RaDec *radec, AltAz *altaz);
RaDec * altaz2radec(double siderealDegrees, GPS_LOCATION_AVERAGE *location, AltAz *altaz, RaDec *radec);

#endif
