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
#include "osd.h"
#include "nexstar.h"
#include "xbox360gamepad.h"
#include "user.h"

/** nexstar_force_align
 *
 * On boot-up if main() detects the Nexstar is not aligned
 * full control is passed here to force the user to align 
 * the Nexstar.
 */
void nexstar_force_align(void) {
    char c, buf[32];

    osd_stringl(11, buf, sprintf(buf, "  Please align the NexStar"));
    osd_stringl(12, buf, sprintf(buf, "  Press 'start'  when done"));
    
    do {
        c = 0;
        while(c != BUTT_START_PRESS) {
            c = user_get_button(false);
        }
    }
    while(!_nexstar_is_aligned());
    
    osd_clear_line(11);
    osd_clear_line(12);
    
    _nexstar_set_tracking_mode(0);
    
}
