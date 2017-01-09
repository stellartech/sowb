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
#include "debug.h"
#include "osd.h"
#include "gps.h"
#include "satapi.h"
#include "utils.h"
#include "nexstar.h"

int test_counter = 0;

/* Define the array of OSD dislay lines. */
OSD_display_line osd_display_area[MAX7456_DISPLAY_LINES];

/* Toggle between odd and even fields. */
int video_field;

/* The top two display lines have a special use case. This int
   is used to select the mode in which it works. */
int l01_mode;

/* Handles whether and how the crosshair is displayed. */
int crosshair_mode;


/** osd_init
 */
void osd_init(void) {

    DEBUG_INIT_START;
    video_field = 0;
    l01_mode = 0;
    crosshair_mode = 1;
    osd_clear();
    DEBUG_INIT_END;
}

/** osd_clear
 *
 * Clear the display area buffer.
 */
void osd_clear(void) {
    for (int i = 0; i < MAX7456_DISPLAY_LINES; i++) {
        osd_clear_line(i);
    }
}

/** osd_clear_line
 *
 * Clear a single line.
 *
 * @param int line The line to clear.
 */
void osd_clear_line(int line) {
    for (int i = 0; i < MAX7456_DISPLAY_LINE_LEN; i++) {
        osd_display_area[line].line_buffer[i] = '\0';
    }
    osd_display_area[line].update = true;
}

/** osd_string
 *
 * Write a null terminated string to a display line.
 *
 * @param int line The display line to write to.
 * @param char *s The null terminated string.
 */
void osd_string(int line, char *s) {
    for (int i = 0; *s; s++, i++) {
        osd_display_area[line].line_buffer[i] = *s;
    }
    osd_display_area[line].update = true;
}

/** osd_string_xy
 *
 * Write a null terminated string to a display line at x y.
 *
 * @param int line The display line to write to.
 * @param char *s The null terminated string.
 */
void osd_string_xy(int x, int y, char *s) {
    for (int i = x; *s; s++, i++) {
        osd_display_area[y].line_buffer[i] = *s;
    }
    osd_display_area[y].update = true;
}

/** osd_string_xyl
 *
 * Write a null terminated string to a display line at x y.
 *
 * @param int line The display line to write to.
 * @param char *s The null terminated string.
 * @param int len The length of the string to print.
 */
void osd_string_xyl(int x, int y, char *s, int len) {
    for (int i = x; len; s++, i++, len--) {
        osd_display_area[y].line_buffer[i] = *s;
    }
    osd_display_area[y].update = true;
}

/** osd_stringl
 *
 * Write a string of specified length to a display line.
 *
 * @param int line The display line to write to.
 * @param char *s The string.
 * @param int len The length to write.
 */
void osd_stringl(int line, char *s, int len) {
    for (int i = 0; len; s++, i++, len--) {
        osd_display_area[line].line_buffer[i] = *s;
    }
    osd_display_area[line].update = true;
}

/** osd_get_mode_l01
 */
int osd_get_mode_l01(void) {
    return l01_mode;
}

/** osd_set_mode_l01
 */
void osd_set_mode_l01(int mode) {
    l01_mode = mode;
}

/** osd_l01_next_mode
 */
void osd_l01_next_mode(void) {
    int m = osd_get_mode_l01();
    m++;
    if (m > L01_MODE_D) m = L01_MODE_A;
    osd_set_mode_l01(m);
}

/** osd_set_crosshair
 */
int osd_set_crosshair(int mode) {
    if (mode > -1) {
        crosshair_mode = mode;
    }
    return crosshair_mode;
}

/** osd_crosshair_toggle
 */
int osd_crosshair_toggle(void) {
    crosshair_mode = crosshair_mode == 0 ? 1 : 0;
    if (!crosshair_mode) osd_clear();
    return crosshair_mode;
}

/** osd_write_buffers
 *
 * Write the line buffers to the display.
 */
static void osd_write_buffers(void) {
    for (int i = 0; i < MAX7456_DISPLAY_LINES; i++) {
        if (osd_display_area[i].update) {
            MAX7456_cursor(0, i);
            MAX7456_write_byte(0x04, 0x01); /* Enable 8bit write. */
            for (int j = 0; j < MAX7456_DISPLAY_LINE_LEN; j++) {
                MAX7456_write_byte(0x80, MAX7456_map_char(osd_display_area[i].line_buffer[j]));
            } 
            MAX7456_write_byte(0x80, 0xFF);
            osd_display_area[i].update = false;            
        }
    }
}

void osd_l01_position(void) {
    GPS_LOCATION_AVERAGE loc;
    char buf[64], buf2[64];
    double el, azm;
    
    gps_get_location_average(&loc);
    
    if (l01_mode != 0) {
        if (IS_NEXSTAR_ALIGNED) {
            nexstar_get_elazm(&el, &azm);
            printDouble_3_2(buf, el);  osd_string_xy(12, 0, buf);
            printDouble_3_2(buf, azm); osd_string_xy(12, 1, buf);
        }
        else {
            strcpy(buf, "---.--");
            osd_string_xy(12, 0, buf);
            osd_string_xy(12, 1, buf);        
        }
    }
    
    switch(l01_mode) {                                                
        case L01_MODE_C:
        case L01_MODE_D:
            double2dms(buf2, loc.latitude);  sprintf(buf, "%c%s", loc.north_south, buf2); osd_string_xy(18, 0, buf);
            double2dms(buf2, loc.longitude); sprintf(buf, "%c%s", loc.east_west, buf2);   osd_string_xy(18, 1, buf);
            break;
        case L01_MODE_A:
        case L01_MODE_B:
            printDouble(buf2, loc.latitude);  sprintf(buf, " %c%c%s", loc.is_valid, loc.north_south, buf2); osd_string_xy(18, 0, buf);
            printDouble(buf2, loc.longitude); sprintf(buf, "%c%c%c%s", loc.sats[0] == '0' ? ' ' : loc.sats[0], loc.sats[1],  loc.east_west, buf2);  osd_string_xy(18, 1, buf);
            break;
    }
}

/** osd_vsync
 *
 * A callback made when the MAX7456 vertical sync fires.
 */
void osd_vsync(void) {
    GPS_TIME latched_time;
    char buffer[32];
    
    /* captire the time at which this VSync pulse occured. */
    gps_get_time(&latched_time);
    
    /* We don't actually know if this is an odd or even field.
       All we can do is divide by two so each frame only displays
       the time, otherwise it will "smudge" the display. We may
       well add in an LM1881 device to the final design as that
       has an odd/even field ttl output. */
    if (!video_field) { video_field = 1; return; }
    else { video_field = 0; }

    /* If no mode is set, do not display line0/1. */
    if (l01_mode == 0) return;
    
    /* Display lines 0 and 1 are used for a specific feature, i.e.
       the display of the time, GPS position, telescope pointing
       position, etc. Handle the "time" display portion now. */
    osd_clear_line(0);
    osd_clear_line(1);

    char buf2[32];
    double jd = gps_julian_date(&latched_time);
    
    if (crosshair_mode) {
        osd_string_xy(13, 7, "\xE0\xE1");
    }
           
    switch(l01_mode) {
        case L01_MODE_D:    
        case L01_MODE_B:
            /* Display time as Julain Date. */
            sprintf(buf2, "%.7f", jd - (long)jd);
            memset(buffer, 0, 32); sprintf(buffer, "JDI %07ld", (long)jd); osd_string_xy(0, 0, buffer);
            memset(buffer, 0, 32); sprintf(buffer, "JDF.%s",  buf2 + 2);   osd_string_xy(0, 1, buffer); 
            break;
        case L01_MODE_C:    
        case L01_MODE_A:
            /* Display time as UTC. */
            memset(buffer, 0, 32); date_AsString(&latched_time, buffer); osd_string_xy(0, 0, buffer);
            memset(buffer, 0, 32); time_AsString(&latched_time, buffer); osd_string_xy(0, 1, buffer);
            break;        
    }
    
    osd_l01_position();
        
    #ifdef NEVECOMPILETHIS    
    /* TEST TEST TEST !!! */
    /* Test failed. It actually worked, once. It seems the total math
       involved in doing this is WAY to much to be inside what is effectively
       an interrupt service routine. It does tell me lots, I need to move all
       this to _process() but do it in a way that's fast enough for user output 
       in "real time" as it would appear to the user. */
    test_counter++;
    if (test_counter == 1000) {
        test_counter = 0;
        SAT_POS_DATA data;
        data.tsince = 0;
        strcpy(data.elements[0], "ISS (ZARYA)");
        strcpy(data.elements[1], "1 25544U 98067A   10249.33655722  .00010912  00000-0  87866-4 0  8417");
        strcpy(data.elements[2], "2 25544 051.6459 175.3962 0008058 342.0418 161.2933 15.71504826676236");
        gps_get_time(&data.t);
        gps_get_location_average(&data.l);    
        satallite_calculate(&data);
        if (!isnan(data.elevation) && !isnan(data.azimuth) && !isnan(data.range)) {
            sprintf(buffer, " ISS EL:%.1f AZM:%.1f RG:%.1f", data.elevation, data.azimuth, data.range);
            osd_string_xy(0, 14, buffer);
        }
    }
    /* TEST TEST TEST !!! */
    #endif
    
    osd_write_buffers();
}





