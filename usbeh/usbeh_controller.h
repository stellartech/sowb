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
 
#ifndef USBEH_CONTROLLER_H
#define USBEH_CONTROLLER_H

#include "usbeh.h"
#include "usbeh_device.h"
#include "usbeh_endpoint.h"

class USBEH_Controller {

    public:
    
    USBEH_HCCA      hcca;
    USBEH_Endpoint  endpoints[USBEH_MAX_ENDPOINTS_TOTAL];
    USBEH_Endpoint  endpointZero;
    USBEH_HCTD      commonTail;
    USBEH_Setup     setupZero;
    USBEH_Device    devices[USBEH_MAX_DEVICES];
    
    USBEH_U32       frameNumber;
    USBEH_U08       callbacksPending;
    USBEH_U08       rootHubStatusChange;
    USBEH_U08       unused0;
    USBEH_U08       unused1;
    USBEH_U08       connectPending;
    USBEH_U08       connectCountdown;
    USBEH_U08       connectHub;
    USBEH_U08       connectPort;
    
    USBEH_U08       SRAM[0];
    
    // Methods.
    void    init(void);
    void    process(void);
    int     initHub(int device);
    int     transfer(USBEH_Endpoint *endpoint, int token, USBEH_U08 *data, int length, int state);
    bool    remove(USBEH_HCED *ed, volatile USBEH_HCED **queue);
    void    release(USBEH_Endpoint *endpoint);
    int     addEndpoint(int device, USBEH_endpointDescriptor *endpoint);
    int     addEndpoint(int device, int endpoint, int attributes, int maxPacketSize, int interval);
    int     addDevice(int hub, int port, bool isLowSpeed);
    int     addDeviceCore(int hub, int port, bool isLowSpeed);
    int     setConfigurationAndInterface(int device, int configuration, int interfaceNumber, USBEH_deviceDescriptor *desc);
    void    processDoneQueue(USBEH_U32 tdList);
    void    resetPort(int hub, int port);
    void    connect(int hub, int port, bool lowSpeed);
    void    disconnect(int hub, int port);
    void    hubStatusChange(int hub, int port, USBEH_U32 status);
    void    delayMS(int ms);
    void    initHW(USBEH_HCCA *cca);
    void    hubInterrupt(int device);
    static void hubInterruptCallback(int device, int endpoint, int status, USBEH_U08 *data, int length, void *userData);
    USBEH_HCTD * reverse(USBEH_HCTD *current);
    USBEH_Endpoint * getEndpoint(int device, int ep);
    USBEH_Endpoint * allocateEndpoint(int device, int endpointAddress, int type, int maxPacketSize);

};


// END #ifndef USBEH_CONTROLLER_H
#endif
