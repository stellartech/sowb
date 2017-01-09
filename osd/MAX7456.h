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

#ifndef MAX7456_H
#define MAX7456_H

#define MAX7456_DISPLAY_LINES 15
#define MAX7456_DISPLAY_LINE_LEN 32

/* Function prototypes. */
void MAX7456_init(void);
void MAX7456_write_byte(unsigned char address, unsigned char byte);
int MAX7456_read_byte(unsigned char address);
void MAX7456_cursor(int x, int y);
unsigned char MAX7456_map_char(unsigned char c);
void MAX7456_convert_string(unsigned char *s);
void MAX7456_string(unsigned char *s);
void MAX7456_stringl(int x, int y, unsigned char *s, int len);
void MAX7456_read_char_map(unsigned char address, unsigned char *data54);
void MAX7456_write_char_map(unsigned char address, const unsigned char *data54);

#endif
