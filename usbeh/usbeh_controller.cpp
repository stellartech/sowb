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
#include "usbeh_endpoint.h"
#include "usbeh_device.h"
#include "usbeh_controller.h"
#include "usbeh_api.h"

#include "main.h"
#include "debug.h"

void USBEH_Controller::process(void) {
    USBEH_Endpoint *endpoint;
    USBEH_U16 elapsed = hcca.frameNumber - (USBEH_U16)this->frameNumber;
    this->frameNumber += elapsed;
    
    while (this->callbacksPending) {
        for (int i = 0; i < USBEH_MAX_ENDPOINTS_TOTAL; i++) {
            endpoint = this->endpoints + i;
            if (endpoint->currentState == USBEH_Endpoint::callbackPending) {
                this->callbacksPending--;
                endpoint->currentState = USBEH_Endpoint::idle;
                endpoint->callback(
                    endpoint->device(),
                    endpoint->address(),
                    endpoint->status(),
                    (USBEH_U08 *)endpoint->userData,
                    endpoint->length,
                    endpoint->userData);
            }
        }
    }
    
    if (this->rootHubStatusChange) {
        USBEH_U32 status = USBEH_HcRhPortStatus1;
        this->rootHubStatusChange = 0;
        if (status >> 16) {
            hubStatusChange(0, 1, status);
            USBEH_HcRhPortStatus1 = status & 0xFFFF0000;
        }
    }
    
    if (this->connectCountdown) {
        if (elapsed >= this->connectCountdown) {
            this->connectCountdown = 0;
            connect(this->connectHub, this->connectPort & 0x7F, this->connectPort & 0x80);
        }
        else {
            this->connectCountdown -= elapsed;
        }
    } 
}

void USBEH_Controller::init(void) {
    memset(this, 0, sizeof(USBEH_Controller));
    endpointZero.currentState = USBEH_Endpoint::notQueued;
    initHW(&hcca);
    delayMS(10);
}

void USBEH_Controller::hubInterrupt(int device) {
    USBEH_Device *dev = &devices[device - 1];
    
    for (int i = 0; i < dev->hubPortCount; i++) {
        int port = i + 1;
        if (dev->hubInterruptData * (1 << port)) {
            USBEH_U32 status = 0;
            usbeh_api_get_port_status(device, port, &status);
            if (status >> 16) {
                if (connectPending && (status & USBEH_CONNECT_STATUS_CHANGE)) {
                    hubStatusChange(device, port, status);
                    if (status & USBEH_CONNECT_STATUS_CHANGE) {
                        usbeh_api_clear_port_feature(device, USBEH_C_PORT_CONNECTION, port);
                    }    
                    if (status & USBEH_PORT_RESET_STATUS_CAHNGE) {
                        usbeh_api_clear_port_feature(device, USBEH_C_PORT_RESET, port);
                    }
                }
            }
        }
    }
}

void USBEH_Controller::hubInterruptCallback(int device, int endpoint, int status, USBEH_U08 *data, int length, void *userData) {
    USBEH_Controller *controller = (USBEH_Controller *)userData;
    if (status == 0) {
        controller->hubInterrupt(device);
    }
    usbeh_api_interrupt_transfer(device, endpoint, data, 1, hubInterruptCallback, userData);
}

int USBEH_Controller::initHub(int device) {
    USBEH_U08 buffer[16];
    int r = usbeh_api_control_transfer(device, USBEH_DEVICE_TO_HOST | USBEH_REQUEST_TYPE_CLASS | USBEH_RECIPIENT_DEVICE, USBEH_GET_DESCRIPTOR, (USBEH_DESCRIPTOR_TYPE_HUB << 8), 0, buffer, sizeof(buffer), 0, 0);
    if (r < 0) return -1;
    
    USBEH_Device *dev = &this->devices[device - 1];
    
    int ports = buffer[2];
    
    for (int i = 0; i < ports; i++) {
        usbeh_api_set_port_power(device, i + 1);
    }
    
    return usbeh_api_interrupt_transfer(device, 0x81, &dev->hubInterruptData,1, hubInterruptCallback, this); 
}

int USBEH_Controller::transfer(USBEH_Endpoint *endpoint, int token, USBEH_U08 *data, int len, int state) {
   
    int toggle = 0;
    
    if (endpoint->address() == 0) {
        toggle = (token == USBEH_TOKEN_SETUP) ? USBEH_TD_TOGGLE_0 : USBEH_TD_TOGGLE_1;
    }
    
    if (token != USBEH_TOKEN_SETUP) {
        token = (token == USBEH_TOKEN_IN ? USBEH_TD_IN : USBEH_TD_OUT);
    }
    
    USBEH_HCTD *head = &endpoint->tdHead;
    USBEH_HCTD *tail = &this->commonTail;
    
    head->control = USBEH_TD_ROUNDING | token | USBEH_TD_DELAY_INT(0) | toggle | USBEH_TD_CC;
    head->currentBufferPointer = (USBEH_U32)data;
    head->bufferEnd = (USBEH_U32)(data + len - 1);
    head->next = (USBEH_U32)tail;
   
    USBEH_HCED *ed = &endpoint->endpointDescriptor;
    ed->headTd = (USBEH_U32)head | (ed->headTd & 0x00000002);
    ed->tailTd = (USBEH_U32)tail;
    
    switch (endpoint->flags & 3) {
        case USBEH_ENDPOINT_CONTROL:
            USBEH_HcControlHeadED = endpoint->queue(USBEH_HcControlHeadED);
            endpoint->currentState = state;
            USBEH_HcCommandStatus = USBEH_HcCommandStatus | USBEH_CONTROL_LIST_FILLED;
            USBEH_HcControl |= USBEH_CONTROL_LIST_ENABLE;
            break;
        case USBEH_ENDPOINT_BULK:
            USBEH_HcBulkHeadED = endpoint->queue(USBEH_HcBulkHeadED);
            endpoint->currentState = state;
            USBEH_HcCommandStatus = USBEH_HcCommandStatus | USBEH_BULK_LIST_FILLED;
            USBEH_HcControl |= USBEH_BULK_LIST_ENABLE;
            break;
        case USBEH_ENDPOINT_INTERRUPT:
            hcca.interruptTable[0] = endpoint->queue(hcca.interruptTable[0]);
            endpoint->currentState = state;
            USBEH_HcControl |= USBEH_PERIODIC_LIST_ENABLE;
            break;
    }
    
    return 0;
}

bool USBEH_Controller::remove(USBEH_HCED *ed, volatile USBEH_HCED **queue) {
    if (*queue == 0) return false;
    if (*queue == (volatile USBEH_HCED *)ed) {
        *queue = (volatile USBEH_HCED *)ed->next;
        return true;    
    }
    
    volatile USBEH_HCED *head = *queue;
    while (head) {
        if (head->next == (USBEH_U32)ed) {
            head->next = ed->next;
            return true;
        }
        head = (volatile USBEH_HCED *)head->next;
    }
    
    return false;
}

void USBEH_Controller::release(USBEH_Endpoint *endpoint) {
    if (endpoint->currentState != USBEH_Endpoint::notQueued) {
        USBEH_HCED *ed = (USBEH_HCED *)endpoint;
        ed->control |= 0x4000;
        switch (endpoint->flags & 0x3) {
            case USBEH_ENDPOINT_CONTROL:
                remove(ed, (volatile USBEH_HCED **)&USBEH_HcControlHeadED);
                break;
            case USBEH_ENDPOINT_BULK:
                remove(ed, (volatile USBEH_HCED **)&USBEH_HcBulkHeadED);
                break;
            case USBEH_ENDPOINT_INTERRUPT:
                for (int i = 0; i < 32; i++) {
                    remove(ed, (volatile USBEH_HCED **)&hcca.interruptTable[i]);
                }
                break;
        }
        
        USBEH_U16 fn = hcca.frameNumber;
        while (fn == hcca.frameNumber) ;
    }
    
    memset(endpoint, 0, sizeof(USBEH_Endpoint));
}

int USBEH_Controller::addEndpoint(int device, USBEH_endpointDescriptor *endpoint) {
    return addEndpoint(device, endpoint->bEndpointAddress, endpoint->bmAttributes, endpoint->wMaxPacketSize, endpoint->bInterval);
}

int USBEH_Controller::addEndpoint(int device, int endpoint, int attributes, int maxPacketSize, int interval) {
    USBEH_Device *dev = &this->devices[device - 1];
    USBEH_Endpoint *ep = allocateEndpoint(device, endpoint, attributes, maxPacketSize);
    if (!ep) {
        return -1;
    }
    dev->setEndpointIndex(endpoint, ep - this->endpoints);
    ep->endpointDescriptor.control |= dev->flags;
    return 0;

}

USBEH_Endpoint * USBEH_Controller::allocateEndpoint(int device, int endpointAddress, int type, int maxPacketSize) {
    for (int i = 0; i < USBEH_MAX_ENDPOINTS_TOTAL; i++) {
        USBEH_Endpoint *ep = &this->endpoints[i];
        if (ep->currentState == 0) {
            ep->flags = (endpointAddress & 0x80) | (type & 0x3);
            ep->currentState = USBEH_Endpoint::notQueued;
            ep->endpointDescriptor.control = (maxPacketSize << 16) | ((endpointAddress & 0x7F) << 7) | device;
            return ep;
        }
    }
    return 0;
}

int USBEH_Controller::addDevice(int hub, int port, bool isLowSpeed) {
    int device = addDeviceCore(hub, port, isLowSpeed);
    if (device < 0) {
        disconnect(hub, port);
        resetPort(hub, port);
        return -1;
    }
    return device;
}

int USBEH_Controller::addDeviceCore(int hub, int port, bool isLowSpeed) {
    
    int lowSpeed = isLowSpeed ? 0x2000 : 0;
    
    USBEH_deviceDescriptor desc;
    
    endpointZero.endpointDescriptor.control = (8 << 16) | lowSpeed;
    
    int result = usbeh_api_get_descriptor(0, USBEH_DESCRIPTOR_TYPE_DEVICE, 0, (USBEH_U08 *)&desc, 8);
        
    if (result < 0) {
        #ifdef DEBUG_USB_DRIVER
        debug_printf("usbeh_api_get_descriptor() failed with %d (0x%x) at line %d\r\n", result, result, __LINE__);
        #endif
        return result;
    }
    
    endpointZero.endpointDescriptor.control = (desc.bMaxPacketSize << 16) | lowSpeed;
    
    result = usbeh_api_get_descriptor(0, USBEH_DESCRIPTOR_TYPE_DEVICE, 0, (USBEH_U08 *)&desc, sizeof(desc));
    if (result < 0) {
        #ifdef DEBUG_USB_DRIVER
        debug_printf("usbeh_api_get_descriptor() failed with %d at line %d\r\n", result, __LINE__);
        #endif
        return result;
    }
    
    int device = 0;
    for (int i = 0; i < USBEH_MAX_DEVICES; i++) {
        if (devices[i].port == 0) {
            device = i + 1;
            break;
        }
    }
    
    if (!device) {
        return -1;
    }
    
    result = usbeh_api_set_address(0, device);
    if (result) {
        return result;
    }
    
    delayMS(2);
    
    USBEH_Device *dev = &devices[device - 1];
    dev->init(&desc, hub, port, device, lowSpeed);
    addEndpoint(device, 0, USBEH_ENDPOINT_CONTROL, desc.bMaxPacketSize, 0);
    this->connectPending = 0;
    
    if ((result = usbeh_api_get_descriptor(device, USBEH_DESCRIPTOR_TYPE_DEVICE, 0, (USBEH_U08 *)&desc, sizeof(desc))) < 0) {
        #ifdef DEBUG_USB_DRIVER
        debug_printf("usbeh_api_get_descriptor() failed with %d at line %d\r\n", result, __LINE__);
        #endif
        return result;
    }
    
    result = setConfigurationAndInterface(device, 1, -1, &desc);
    
    if (desc.bDeviceClass == CLASS_HUB) {
        initHub(device);
    }
    
    return device;
}

int USBEH_Controller::setConfigurationAndInterface(int device, int configuration, int interfaceNumber, USBEH_deviceDescriptor *desc) {
    USBEH_U08 buffer[255];
    USBEH_interfaceDescriptor *found[16];
    USBEH_endpointDescriptor *ed;
    
    for (int i = 0; i < 16; i++) {
        found[i] = (USBEH_interfaceDescriptor *)NULL;
    }
    
    int err = usbeh_api_get_descriptor(device, USBEH_DESCRIPTOR_TYPE_CONFIGURATION, 0, buffer, sizeof(buffer));
    if (err < 0) {
        #ifdef DEBUG_USB_DRIVER
        debug_printf("GET_DESCRIPTOR failed at line %d\r\n", __LINE__);
        #endif
        return err;
    }
    
    err = usbeh_api_set_configuration(device, configuration);
    if (err < 0) {
        return err;
    }
    
    int interfaceCounter = 0;
    int len        = buffer[2] | (buffer[3] << 8);
    USBEH_U08 *d   = buffer;
    USBEH_U08 *end = d + len;
    while (d < end) {
        //printf("Testing descriptor type %02x\n\r", d[1]);
        if (d[1] == USBEH_DESCRIPTOR_TYPE_INTERFACE) {
            //printf("  Found interface descriptor type %02x\n\r", d[1]);
            USBEH_interfaceDescriptor *id = (USBEH_interfaceDescriptor *)d;
            if (interfaceNumber == -1 || id->bInterfaceNumber == interfaceNumber) {
                found[interfaceCounter++] = id;
                d += d[0];
                while (d < end && d[1] != USBEH_DESCRIPTOR_TYPE_INTERFACE) {
                    switch (d[1]) {
                        case USBEH_DESCRIPTOR_TYPE_ENDPOINT:
                            ed = (USBEH_endpointDescriptor *)d;
                            //printf("  Adding endpoint 0x%02x for interface %d\r\n", ed->bEndpointAddress, id->bInterfaceNumber);
                            addEndpoint(device, ed);
                            break;
                        default:
                            // Skip unknown descriptor.
                            //printf("  Unknown descriptor type: %02x\r\n", d[1]);
                            break;
                    }
                    d += d[0];
                }
            }
        }
        else {
            d += d[0];
        }
    }
    
    if (interfaceCounter == 0) {
        return USBEH_ERR_INTERFACE_NOT_FOUND;
    }
    
    usbeh_api_on_load_device(device, desc, found);
    
    return 0;
}

void USBEH_Controller::processDoneQueue(USBEH_U32 tdList) {
    USBEH_Endpoint *endpoint;
    USBEH_HCTD *list = reverse((USBEH_HCTD *)tdList);
    while (list) {
        endpoint = (USBEH_Endpoint *)(list - 1);
        list = (USBEH_HCTD *)list->next;
        int ep = endpoint->address();
        bool in = endpoint->flags & 0x80;
        int status = (endpoint->tdHead.control >> 28) & 0xF;
        
        if (status != 0) {
            endpoint->currentState = USBEH_Endpoint::idle;
        }
        else {
            switch (endpoint->currentState) {
                case USBEH_Endpoint::setupQueued:
                    if (endpoint->length == 0) {
                        transfer(endpoint, in ? USBEH_TOKEN_OUT : USBEH_TOKEN_IN, 0, 0, USBEH_Endpoint::statusQueued);
                    }
                    else {
                        transfer(endpoint, in ? USBEH_TOKEN_IN : USBEH_TOKEN_OUT, (USBEH_U08 *)endpoint->data, endpoint->length, USBEH_Endpoint::dataQueued);
                    }
                    break;
                    
                case USBEH_Endpoint::dataQueued:
                    if (endpoint->tdHead.currentBufferPointer) {
                        endpoint->length = endpoint->tdHead.currentBufferPointer - (USBEH_U32)endpoint->data;
                    }
                    
                    if (ep == 0) {
                        transfer(endpoint, in ? USBEH_TOKEN_OUT : USBEH_TOKEN_IN, 0, 0, USBEH_Endpoint::statusQueued);
                    }
                    else {
                        endpoint->currentState = USBEH_Endpoint::idle;    
                    }
                    break;
                    
                case USBEH_Endpoint::statusQueued:
                    endpoint->currentState = USBEH_Endpoint::idle;    
                    break;
            }
        }
        
        if (endpoint->callback && endpoint->currentState == USBEH_Endpoint::idle) {
            //if (endpoint->address() != 0x81) printf("\r\nCallback pending for 0x%02X\r\n", endpoint->address());
            endpoint->currentState = USBEH_Endpoint::callbackPending;
            this->callbacksPending++;
        }
    }
}

void USBEH_Controller::resetPort(int hub, int port) {
    this->connectPending++;
    if (hub == 0) {
        USBEH_HcRhPortStatus1 = USBEH_PORT_RESET_STATUS;
    }
    else {
        usbeh_api_set_port_reset(hub, port);
    }
}

void USBEH_Controller::connect(int hub, int port, bool lowSpeed) {
    #ifdef DEBUG_USB_DRIVER
    debug_printf("%s called at line %d\r\n", __FUNCTION__, __LINE__);
    #endif
    addDevice(hub, port, lowSpeed);
}

void USBEH_Controller::disconnect(int hub, int port) {
    for (int i = 0; i < USBEH_MAX_DEVICES; i++) {
        USBEH_Device *dev = this->devices + i;
        if (dev->port == port && dev->hub == hub) {
            for (int p = 0; p < dev->hubPortCount; p++) {
                disconnect(i + 1, p + 1);
            }
            for (int j = 1; j < USBEH_MAX_ENDPOINTS_PER_DEVICE * 2; j += 2) {
                USBEH_U08 endpointIndex = dev->endpoints[j];
                if (endpointIndex != 0xFF) {
                    release(this->endpoints + endpointIndex);
                }
                dev->port = 0;
                dev->flags = 0;
                return;
            }
        }
    }
}

void USBEH_Controller::hubStatusChange(int hub, int port, USBEH_U32 status) {
    if (status & USBEH_CONNECT_STATUS_CHANGE) {
        if (status & USBEH_CURRENT_CONNECT_STATUS) {
            resetPort(hub, port);
        }
        else {
            disconnect(hub, port);
        }
    }
    
    if (status & USBEH_PORT_RESET_STATUS_CHANGE) {
        if (!(status & USBEH_PORT_RESET_STATUS)) {
            this->connectCountdown = 200;
            if (status & USBEH_LOW_SPEED_DEVICE) {
                port |= 0x80;
            }
            this->connectHub = hub;
            this->connectPort = port;
        }
    }
}

void USBEH_Controller::delayMS(int ms) {
    USBEH_U16 f = ms + hcca.frameNumber;
    while (f != hcca.frameNumber) ;
}

void USBEH_Controller::initHW(USBEH_HCCA *cca) {
    NVIC_DisableIRQ(USB_IRQn);
    
    LPC_SC->PCONP           |= (1UL << 31);
    LPC_USB->USBClkCtrl     |= USBEH_CLOCK_MASK;
    while ((LPC_USB->USBClkSt & USBEH_CLOCK_MASK) != USBEH_CLOCK_MASK);
    
    USBEH_OTGStCtrl |= 1;
    USBEH_USBClkCtrl &= ~USBEH_PORTSEL_CLK_EN;
    
    LPC_PINCON->PINSEL1 &= ~( (3<<26) | (3<<28) );    
    LPC_PINCON->PINSEL1 |=  ( (1<<26) | (1<<28));
    
    USBEH_HcControl = 0;
    USBEH_HcControlHeadED = 0;
    USBEH_HcBulkHeadED = 0;
    USBEH_HcCommandStatus = USBEH_HOST_CONTROLLER_RESET;
    USBEH_HcFmInterval = USBEH_DEFAULT_FMINTERVAL;
    USBEH_HcPeriodicStart = USBEH_FRAMEINTERVAL * 90 / 100;
    
    USBEH_HcControl = (USBEH_HcControl & (~USBEH_HOST_CONTROLLER_FUNCTIONAL_STATE)) | USBEH_OPERATIONAL_MASK;
    USBEH_HcRhStatus = USBEH_SET_GLOBAL_POWER;
    
    USBEH_HcHCCA = (USBEH_U32)cca;
    USBEH_HcInterruptStatus |= USBEH_HcInterruptStatus;
    USBEH_HcInterruptEnable |= USBEH_MASTER_IRQ_ENABLE | USBEH_WRITEBACK_DONE_HEAD | USBEH_ROOT_HUB_STATUS_CHANGE | USBEH_FRAME_NUMBER_OVERFLOW | USBEH_START_OF_FRAME;
    
    NVIC_EnableIRQ(USB_IRQn);
    while (cca->frameNumber < 10);
}

USBEH_HCTD * USBEH_Controller::reverse(USBEH_HCTD *current) {
    USBEH_HCTD *result = NULL, *temp;
    while (current) {
        temp = (USBEH_HCTD *)current->next;
        current->next = (USBEH_U32)result;
        result = current;
        current = temp;
    }
    return result;
}

USBEH_Endpoint * USBEH_Controller::getEndpoint(int device, int ep) {
    if (device == 0) {
        return &endpointZero;
    }
    if (device > USBEH_MAX_DEVICES) {
        return 0;
    }
    int i = devices[device - 1].getEndpointIndex(ep);
    if (i == -1) {
        return 0;
    }
    return endpoints + i;
}

/* The controller is defined withinn the API section. */
extern USBEH_Controller sys_usb_controller;

USBEH_SOF_COUNTER   *sof_counter_head = NULL;

/* The USB interrupt handler. */
extern "C" void USB_IRQHandler (void) __irq {

    USBEH_U32 int_status = USBEH_HcInterruptStatus;
  
    if (int_status & USBEH_ROOT_HUB_STATUS_CHANGE) {
        sys_usb_controller.rootHubStatusChange++;  
    }

    USBEH_U32 head = 0;
    
    if (int_status & USBEH_WRITEBACK_DONE_HEAD) {
        head = sys_usb_controller.hcca.doneHead;
        sys_usb_controller.hcca.doneHead = 0;
    }
    
    if (int_status & USBEH_START_OF_FRAME) {
        for (USBEH_SOF_COUNTER *list = sof_counter_head; list != NULL; list = list->next) {
            if (list->mode & USBEH_SOF_COUNTER_DEC && list->flag == 0) {
                if (list->counter > 0) list->counter--;
                if (list->counter == 0) {
                    list->flag = 1;
                    list->counter = list->reload;
                    if (list->callback != NULL) {
                        (list->callback)((struct _sof_counter *)list);
                    }
                }   
            }
        }
    }
                 
    USBEH_HcInterruptStatus = int_status;

    if (head) {
        sys_usb_controller.processDoneQueue(head);
    }
}

