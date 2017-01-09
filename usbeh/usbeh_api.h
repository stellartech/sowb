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

#ifndef USBEH_API_H
#define USBEH_API_H

#include "usbeh.h"
#include "usbeh_endpoint.h"
#include "usbeh_device.h"
#include "usbeh_controller.h"

/* Define a callback for device driver OnLoad handlers. 
   Cast *info as (USBEH_DEVICE_INFO *) as that's what 
   will be passed. */
typedef int (*USBEH_device_info_callback)(int device, USBEH_deviceDescriptor *deviceDesc, USBEH_interfaceDescriptor **interfaceDesc);

/* Called for a device insertion. Define the driver in this function. */
void usbeh_api_on_load_device(int device, USBEH_deviceDescriptor* deviceDesc, USBEH_interfaceDescriptor **interfaceDesc);

/* Local statics. */
static USBEH_Setup* usbeh_api_get_setup(int device);
static int usbeh_api_wait_IO_done(USBEH_Endpoint* endpoint);

/* The exported API. */
int usbeh_api_init(void);
void usbeh_api_process(void);
int usbeh_api_set_address(int device, int new_addr);
int usbeh_api_get_descriptor(int device, int descType,int descIndex, USBEH_U08* data, int length);
int usbeh_api_get_string(int device, int index, char *dst, int length);
int usbeh_api_get_port_status(int device, int port, USBEH_U32 *status);
int usbeh_api_clear_port_feature(int device, int feature, int index);
int usbeh_api_set_port_feature(int device, int feature, int index);
int usbeh_api_set_port_power(int device, int port);
int usbeh_api_set_configuration(int device, int configNum);
int usbeh_api_set_port_reset(int device, int port);
int usbeh_api_transfer(int device, int ep, USBEH_U08 flags, USBEH_U08 *data, int length, USBEH_callback callback, void* userData);
int usbeh_api_interrupt_transfer(int device, int ep, USBEH_U08* data, int length, USBEH_callback callback, void* userData);
int usbeh_api_control_transfer_short(int device, int request_type, int request, int value, int index, USBEH_U08 *data, int length);
int usbeh_api_control_transfer(int device, int request_type, int request, int value, int index, USBEH_U08 *data, int length, USBEH_callback callback, void *userData);

void usbeh_sof_counter_init(USBEH_SOF_COUNTER *q, USBEH_U08 mode, USBEH_U32 count);
void usbeh_sof_counter_register(USBEH_SOF_COUNTER *q);
void usbeh_sof_counter_unregister(USBEH_SOF_COUNTER *q);

/* Debugging. */
void usbeh_api_list_endpoints(void);

// END #ifndef USBEH_API_H
#endif
