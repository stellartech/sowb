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
#include "debug.h"
#include "usbeh_api.h"
#include "usbeh_endpoint.h"
#include "usbeh_device.h"
#include "usbeh_controller.h"
#include "xbox360gamepad.h"

#include "main.h"

/* Define an array of data structures for the buttons. */
static XBOX360_BUTTON buttons[16];
static unsigned char button_buffer[16];
static unsigned char button_buffer_in;
static unsigned char button_buffer_out;

/* Place holders for left and right triggers. */
static unsigned char trigger_left;
static unsigned char trigger_right;

static XBOX360_STICK stick_left;
static XBOX360_STICK stick_right;

/* A map of wired Xbox 360 controllers. 
   Data copied from the Linux xboxdrv project, thx :) 
   http://pingus.seul.org/~grumbel/xboxdrv */
static XBOX360_DEVICE xbox360_devices[] = {
    { 0x045e, 0x028e, "Microsoft Xbox 360 Controller" },
    { 0x0738, 0x4716, "Mad Catz Xbox 360 Controller" },
    { 0x0738, 0x4726, "Mad Catz Xbox 360 Controller" },
    { 0x0738, 0x4740, "Mad Catz Beat Pad" },
    { 0x0738, 0xf738, "Super SFIV FightStick TE S" },
    { 0x0f0d, 0x000a, "Hori Co. DOA4 FightStick" },
    { 0x0f0d, 0x000d, "Hori Fighting Stick Ex2" },
    { 0x0f0d, 0x0016, "Hori Real Arcade Pro Ex" },
    { 0x162e, 0xbeef, "Joytech Neo-Se Take2" },
    { 0x046d, 0xc242, "Logitech ChillStream" },
    { 0x0738, 0xcb03, "Saitek P3200 Rumble Pad - PC/Xbox 360" },
    { 0x0e6f, 0x0201, "Pelican TSZ360 Pad" },
    { 0x0738, 0xb726, "Mad Catz Xbox controller - MW2" }, // Here's one I found that isn't in the Linux distro.
    { 0x0000, 0x0000, "End of list" }
};

/* Although Microsoft has not published any data or specifications
   regarding the Xbox360 gamepad/controller, it's basic makeup is
   well known. Define here the pipes for the interfaces we are
   interested in. */

#define XBOX360_GAMEPAD_PIPE_BUFFER_SIZE    32

typedef struct {
    USBEH_U08   data_in[XBOX360_GAMEPAD_PIPE_BUFFER_SIZE]; 
    USBEH_U08   data_out[XBOX360_GAMEPAD_PIPE_BUFFER_SIZE];
    USBEH_U08   ep_number_in;
    USBEH_U08   ep_number_out;
} XBOX360_PIPE;

typedef struct {
    USBEH_U08   data_in[256]; 
    USBEH_U08   data_out[256];
    USBEH_U08   ep_number_in;
    USBEH_U08   ep_number_out;
} XBOX360_CONTROL_PIPE;

struct _xbox360_gamepad_vendor {
    XBOX360_CONTROL_PIPE    pipe[1];
} IF_VENDOR __attribute__((aligned(256)));
   
struct _xbox360_gamepad_interface0 {
    XBOX360_PIPE    pipe[1];
} IF_ZERO __attribute__((aligned(256)));

struct _xbox360_gamepad_interface1 {
    XBOX360_PIPE    pipe[2];
} IF_ONE __attribute__((aligned(256)));

struct _xbox360_gamepad_interface2 {
    XBOX360_PIPE    pipe[1];
} IF_TWO __attribute__((aligned(256)));

int deviceNumber;

int xbox360gamepad_init(void) {
    DEBUG_INIT_START;
    for (int i = 0; i < 16; i++) {
        buttons[i].state = BUTTON_RELEASED;
        buttons[i].count = 0;
        button_buffer[i] = 0xFF;
        usbeh_sof_counter_init(&buttons[i].pressHold, USBEH_SOF_COUNTER_DEC | USBEH_SOF_COUNTER_RELOAD, BUTTON_HOLD_TIME);
        buttons[i].pressHold.userData = i;
        buttons[i].pressHold.callback = xbox360gamepad_button_hold_callback;
        usbeh_sof_counter_register(&buttons[i].pressHold);
    }
    button_buffer_in  = 0;
    button_buffer_out = 0;
    trigger_left      = 0;
    trigger_right     = 0;
    stick_left.x      = 0;
    stick_left.y      = 0;
    stick_right.x     = 0;
    stick_right.y     = 0;
    DEBUG_INIT_END;
    return 0;
}

void xbox360gamepad_process(void) {
    // This currently does nothing.  
}

void xbox360gamepad_button_press(unsigned char button) {
    button_buffer[button_buffer_in] = button;
    button_buffer_in++;
    button_buffer_in &= 0x0F;
}

char xbox360gamepad_get_button(void) {
    if (button_buffer_in == button_buffer_out) {
        return 0;
    }
    
    char button = button_buffer[button_buffer_out];
    button_buffer_out++;
    button_buffer_out &= 0x0F;
    return button;
}

char xbox360gamepad_get_button_preview(void) {
    if (button_buffer_in == button_buffer_out) {
        return 0;
    }
    
    char button = button_buffer[button_buffer_out];
    return button;
}

unsigned char xbox360gamepad_get_trigger_left(void) {
    return trigger_left;
}

unsigned char xbox360gamepad_get_trigger_right(void) {
    return trigger_right;
}

XBOX360_STICK * xbox360gamepad_get_stick_left(void) {
    return &stick_left;
}

XBOX360_STICK * xbox360gamepad_get_stick_right(void) {
    return &stick_right;
}

void xbox360gamepad_button_hold_callback(USBEH_SOF_COUNTER *q) {
    int i = (int)q->userData;
    if (buttons[i].state == BUTTON_PRESSED) {
        xbox360gamepad_button_press((char)(i + 1 + 32));
    }
}
    
/** 
 * xbox360gamepad_interface_0_in
 *
 * A callback function to handle interface0 pipe0 data.
 */
void xbox360gamepad_interface_0_in(int device, int endpoint, int status, USBEH_U08 *data, int len, void *userData) {
    unsigned button_flags;
    
    /* Is this a button press report? */
    if(IF_ZERO.pipe[(int)userData].data_in[0] == 0 && IF_ZERO.pipe[(int)userData].data_in[1] == 0x14) {
        
        /* Handle the button flags. */
        button_flags = ((IF_ZERO.pipe[(int)userData].data_in[2] & 0xFF) << 8) | (IF_ZERO.pipe[(int)userData].data_in[3] & 0xFF);
        for (int i = 0; i < 16; i++) {
            if ((button_flags & (1 << i)) != 0) {
                if (buttons[i].state == BUTTON_RELEASED) {
                    buttons[i].state = BUTTON_PRESSED;
                    buttons[i].count++;
                    buttons[i].pressHold.flag = 0;
                    xbox360gamepad_button_press((char)(i + 1));
                }
            }
            else {
                if (buttons[i].state == BUTTON_PRESSED) {
                    buttons[i].state = BUTTON_RELEASED;
                    buttons[i].pressHold.flag = 1;
                    buttons[i].pressHold.counter = BUTTON_HOLD_TIME;
                    buttons[i].count = 0;
                    xbox360gamepad_button_press((char)(i + 1 + 16));
                }
            }
        }
        
        /* Handle the analogue triggers. */
        trigger_left  = (unsigned char)IF_ZERO.pipe[(int)userData].data_in[4];
        trigger_right = (unsigned char)IF_ZERO.pipe[(int)userData].data_in[5];
        
        /* Handle the analogue sticks. */
        {
            short x, y;
            x = (short)((IF_ZERO.pipe[(int)userData].data_in[6])  | IF_ZERO.pipe[(int)userData].data_in[7]  << 8);
            y = (short)((IF_ZERO.pipe[(int)userData].data_in[8])  | IF_ZERO.pipe[(int)userData].data_in[9]  << 8);
            if (x != stick_left.x_previous) {
                stick_left.x_previous = stick_left.x;
                stick_left.x = x;
            }
            if (y != stick_left.y_previous) {
                stick_left.y_previous = stick_left.y;
                stick_left.y = y;
            }
            x = (short)((IF_ZERO.pipe[(int)userData].data_in[10]) | IF_ZERO.pipe[(int)userData].data_in[11] << 8);
            y = (short)((IF_ZERO.pipe[(int)userData].data_in[12]) | IF_ZERO.pipe[(int)userData].data_in[13] << 8);
            if (x != stick_right.x_previous) {
                stick_right.x_previous = stick_right.x;
                stick_right.x = x;
            }
            if (y != stick_right.y_previous) {
                stick_right.y_previous = stick_right.y;
                stick_right.y = y;
            }
        }
    }
    else if(len == 3) {
        /* Chatpad return? */
        #ifdef DEBUG_USB_DRIVER_IF_0
        debug_printf("Got %02x %02x %02x \r\n", buf[0], buf[1], buf[2]);
        #endif
    }
    else {
        #ifdef DEBUG_USB_DRIVER_IF_0
        DEBUG_UNKNOWN_PACKET_IF_0
        #endif
    }
    
    /* Schedule another transfer to keep the IN flow of data coming. */
    usbeh_api_interrupt_transfer(device, endpoint, IF_ZERO.pipe[(int)userData].data_in, XBOX360_GAMEPAD_PIPE_BUFFER_SIZE, xbox360gamepad_interface_0_in, userData);
}

void xbox360gamepad_interface_1_in(int device, int endpoint, int status, USBEH_U08 *data, int len, void *userData) {
    /* We don't currently do anything with Interface:1 */
    #ifdef DEBUG_USB_DRIVER_IF_1
    DEBUG_UNKNOWN_PACKET_IF_1
    #endif
    
    /* Schedule another transfer to keep the IN flow of data coming. */
    usbeh_api_interrupt_transfer(device, endpoint, IF_ONE.pipe[(int)userData].data_in, XBOX360_GAMEPAD_PIPE_BUFFER_SIZE, xbox360gamepad_interface_1_in, userData);
}

void xbox360gamepad_interface_2_in(int device, int endpoint, int status, USBEH_U08 *data, int len, void *userData) {
    /* We don't currently do anything with Interface:2 */
    #ifdef DEBUG_USB_DRIVER_IF_2
    DEBUG_UNKNOWN_PACKET_IF_2
    #endif
    
    /* Schedule another transfer to keep the IN flow of data coming. */
    usbeh_api_interrupt_transfer(device, endpoint, IF_TWO.pipe[(int)userData].data_in, XBOX360_GAMEPAD_PIPE_BUFFER_SIZE, xbox360gamepad_interface_2_in, userData);
}

int xbox360gamepad_add_interface(int device, USBEH_interfaceDescriptor *iface, USBEH_endpointDescriptor *ed) {
    
    /* Handle interrupt IN transfers. */
    if ((ed->bmAttributes & 3) != USBEH_ENDPOINT_INTERRUPT) {
        return 0;
    }
    
    if (iface->bInterfaceNumber == 0) {
        int pipe = 0;
        if (ed->bEndpointAddress & 0x80) {
            #ifdef DEBUG_USB_DRIVER
            DEBUG_USB_MSG_ALLOCATION
            #endif
            IF_ZERO.pipe[pipe].ep_number_in = ed->bEndpointAddress;
            usbeh_api_interrupt_transfer(device, ed->bEndpointAddress, IF_ZERO.pipe[0].data_in, XBOX360_GAMEPAD_PIPE_BUFFER_SIZE, xbox360gamepad_interface_0_in, (void *)pipe);
        }
        else {
            #ifdef DEBUG_USB_DRIVER
            DEBUG_USB_MSG_ALLOCATION
            #endif
            IF_ZERO.pipe[pipe].ep_number_out = ed->bEndpointAddress;
        }
    }
    
    if (iface->bInterfaceNumber == 1) {
        int pipe = (IF_ONE.pipe[0].ep_number_in == 0) ? 0 : 1;
        if (ed->bEndpointAddress & 0x80) {
            IF_ONE.pipe[pipe].ep_number_in = ed->bEndpointAddress;
            #ifdef DEBUG_USB_DRIVER
            DEBUG_USB_MSG_ALLOCATION
            #endif
            usbeh_api_interrupt_transfer(device, ed->bEndpointAddress, IF_ONE.pipe[pipe].data_in, XBOX360_GAMEPAD_PIPE_BUFFER_SIZE, xbox360gamepad_interface_1_in, (void *)pipe);
        }
        else {
            #ifdef DEBUG_USB_DRIVER
            DEBUG_USB_MSG_ALLOCATION
            #endif
            IF_ONE.pipe[pipe].ep_number_out = ed->bEndpointAddress;
        }
    }
    
    
    if (iface->bInterfaceNumber == 2) {
        int pipe = (IF_TWO.pipe[0].ep_number_in == 0) ? 0 : 1;
        if (ed->bEndpointAddress & 0x80) {
            IF_TWO.pipe[pipe].ep_number_in = ed->bEndpointAddress;
            #ifdef DEBUG_USB_DRIVER
            DEBUG_USB_MSG_ALLOCATION
            #endif
            usbeh_api_interrupt_transfer(device, ed->bEndpointAddress, IF_TWO.pipe[pipe].data_in, XBOX360_GAMEPAD_PIPE_BUFFER_SIZE, xbox360gamepad_interface_2_in, (void *)pipe);
        }
        else {
            #ifdef DEBUG_USB_DRIVER
            DEBUG_USB_MSG_ALLOCATION
            #endif
            IF_TWO.pipe[pipe].ep_number_out = ed->bEndpointAddress;
        }
    }
       
    return 1;
}

static XBOX360_DEVICE * xbox360gamepad_vendor_product_check(uint16_t vendorId, uint16_t productId) {
    for (int i = 0; xbox360_devices[i].idVendor != 0; i++) {
        if (xbox360_devices[i].idVendor == vendorId && xbox360_devices[i].idProduct == productId) {
            return &xbox360_devices[i];
        }
    }
    return (XBOX360_DEVICE *)NULL;
}

void xbox360gamepad_led(int code) {
    IF_ZERO.pipe[0].data_out[0] = 0x01;
    IF_ZERO.pipe[0].data_out[1] = 0x03;
    IF_ZERO.pipe[0].data_out[2] = code;
    usbeh_api_interrupt_transfer(deviceNumber, IF_ZERO.pipe[0].ep_number_out, IF_ZERO.pipe[0].data_out, 3, NULL, NULL);
}

USBEH_U08 noise[] = "\x0\x8\x0\x0\x0\x0\x0\x0";

void xbox360_noise(int device, int endpoint, int status, USBEH_U08 *data, int len, void *userData) {
    usbeh_api_interrupt_transfer(deviceNumber, IF_ZERO.pipe[0].ep_number_out, noise, 8, xbox360_noise, NULL);
}

int xbox360gamepad_onload_callback(int device, USBEH_deviceDescriptor *deviceDesc, USBEH_interfaceDescriptor **interfaceDesc) {
    USBEH_interfaceDescriptor *iface;
    int length, endpoint_count, interfaceCounter = 0;
    USBEH_U08 *start, *end;
    
    /* Is this a Microsoft Xbox360 wired Gamepad? */
    XBOX360_DEVICE *xd = xbox360gamepad_vendor_product_check(deviceDesc->idVendor, deviceDesc->idProduct);
    if (!xd) {
        /* If not, don't claim it. */
        return 0;
    }
    
    #ifdef DEBUG_USB_DRIVER
    debug_printf("Found device '%s'\r\n", xd->name);
    #endif
    
    /* Save our device number in a global. */
    deviceNumber = device;
    
    memset((char *)&IF_VENDOR, 0, sizeof(IF_VENDOR));
    memset((char *)&IF_ZERO, 0, sizeof(IF_ZERO));
    memset((char *)&IF_ONE, 0, sizeof(IF_ONE));
    memset((char *)&IF_TWO, 0, sizeof(IF_TWO));
    
    #ifdef DEBUG_USB_DRIVER
    debug_printf("%s() ACTIVATED for device id = %d\r\n", __FUNCTION__, device);
    debug_printf("  OnLoad VendorId = 0x%04x ProductId = 0x%04x \r\n", deviceDesc->idVendor, deviceDesc->idProduct);
    #endif
        
    /* Parse the interface configuration and setup the endpoint handlers. */
    while ((iface = interfaceDesc[interfaceCounter]) != (USBEH_interfaceDescriptor *)NULL) { 
        #ifdef DEBUG_USB_DRIVER
        debug_printf("  interface%d:- \r\n", interfaceCounter);
        debug_printf("    InterfaceClass    = %02x \r\n", iface->bInterfaceClass);
        debug_printf("    InterfaceSubClass = %02x \r\n", iface->bInterfaceSubClass);
        debug_printf("    InterfaceProtocol = %02x \r\n", iface->bInterfaceProtocol);
        #endif
        start = (USBEH_U08 *)iface;
        length = start[0];
        end   = start + length; 
        #ifdef DEBUG_USB_DRIVER
        debug_printf("  Scanning at start:%08x length:%d end:%08x \r\n", start, length, end);
        #endif
        while (start < end) {
            if (start[1] == USBEH_DESCRIPTOR_TYPE_INTERFACE) {
                USBEH_interfaceDescriptor *id = (USBEH_interfaceDescriptor *)start;
                //int interfaceNumber = (int)id->bInterfaceNumber;                
                endpoint_count = (int)id->bNumEndpoints;
                #ifdef DEBUG_USB_DRIVER
                debug_printf("  found definition for if:%d with %d endpoints.\r\n", interfaceNumber, endpoint_count);
                #endif
                start += start[0];
                while (endpoint_count > 0) { 
                    if (start[1] == USBEH_DESCRIPTOR_TYPE_ENDPOINT) {
                        USBEH_endpointDescriptor *ed = (USBEH_endpointDescriptor *)start;
                        #ifdef DEBUG_USB_DRIVER
                        debug_printf("  found endpoint 0x%02X\r\n", ed->bEndpointAddress);
                        #endif
                        xbox360gamepad_add_interface(device, id, ed);
                        endpoint_count--;
                    }
                    else {
                        #ifdef DEBUG_USB_DRIVER
                        debug_printf("  no endpoint for interface, descriptor class: %02X\r\n", start[1]);
                        #endif
                    }                    
                    start += start[0];
                }
            }
            else {
                #ifdef DEBUG_USB_DRIVER
                debug_printf("  no IF descr found at %08x\r\n", start);
                #endif
                start += start[0];
            } 
        }
        
        interfaceCounter++;
    }
    
    #ifdef DEBUG_USB_DRIVER
    debug_printf("Device claimed by %s().\r\n\n", __FUNCTION__);
    #endif
            
    /* When the gamepad boots up, the LED ring flashes after it's
       configuration is set. Let's flash the led and go steady. */
    xbox360gamepad_led(LED_1_FLASH_THEN_ON);
    
    /* Lets just send "noise" to the headset and see what happens. */
    //usbeh_api_interrupt_transfer(deviceNumber, IF_ZERO.pipe[0].ep_number_out, noise, 8, xbox360_noise, NULL);
    
    /* I give up, wtf is it with this chatpad that MS needs to keep so secret? */
    //xbox360_chatpad_init();
    
    /* Tell the USB embedded host that we have claimed this device. */
    return 1; 
}

#ifdef NEVERCOMPILETHISCODE
/* 

UPDATE: Everything below is ABANDONED when I basically figured out from one replay to the next
that MS use the security channel to "handshake" init the chatpad. Many people believe
the security device in the gamepad is to "tell the xbox I am a real auth MS gamepad". However,
as I have discovered, it's "two-way". There also exists the "tell the gamepad that an auth
Xbox360 is the host" and without that the chatpad will not function. I have no interest in
trying to crack anyones security chips so no further work will be done on this.

Everything below here is related to the chatpad attachment for the Xbox360 gamepad controller.
Unlike the sticks, triggers and buttons, the chatpad appears to require some sort of initialization
in order to start running. Since MS have not published any sort of USB protocol specifications
for their gamepad, what we do here is basically a "replay attack" based on packets sent and received
between an Xbox and a gamepad with a USB protocol analyser in the middle. A classic man-in-the-middle
type of attack.

Here's the basic sequence we attempt to follow:-

 1. -> Vendor request IN EP0 (0x01) 
       -> SETUP C0 01 00 00 00 00 04 00
       <- IN    80 03 0D 47
 2. -> Vendor request OUT EP0 (0xA9)
       -> 40 A9 0C A3 23 44 00
       <- Expect STALLED no data
 3. -> Vendor request OUT EP0 (0xA9)
       -> 40 A9 44 23 03 7F 00 00
       <- Expect STALLED no data
 4. -> Vendor request OUT EP0 (0xA9)
       -> 40 A9 39 58 32 68 00 00
       <- Expect STALLED no data
 5. <- IN IF:0 EP2 3 bytes 01 03 0E
 6. -> Vendor request IN EP0 (0xA1) 
       -> SETUP C0 A1 00 00 16 E4 02 00
       <- IN    01 00
 7. -> OUT IF:0 EP1 3 bytes 01 03 01
 8. -> Vendor request OUT EP0 (0xA1)
       -> SETUP 40 A1 00 00 16 E4 02 00
       -> OUT   09 00                    
 9. -> Vendor request IN EP0 (0xA1)
       -> SETUP C0 A1 00 00 16 E4 02 00
       -> IN    09 00                   (echo previous OUT?)
10. <- IN IF:0 EP 1 3 bytes 02 03 00
11. <- IN IF:0 EP 1 3 bytes 03 03 03
12. <- IN IF:0 EP 1 3 bytes 08 03 00
13. <- IN IF:0 EP 1 3 bytes 01 03 01
14. -> Vendor request OUT EP0 (0x00)
       -> SETUP 41 00 1F 00 02 00 00 00
       -> IN No data
15. -> Vendor request OUT EP0 (0x00)
       -> SETUP 41 00 1E 00 02 00 00 00
       -> IN No data

Note that 14. and 15. are 1second apart and repeat continuely
toggling between 1F and 1E each second.

16. <- IN IF:2 EP4 (IF:2 PIPE1) 5 bytes F0 03 00 01 01
17. -> Vendor request OUT EP0 (0x00)
       -> SETUP 41 00 1B 00 02 00 00 00
       -> IN No data
18. <- IN IF:2 EP4 (IF:2 PIPE1) 5 bytes F0 03 00 01 01
19. -> Vendor request OUT EP0 (0x00)
       -> SETUP 41 00 1B 00 02 00 00 00
       -> IN No data


After this we start receiving data from IF:2 PIPE1
  
*/
USBEH_U08 chatpad_trash_buffer[256];
USBEH_U08 chatpad_toggle;
USBEH_U08 chatpad_send_toggle;
USBEH_U08 chatpad_toggle_1Bsent;
USBEH_SOF_COUNTER chatpad_toggle_timer;

void xbox360_chatpad_toggle(int device, int endpoint, int status, USBEH_U08 *data, int len, void *userData) {
    int result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_INTERFACE, 
        0x00, 
        (int)userData, 
        0x0002, 
        chatpad_trash_buffer, 
        0, 
        xbox360_chatpad_toggle, 
        (userData == (void *)0x1E) ? (void *)0x001F : (void *)0x001E);
    //debug.printf("TOGGLE = 0x%x\r\n", -result & 0xf);
}

void xbox360_chatpad_timed_toggle(USBEH_SOF_COUNTER *q) {
    int result;
   
    debug_printf("%s() called.\r\n", __FUNCTION__);
                
    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_INTERFACE, 
        0x00, 
        (int)q->userData == 0x1F ? 0x001F : 0x001E, 
        0x0002, 
        chatpad_trash_buffer,
        0, 
        0, 0);
    
    debug_printf("repeat! 1F/E = 0x%x\r\n", -result & 0xf);
    
    if (1 && !chatpad_toggle_1Bsent) {
        chatpad_toggle_1Bsent = 1;
        
        result = usbeh_api_control_transfer(deviceNumber, 
            USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_INTERFACE, 
            0x00, 
            0x001B, 
            0x0002, 
            chatpad_trash_buffer, 
            0, 
            0, 0);
    }
    
    debug_printf("repeat! 1B = 0x%x\r\n", -result & 0xf);
    
    //usbeh_sof_counter_unregister(&chatpad_toggle_timer);
    //usbeh_sof_counter_init(&chatpad_toggle_timer,  USBEH_SOF_COUNTER_DEC | USBEH_SOF_COUNTER_RELOAD, 1700);
    //chatpad_toggle_timer.userData = (int)q->userData == 0x1F ? 0x001F : 0x001E;
    //chatpad_toggle_timer.callback = xbox360_chatpad_timed;
    //chatpad_toggle_timer.flag = 0;
    //usbeh_sof_counter_register(&chatpad_toggle_timer);
    
    
}

void xbox360_chatpad_init(void) {
    int result;
    USBEH_U08   buffer[256];
    USBEH_U08   data[256];
    
    // int usbeh_api_control_transfer(int device, 
    //      int request_type, 
    //      int request, 
    //      int value, 
    //      int index, 
    //      USBEH_U08 *data, 
    //      int length, 
    //      USBEH_callback callback, void *userData) {
    // int r = usbeh_api_control_transfer(device, USBEH_DEVICE_TO_HOST | USBEH_REQUEST_TYPE_CLASS | USBEH_RECIPIENT_DEVICE, USBEH_GET_DESCRIPTOR, (USBEH_DESCRIPTOR_TYPE_HUB << 8), 0, buffer, sizeof(buffer), 0, 0);
    
    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_DEVICE_TO_HOST | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_DEVICE, 
        0x01, 
        0x0000, 
        0x0000, 
        buffer, 
        4, 
        0, 0);
    debug_printf("Step 1 = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);

    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_DEVICE, 
        0xA9, 
        0xA30C, 
        0x4423, 
        buffer, 
        0, 
        0, 0);
    debug_printf("Step 2 = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);

    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_DEVICE, 
        0xA9, 
        0x2344, 
        0x7F03, 
        buffer, 
        0, 
        0, 0);
    debug_printf("Step 3 = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);

    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_DEVICE, 
        0xA9, 
        0x5839, 
        0x6832, 
        buffer, 
        0, 
        0, 0);
    debug_printf("Step 4 = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);

    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_DEVICE_TO_HOST | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_DEVICE, 
        0xA1, 
        0x0000, 
        0xE416, 
        buffer, 
        2, 
        0, 0);
    debug_printf("Step 5 = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);
    
    data[0] = 9; data[1] = 0;
    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_DEVICE, 
        0xA1, 
        0x5839, 
        0xE416, 
        data, 
        2, 
        0, 0);
    debug_printf("Step 6 = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);

    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_DEVICE_TO_HOST | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_DEVICE, 
        0xA1, 
        0x0000, 
        0xE416, 
        buffer, 
        2, 
        0, 0);
    debug_printf("Step 7 = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);

    // Now begin toggling 1F / 1E
    chatpad_toggle = 0x1E;
    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_INTERFACE, 
        0x00, 
        0x001F, 
        0x0002, 
        chatpad_trash_buffer, 
        0, 
        xbox360_chatpad_toggle, 
        (void *)0x1E);
    debug_printf("Step 8 TOGGLE = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);
    
    result = usbeh_api_control_transfer(deviceNumber, 
        USBEH_HOST_TO_DEVICE | USBEH_SETUP_TYPE_MASK_VENDOR | USBEH_RECIPIENT_INTERFACE, 
        0x00, 
        0x0003, 
        0x0002, 
        chatpad_trash_buffer, 
        0, 
        0, 
        0);
    debug_printf("Step 9 OUT 0x03 = 0x%x %02x %02x %02x %02x \r\n", result, buffer[0], buffer[1], buffer[2], buffer[3]);
    
    //usbeh_sof_counter_init(&chatpad_toggle_timer,  USBEH_SOF_COUNTER_DEC | USBEH_SOF_COUNTER_RELOAD, 1700);
    //chatpad_toggle_timer.userData = (int)chatpad_toggle;
    //chatpad_toggle_timer.callback = xbox360_chatpad_timed;
    //usbeh_sof_counter_register(&chatpad_toggle_timer);

}

#endif
