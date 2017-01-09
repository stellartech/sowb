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

#include "string.h"
#include "sowb.h"
#include "osd.h"
#include "sgp4sdp4.h"
#include "predict_th.h"
#include "gps.h"
#include "gpio.h"
#include "satapi.h"
#include "debug.h"

#ifdef PREDICT_TH_RUN

tle_t       ISS_TLE;



int sgp4sdp4_th_init(void) {
    char buf[128];
    GPS_TIME t;
    double jd_epoch, jd_utc, tsince, phase;
    vector_t vel = { 0, 0, 0 };
    vector_t pos = { 0, 0, 0 };
    vector_t obs_set;
    geodetic_t obs_geodetic;
    geodetic_t sat_geodetic;
    tle_t tle, localtle;
    char elements[3][80];

    /* Prepare to begin ISS */
    strcpy(elements[0], "ISS (ZARYA)");
    strcpy(elements[1], "1 25544U 98067A   10278.19511664  .00012217  00000-0  97221-4 0   147");
    strcpy(elements[2], "2 25544 051.6473 027.7875 0007506 064.6316 006.5147 15.71651651680777");
    
    debug_printf("SGP4SDP4 TH starting 1\r\n");
    
    ClearFlag(ALL_FLAGS);
    Get_Next_Tle_Set(elements, &tle);
    memcpy(&localtle, &tle, sizeof(tle_t));
    select_ephemeris(&tle);
    
    gps_get_time(&t);
    
    if (!t.is_valid) {
        debug_printf("SGP4SDP4 TH Abort, invalid time.\r\n");
        return 0;
    }
    
    jd_utc = gps_julian_date(&t);
    jd_epoch = Julian_Date_of_Epoch(tle.epoch);
    tsince = (jd_utc - jd_epoch) * xmnpda;
    
    if (isFlagSet(DEEP_SPACE_EPHEM_FLAG)) {
        //debug_printf("Using SDP4\r\n");
        SDP4(tsince, &tle, &pos, &vel, &phase);
    }
    else {
        //debug_printf("Using SGP4\r\n");
        SGP4(tsince, &tle, &pos, &vel, &phase);
    }

    Convert_Sat_State(&pos, &vel);
    //SgpMagnitude(&vel); // scalar magnitude, not brightness...
    //double velocity = vel.w;

    GPS_LOCATION_AVERAGE loc;    
    gps_get_location_average(&loc);
    if (loc.east_west   == 'W') loc.longitude *= -1.;
    if (loc.north_south == 'S') loc.latitude  *= -1.;

    obs_geodetic.lat   = loc.latitude * de2ra; // * 56.1920;
    obs_geodetic.lon   = loc.longitude * de2ra; // * -3.0339;
    obs_geodetic.alt   = loc.height / 1000.;
    
    Calculate_Obs(jd_utc, &pos, &vel, &obs_geodetic, &obs_set);
    Calculate_LatLonAlt(jd_utc, &pos, &sat_geodetic);

    double azimuth     = Degrees(obs_set.x);
    double elevation   = Degrees(obs_set.y);
    double range       = obs_set.z;
    //double rangeRate   = obs_set.w;
    //double height      = sat_geodetic.alt;
    
    //sprintf(buf, "JD UTC : %.5f  JD SAT : %.5f  DIF : %f\r\n", jd_utc, jd_epoch, jd_utc - jd_epoch);
    //debug_printf("%s", buf);
    
    sprintf(buf, "ISS El:%.1f AZ:%.1f %dKm\r\n\n", elevation, azimuth, (int)range);
    osd_string_xy(0, 14, buf);
    debug_printf("%s", buf);
    return 1;
    
    
}

#endif

