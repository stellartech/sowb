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
#include "usbeh.h"
#include "usbeh_endpoint.h"

int USBEH_Endpoint::address(void) {
    int address = (endpointDescriptor.control >> 7) & 0xf;
    if (address) {
        address |= flags & 0x80;
    }
    return address;    
}

int USBEH_Endpoint::device(void) {
    return endpointDescriptor.control & 0x7f;
}

int USBEH_Endpoint::status(void) {
    return (tdHead.control >> 28) & 0xF;
}

USBEH_U32 USBEH_Endpoint::queue(USBEH_U32 head) {
    if (currentState == notQueued) {
        endpointDescriptor.next = head;
        head = (USBEH_U32)&endpointDescriptor;
        currentState = idle;
    }   
    return head;
}
