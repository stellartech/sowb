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
#include "user.h"
#include "satapi.h"
#include "utils.h"
#include "debug.h"
#include "gpio.h"
#include "osd.h"
#include "nexstar.h"
#include "utils.h"

#ifndef M_PI
#define M_PI 3.1415926535898
#endif

#define SCAN_INTERVAL 60 

SAT_POS_DATA satellite;

double satapi_aos(char *l0, char *l1, char *l2, SAT_POS_DATA *q, bool goto_aos) {
    double tsince;
    char temp1[32], temp2[32];
    
    if (q == (SAT_POS_DATA *)NULL) {
        q = &satellite;
    }
        
    strcpy(q->elements[0], l0);
    strcpy(q->elements[1], l1);
    strcpy(q->elements[2], l2);
        
    observer_now(q);
       
    for (q->tsince = 0; q->tsince < (SCAN_INTERVAL * 90); q->tsince += SCAN_INTERVAL) {
        KICK_WATCHDOG; /* We are busy! */
        satallite_calculate(q);
        if (q->elevation >= 10.) {
            /* Above horizon viewing. Work back to AOS. */
            for (tsince = q->tsince, q->tsince--; q->elevation > 10. ; tsince = q->tsince--) {
                satallite_calculate(q);
                if (q->elevation < 10.) {
                    //sprintf(temp, "%03f Q AOS El:%.1f AZ:%.1f %dKm\r\n", q->tsince, q->elevation, q->azimuth, (int)q->range);
                    //debug_printf(temp);
                    q->tsince = tsince;
                    satallite_calculate(q);
                    sprintf(temp1, "%03f T AOS El:%.1f AZ:%.1f %dKm\r\n", q->tsince, q->elevation, q->azimuth, (int)q->range);
                    debug_printf(temp1);
                    P22_DEASSERT;
                    if (goto_aos) {
                        sprintf(temp1, "%s  T-%.2f", q->elements[0], tsince);                                     
                        osd_string_xy(1, 12, temp1);                        
                        sprintf(temp1, "AOS %.2f%c %s%c %dKm", q->elevation, 176, printDouble_3_2(temp2, q->azimuth), 176, (int)q->range);
                        osd_string_xy(1, 13, temp1);
                        _nexstar_goto((uint32_t)((q->elevation / 360.) * 65536.0), (uint32_t)((q->azimuth / 360.) * 65536.0));
                    }
                    return tsince;
                }
            }
        }        
    }
           
    return 0.;     
}

int satallite_calculate(SAT_POS_DATA *q) {
    double tsince;

    /* Ensure the time and place are valid. */
    if (!q->time.is_valid)      return -1;
    if (!q->location.is_valid)  return -2;
        
    ClearFlag(ALL_FLAGS);
    
    Get_Next_Tle_Set(q->elements, &q->tle);

    select_ephemeris(&q->tle);
    
    q->jd_utc = gps_julian_date(&q->time);
    q->jd_epoch = Julian_Date_of_Epoch(q->tle.epoch);
    
    tsince = ((q->jd_utc + (q->tsince * (1 / 86400.))) - q->jd_epoch) * xmnpda;
    
    if (isFlagSet(DEEP_SPACE_EPHEM_FLAG)) {
        SDP4(tsince, &q->tle, &q->pos, &q->vel, &q->phase);
    }
    else {
        SGP4(tsince, &q->tle, &q->pos, &q->vel, &q->phase);
    }

    Convert_Sat_State(&q->pos, &q->vel);
    SgpMagnitude(&q->vel); // scalar magnitude, not brightness...
    q->velocity = q->vel.w;

    /* Populate the geodetic_t struct from data supplied. */
    q->observer.lat   = q->location.latitude  * de2ra;
    q->observer.lon   = q->location.longitude * de2ra;
    q->observer.alt   = q->location.height / 1000.;
    if (q->location.north_south == 'S') q->observer.lat *= -1.;
    if (q->location.east_west   == 'W') q->observer.lon *= -1.;
    
    Calculate_Obs(q->jd_utc, &q->pos, &q->vel, &q->observer, &q->obs_set);
    Calculate_LatLonAlt(q->jd_utc, &q->pos, &q->sat_geodetic);

    q->azimuth   = Degrees(q->obs_set.x);
    q->elevation = Degrees(q->obs_set.y);
    q->range     = q->obs_set.z;
    q->rangeRate = q->obs_set.w;
    q->height    = q->sat_geodetic.alt;
        
    return 0;
}

/** observer_now
 *
 * Fills the data structure with the observers time and position.
 * 
 * @param SAT_POS_DATA * A pointer to the data structure.
 */
SAT_POS_DATA * observer_now(SAT_POS_DATA *q) {
    gps_get_time(&q->time);
    gps_get_location_average(&q->location);
    return q;    
}

AltAz * radec2altaz(double siderealDegrees, GPS_LOCATION_AVERAGE *location, RaDec *radec, AltAz *altaz) {
    double HA, DEC, LAT, mul, altitude, azimuth;
    
    mul = location->north_south == 'S' ? -1.0 : 1.0;
     
    /* Convert to radians. */
    HA = siderealDegrees * (M_PI / 180.0) - (radec->ra * (M_PI / 180));
    DEC = radec->dec * (M_PI / 180.0);
    LAT = (location->latitude * mul) * (M_PI / 180.0);
    
    altitude = atan2(- sin(HA) * cos(DEC), cos(LAT) * sin(DEC) - sin(LAT) * cos(DEC) * cos(HA));
    azimuth = asin(sin(LAT) * sin(DEC) + cos(LAT) * cos(DEC) * cos(HA));

    // Convert to degrees and swing azimuth around if needed.
    altaz->alt = azimuth * 180.0 / M_PI;
    altaz->azm = altitude * 180.0 / M_PI;
    if (altaz->azm < 0) altaz->azm += 360.0;
  
    return altaz;
}

RaDec * altaz2radec(double siderealDegrees, GPS_LOCATION_AVERAGE *location, AltAz *altaz, RaDec *radec) {
    double ALT, AZM, LAT, HA, DEC, mul;
    
    mul = location->north_south == 'S' ? -1.0 : 1.0;
    
    /* Convert to radians. */
    LAT = (location->latitude * mul) * (M_PI / 180.0);
    ALT = altaz->alt * (M_PI / 180.0);
    AZM = altaz->azm * (M_PI / 180.0);
    
    /* Calculate the declination. */
    DEC = asin( ( sin(ALT) * sin(LAT) ) + ( cos(ALT) * cos(LAT) * cos(AZM) ) );
    radec->dec = DEC * 180.0 / M_PI;
    while (radec->dec < 0.0)   radec->dec += 360.0;
    while (radec->dec > 360.0) radec->dec -= 360.0;
    
    /* Calculate the hour angle. */
    HA = ( acos((sin(ALT) - sin(LAT) * sin(DEC)) / (cos(LAT) * cos(DEC)))) * 180.0 / M_PI;
    if (sin(AZM) > 0.0) HA = 360.0 - HA;
    
    /* Correct the HA for our sidereal time. */    
    HA = (siderealDegrees / 360.0 * 24.0) - (HA / 15.0);
    if (HA < 0.0) HA += 24.0;
    
    /* Convert the HA into degrees for the return. */
    radec->ra = HA / 24.0 * 360.0;
    
    return radec;
}

