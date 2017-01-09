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

#ifndef USER_H
#define USER_H

#include "xbox360gamepad.h"

#define WHILE_WAITING_DO_PROCESS_FUNCTIONS user_call_process();

#define KICK_WATCHDOG   { LPC_WDT->WDFEED = 0xAA; LPC_WDT->WDFEED = 0x55; }

typedef struct _user_input {
    char            xbox_button;
    unsigned char   trigger_left;
    unsigned char   trigger_right;    
    XBOX360_STICK * stick_left;
    XBOX360_STICK * stick_right;
} USER_INPUT;


char user_get_button(bool peek);
void user_wait_ms(uint32_t ms);
void user_wait_ms_blocking(uint32_t ms);
void user_call_process(void);

#endif