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

#ifndef OSD_H
#define OSD_H

#include "MAX7456.h"

#define L01_MODE_A    1
#define L01_MODE_B    2
#define L01_MODE_C    3
#define L01_MODE_D    4

/* Define a structure that holds a OSD character line.
   This is used to basically buffer the line so that it
   can be displayed during the vertical sync period.
   Additionally, we hold a binary flag to state whether
   teh buffer has been updated. That way we know if we
   don't need to send the buffer to the MAX7456 device
   if no update to the buffer has occured. */
   
typedef struct {
    bool update;
    char line_buffer[MAX7456_DISPLAY_LINE_LEN];
} OSD_display_line;


void osd_init(void);
void osd_clear(void);
void osd_clear_line(int line);
void osd_string(int line, char *s);
void osd_string_xy(int x, int y, char *s);
void osd_stringl(int line, char *s, int len);
void osd_string_xyl(int x, int y, char *s, int len);

void osd_set_mode_l01(int mode);
void osd_l01_next_mode(void);
int osd_set_crosshair(int mode);
int osd_crosshair_toggle(void);

void osd_vsync(void);



#endif

