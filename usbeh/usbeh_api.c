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
#include "usbeh_api.h"
#include "usbeh_endpoint.h"
#include "usbeh_device.h"
#include "usbeh_controller.h"

#include "main.h"
#include "debug.h"

/* Device driver headers. */
#include "xbox360gamepad.h"

#define DEBUG_USB_API 1

USBEH_device_info_callback usb_devices_callback_map[] = {
    (xbox360gamepad_onload_callback),
    NULL
};

int usbeh_no_device_found(int device, USBEH_deviceDescriptor *deviceDesc, USBEH_interfaceDescriptor **interfaceDesc) {
    USBEH_interfaceDescriptor *iface;
    int interfaceCounter = 0;
    debug_printf("%s CALLBACK ACTIVATED for device %d\r\n", __FUNCTION__, device);
    debug_printf("  VendorId = %04X ProductId = %04X \r\n", deviceDesc->idVendor, deviceDesc->idProduct);
    while ((iface = interfaceDesc[interfaceCounter]) != (USBEH_interfaceDescriptor *)NULL) { 
        debug_printf("  interface%d:- \r\n", interfaceCounter);
        debug_printf("    InterfaceClass    = %02X \r\n", iface->bInterfaceClass);
        debug_printf("    InterfaceSubClass = %02X \r\n", iface->bInterfaceSubClass);
        debug_printf("    InterfaceProtocol = %02X \r\n", iface->bInterfaceProtocol);
        interfaceCounter++;
    }
    debug_printf("  No device driver loaded.\r\n");
    return 0;
}

/* Create an instance of the USBEH controller and place
   it within the AHB static ram area. */
USBEH_Controller sys_usb_controller __attribute__((at(0x2007C000)));


void usbeh_api_on_load_device(int device, USBEH_deviceDescriptor* deviceDesc, USBEH_interfaceDescriptor **interfaceDesc) {
    int i, slen, driver_loaded = 0;
    char s[3][128];
    
    memset((char *)s, 0, 3 * 128);
    
    /* Get device strings. */    
    for (i = 1; i < 3; i++) {
        slen = usbeh_api_get_string(device, i, s[i - 1], 128);
        if (slen < 0) {
            break;
        }
    }
 
    /* Scan through the device driver onLoad callbacks to see who wants to claim it. */
    for (i = 0; usb_devices_callback_map[i] != NULL && driver_loaded == 0; i++) {
        driver_loaded = (usb_devices_callback_map[i])(device,deviceDesc,interfaceDesc);
    }
    
    if (driver_loaded == 0) {
        usbeh_no_device_found(device, deviceDesc, interfaceDesc);
    }
}

int usbeh_api_init(void) {
    DEBUG_INIT_START;
    sys_usb_controller.init();
    DEBUG_INIT_END;
    return 0;
}

void usbeh_api_process(void) {
    sys_usb_controller.process();
}

int usbeh_api_set_address(int device, int new_addr) {
    return usbeh_api_control_transfer(device, USBEH_HOST_TO_DEVICE | USBEH_RECIPIENT_DEVICE, USBEH_SET_ADDRESS, new_addr, 0, 0, 0, 0, 0);
}

int usbeh_api_get_descriptor(int device, int descType,int descIndex, USBEH_U08* data, int length) {
    return usbeh_api_control_transfer(device, USBEH_DEVICE_TO_HOST | USBEH_RECIPIENT_DEVICE, USBEH_GET_DESCRIPTOR, (descType << 8)|(descIndex), 0, data, length, 0, 0);
}

static USBEH_Setup* usbeh_api_get_setup(int device) {
    if (device == 0) {
        return &sys_usb_controller.setupZero;
    }
    
    if (device < 1 || device > USBEH_MAX_DEVICES) {
        return 0;
    }
    
    return &sys_usb_controller.devices[device-1].setupBuffer;
}

static int usbeh_api_wait_IO_done(USBEH_Endpoint* endpoint) {

    if (endpoint->currentState == USBEH_Endpoint::notQueued) {
        return 0;
    }
    
    while (endpoint->currentState != USBEH_Endpoint::idle) {
        usbeh_api_process();
    }
    
    int status = endpoint->status();
    if (status == 0) {
        return endpoint->length;
    }

    #ifdef DEBUG_USB_DRIVER
    debug_printf("usbeh_api_wait_IO_done() error with 0x%x at line %d\r\n", status, __LINE__);
    #endif
    
    return -status;
}


int usbeh_api_get_port_status(int device, int port, USBEH_U32 *status) {
    return usbeh_api_control_transfer_short(device, USBEH_DEVICE_TO_HOST | USBEH_REQUEST_TYPE_CLASS | USBEH_RECIPIENT_OTHER, USBEH_GET_STATUS, 0, port, (USBEH_U08 *)status, 4);    
}

int usbeh_api_clear_port_feature(int device, int feature, int index) {
    return usbeh_api_control_transfer(device, USBEH_HOST_TO_DEVICE | USBEH_REQUEST_TYPE_CLASS | USBEH_RECIPIENT_OTHER, USBEH_CLEAR_FEATURE, feature, index, 0, 0, 0, 0);
}

int usbeh_api_set_port_power(int device, int port) {
    int result = usbeh_api_set_port_feature(device, USBEH_PORT_POWER, port);
    USBEH_OS_DELAY_MS(20);
    return result;
}

int usbeh_api_set_port_feature(int device, int feature, int index) {
    return usbeh_api_control_transfer(device, USBEH_HOST_TO_DEVICE | USBEH_REQUEST_TYPE_CLASS | USBEH_RECIPIENT_OTHER, USBEH_SET_FEATURE, feature, index, 0, 0, 0, 0);
}

int usbeh_api_set_configuration(int device, int configNum) {
    return usbeh_api_control_transfer(device, USBEH_HOST_TO_DEVICE | USBEH_RECIPIENT_DEVICE, USBEH_SET_CONFIGURATION, configNum, 0, 0, 0, 0, 0);
}

int usbeh_api_set_port_reset(int device, int port) {
    return usbeh_api_set_port_feature(device, USBEH_PORT_RESET, port);
}

int usbeh_api_get_string(int device, int index, char *dst, int length) {
    
    USBEH_U08 buffer[255];
    
    int le = usbeh_api_get_descriptor(device, USBEH_DESCRIPTOR_TYPE_STRING, index, buffer, sizeof(buffer));
    
    if (le < 0) {
        return le;
    }
    
    if (length < 1) {
        return -1;
    }
    
    length <<= 1;
    
    if (le > length) {
        le = length;
    }
    
    for (int j = 2; j < le; j += 2) {
        *dst++ = buffer[j];
    }
    
    *dst = 0;
    
    return (le >> 1) - 1;
}

int usbeh_api_transfer(int device, int ep, USBEH_U08 flags, USBEH_U08 *data, int length, USBEH_callback callback, void *userData) {
    USBEH_Endpoint *endpoint = sys_usb_controller.getEndpoint(device, ep);
    if (!endpoint) {
        #ifdef DEBUG_USB_DRIVER
        debug_printf("sys_usb_controller.getEndpoint() failed at line %d\r\n", __LINE__);
        #endif
        return USBEH_ERR_ENDPOINT_NOT_FOUND;
    }
        
    usbeh_api_wait_IO_done(endpoint);
    
    endpoint->flags = flags;
    endpoint->data = data;
    endpoint->length = length;
    endpoint->callback = callback;
    endpoint->userData = userData;
    
    if (ep == 0) {
        sys_usb_controller.transfer(endpoint, USBEH_TOKEN_SETUP, (USBEH_U08 *)usbeh_api_get_setup(device), 8, USBEH_Endpoint::setupQueued);
    }
    else {
        sys_usb_controller.transfer(endpoint, flags & 0x80 ? USBEH_TOKEN_IN : USBEH_TOKEN_OUT, data, length, USBEH_Endpoint::dataQueued);
    }
    
    if (callback) {
        return USBEH_IO_PENDING;
    }
    
    return usbeh_api_wait_IO_done(endpoint);
}

int usbeh_api_interrupt_transfer(int device, int ep, USBEH_U08 *data, int length, USBEH_callback callback, void *userData) {
    return usbeh_api_transfer(device, ep, (ep & 0x80) | USBEH_ENDPOINT_INTERRUPT, data, length, callback, userData);
}

int usbeh_api_control_transfer_short(int device, int request_type, int request, int value, int index, USBEH_U08 *data, int length) {
    return usbeh_api_control_transfer(device, request_type, request, value, index, data, length, 0, 0);
}

int usbeh_api_control_transfer(int device, int request_type, int request, int value, int index, USBEH_U08 *data, int length, USBEH_callback callback, void *userData) {
    USBEH_Setup* setup = usbeh_api_get_setup(device);
    if (!setup) {
        #ifdef DEBUG_USB_DRIVER
        debug_printf("usbeh_api_get_setup() failed at line %d\r\n", __LINE__);
        #endif
        return USBEH_ERR_DEVICE_NOT_FOUND;
    }
        
    usbeh_api_wait_IO_done(sys_usb_controller.getEndpoint(device,0));
    
    setup->bm_request_type = request_type;
    setup->b_request = request;
    setup->w_value = value;
    setup->w_index = index;
    setup->w_length = length;

    return usbeh_api_transfer(device, 0, request_type & USBEH_DEVICE_TO_HOST, data, length, callback, userData);
}

void usbeh_sof_counter_init(USBEH_SOF_COUNTER *q, USBEH_U08 mode, USBEH_U32 count) {
    q->mode     = mode;
    q->flag     = 0;
    q->counter  = count;
    q->reload   = count;
    q->callback = NULL;
    q->next     = NULL;
}

extern USBEH_SOF_COUNTER   *sof_counter_head;
void usbeh_sof_counter_register(USBEH_SOF_COUNTER *q) {
    q->next = sof_counter_head;
    sof_counter_head = q;
}

void usbeh_sof_counter_unregister(USBEH_SOF_COUNTER *q) {
    USBEH_SOF_COUNTER *list;
    for (list = sof_counter_head; list != NULL; list = list->next) {
        if (list->next == q) {
            list->next = q->next;
            return;
        }
    }
}


void usbeh_api_list_endpoints(void) {
    debug_printf("List system endpoints\r\n");
    for (int i = 0; i < USBEH_MAX_ENDPOINTS_TOTAL; i++) {
        printf("Index %d: 0x%02X Status:%02X State:%d\r\n", i, 
            sys_usb_controller.endpoints[i].address(), 
            sys_usb_controller.endpoints[i].status(), 
            sys_usb_controller.endpoints[i].currentState
        ); 
    }
}
