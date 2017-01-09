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

#ifndef UTILS_H
#define UTILS_H

#include "gps.h"

char ascii2bin(char c);
uint32_t hex2bin(char *s, int len);
uint32_t dec2bin(char *s, int len);

char bin2ascii(char c);
char * bin2hex(uint32_t d, int len, char *s);

char strcsum(char *s);
char strcsuml(char *s, int len);

char strsum(char *s);
char strsuml(char *s, int len);

void date_AsString(GPS_TIME *t, char *s);
void time_AsString(GPS_TIME *t, char *s);

void gps_get_coord_AsString(char *s, int type);

void double2dms(char *s, double d);
void printDouble(char *s, double d);
char * printDouble_3_1(char *s, double d);
char * printDouble_3_2(char *s, double d);

void printBuffer(char *s, int len);
#endif
