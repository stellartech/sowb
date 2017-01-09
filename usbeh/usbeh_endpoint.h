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
 
#ifndef USBEH_ENDPOINT_H
#define USBEH_ENDPOINT_H

#ifndef USBEH_H
#include "usbeh.h"
#endif

class USBEH_Endpoint {

    public:
    USBEH_HCED  endpointDescriptor;
    USBEH_HCTD  tdHead;
    
    // State definitions.
    enum state {
        free,               // 0
        notQueued,          // 1
        idle,               // 2
        setupQueued,        // 3
        dataQueued,         // 4
        statusQueued,       // 5
        callbackPending     // 6
    };

    // Local data.
    volatile USBEH_U08  currentState;
    USBEH_U08           flags;
    USBEH_U16           length;
    USBEH_U08           *data;
    USBEH_callback      callback;
    void                *userData;
    
    // Member functions.
    int address(void);
    int device(void);
    int status(void);
    USBEH_U32 queue(USBEH_U32);
};

// END #ifndef USBEH_ENDPOINT_H
#endif
