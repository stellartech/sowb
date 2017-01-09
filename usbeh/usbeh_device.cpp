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
#include "usbeh_device.h"

int USBEH_Device::init(USBEH_deviceDescriptor *d, int hub, int port, int addr, int lowSpeed) {
    this->hub  = hub;
    this->port = port;
    this->addr = addr;
    this->flags = lowSpeed;
    memset(endpoints,0xFF,sizeof(endpoints));
    return 0;    
}


int USBEH_Device::setEndpointIndex (int endpoint, int endpointIndex) {
    for (int i = 0; i < 16; i += 2) {
        if (endpoints[i] == 0xFF) {
            endpoints[i+0] = endpoint;
            endpoints[i+1] = endpointIndex;
            return 0;
        }
    }
    return -1;
}

int USBEH_Device::getEndpointIndex (int endpoint) {
    for (int i = 0; i < 16; i += 2) {
        if (endpoints[i] == endpoint) return endpoints[i+1];
        if (endpoints[i] == 0xFF) break;
    }
    return -1;
}
