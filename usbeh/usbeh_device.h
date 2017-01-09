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
 
#ifndef USBEH_DEVICE_H
#define USBEH_DEVICE_H

#ifndef USBEH_H
#include "usbeh.h"
#endif

class USBEH_Device {

    public:
    
    USBEH_U08   endpoints[16 * 2];
    USBEH_U08   hub;
    USBEH_U08   port;
    USBEH_U08   addr;
    USBEH_U08   pad;
    
    USBEH_U08   hubPortCount;
    USBEH_U08   hubInterruptData;
    USBEH_U08   hubMap;
    USBEH_U08   hubMask;
    
    int         flags;
    USBEH_Setup setupBuffer;
    
    // Member functions.
    int init(USBEH_deviceDescriptor *d, int hub, int port, int addr, int lowSpeed);
    int setEndpointIndex (int endpoint, int endpointIndex);
    int getEndpointIndex (int endpoint);
    
};


// END #ifndef USBEH_DEVICE_H
#endif
