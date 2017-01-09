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
 
#ifndef TH_XBOX360GAMEPAD_H
#define TH_XBOX360GAMEPAD_H

#ifndef BUTT_LS_PRESS
#define BUTT_LS_PRESS                 1
#define BUTT_RS_PRESS                 2
#define BUTT_XBOX_PRESS               3
#define BUTT_UNUSED_PRESS             4
#define BUTT_A_PRESS                  5
#define BUTT_B_PRESS                  6
#define BUTT_X_PRESS                  7
#define BUTT_Y_PRESS                  8
#define BUTT_DPAD_UP_PRESS            9
#define BUTT_DPAD_DOWN_PRESS          10
#define BUTT_DPAD_LEFT_PRESS          11
#define BUTT_DPAD_RIGHT_PRESS         12
#define BUTT_START_PRESS              13
#define BUTT_BACK_PRESS               14
#define BUTT_LEFT_HAT_PRESS           15
#define BUTT_RIGHT_HAT_PRESS          16
#endif

#define STICK_DIVISOR    1024


void th_xbox360gamepad_init(void);
void th_xbox360gamepad(void);

#endif

