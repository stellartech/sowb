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
 
#ifndef XBOX360GAMEPAD_H
#define XBOX360GAMEPAD_H

#include "usbeh_api.h"

#define BUTTON_PRESSED      1
#define BUTTON_RELEASED     0

#define BUTTON_HOLD_TIME    3000

#define BUTT_LS_PRESS                 1
#define BUTT_RS_PRESS                 2
#define BUTT_XBOX_PRESS               3
#define BUTT_UNUSED_PRESS             4
#define BUTT_A_PRESS                  5
#define BUTT_B_PRESS                  6
#define BUTT_X_PRESS                  7
#define BUTT_Y_PRESS                  8
#define BUTT_DPAD_UP_PRESS            9
#define BUTT_DPAD_DOWN_PRESS          10
#define BUTT_DPAD_LEFT_PRESS          11
#define BUTT_DPAD_RIGHT_PRESS         12
#define BUTT_START_PRESS              13
#define BUTT_BACK_PRESS               14
#define BUTT_LEFT_HAT_PRESS           15
#define BUTT_RIGHT_HAT_PRESS          16

#define BUTT_LS_RELEASE                 (BUTT_LS_PRESS + 16)
#define BUTT_RS_RELEASE                 (BUTT_RS_PRESS + 16)
#define BUTT_XBOX_RELEASE               (BUTT_XBOX_PRESS + 16)
#define BUTT_UNUSED_RELEASE             (BUTT_UNUSED_PRESS + 16)
#define BUTT_A_RELEASE                  (BUTT_A_PRESS + 16)
#define BUTT_B_RELEASE                  (BUTT_B_PRESS + 16)
#define BUTT_X_RELEASE                  (BUTT_X_PRESS + 16)
#define BUTT_Y_RELEASE                  (BUTT_Y_PRESS + 16)
#define BUTT_DPAD_UP_RELEASE            (BUTT_DPAD_UP_PRESS + 16)
#define BUTT_DPAD_DOWN_RELEASE          (BUTT_DPAD_DOWN_PRESS + 16)
#define BUTT_DPAD_LEFT_RELEASE          (BUTT_DPAD_LEFT_PRESS + 16)
#define BUTT_DPAD_RIGHT_RELEASE         (BUTT_DPAD_RIGHT_PRESS + 16)
#define BUTT_START_RELEASE              (BUTT_START_PRESS + 16)
#define BUTT_BACK_RELEASE               (BUTT_BACK_PRESS + 16)
#define BUTT_LEFT_HAT_RELEASE           (BUTT_LEFT_HAT_PRESS + 16)
#define BUTT_RIGHT_HAT_RELEASE          (BUTT_RIGHT_HAT_PRESS + 16)

#define BUTT_LS_HOLD                    (BUTT_LS_PRESS + 32)
#define BUTT_RS_HOLD                    (BUTT_RS_PRESS + 32)
#define BUTT_XBOX_HOLD                  (BUTT_XBOX_PRESS + 32)
#define BUTT_UNUSED_HOLD                (BUTT_UNUSED_PRESS + 32)
#define BUTT_A_HOLD                     (BUTT_A_PRESS + 32)
#define BUTT_B_HOLD                     (BUTT_B_PRESS + 32)
#define BUTT_X_HOLD                     (BUTT_X_PRESS + 32)
#define BUTT_Y_HOLD                     (BUTT_Y_PRESS + 32)
#define BUTT_DPAD_UP_HOLD               (BUTT_DPAD_UP_PRESS + 32)
#define BUTT_DPAD_DOWN_HOLD             (BUTT_DPAD_DOWN_PRESS + 32)
#define BUTT_DPAD_LEFT_HOLD             (BUTT_DPAD_LEFT_PRESS + 32)
#define BUTT_DPAD_RIGHT_HOLD            (BUTT_DPAD_RIGHT_PRESS + 32)
#define BUTT_START_HOLD                 (BUTT_START_PRESS + 32)
#define BUTT_BACK_HOLD                  (BUTT_BACK_PRESS + 32)
#define BUTT_LEFT_HAT_HOLD              (BUTT_LEFT_HAT_PRESS + 32)
#define BUTT_RIGHT_HAT_HOLD             (BUTT_RIGHT_HAT_PRESS + 32)

#define LED_ALL_OFF                     0x00
#define LED_ALL_BLINKING                0x01
#define LED_1_FLASH_THEN_ON             0x02
#define LED_2_FLASH_THEN_ON             0x03
#define LED_3_FLASH_THEN_ON             0x04
#define LED_4_FLASH_THEN_ON             0x05
#define LED_1_ON                        0x06
#define LED_2_ON                        0x07
#define LED_3_ON                        0x08
#define LED_4_ON                        0x09
#define LED_ROTATING                    0x0A
#define LED_BLINKING                    0x0B
#define LED_SLOW_BLINKING               0x0C
#define LED_ALTERNATING                 0x0D

typedef struct {
  uint16_t    idVendor;
  uint16_t    idProduct;
  const char*       name;
} XBOX360_DEVICE;

typedef struct {
    unsigned            state;
    unsigned            mask;
    unsigned            count;
    USBEH_SOF_COUNTER   pressHold;
} XBOX360_BUTTON;

typedef struct {
    short x;
    short y;
    short x_previous;
    short y_previous;
} XBOX360_STICK;

/* API functions. */
char xbox360gamepad_get_button(void);
char xbox360gamepad_get_button_preview(void);
unsigned char xbox360gamepad_get_trigger_left(void);
unsigned char xbox360gamepad_get_trigger_right(void);
void xbox360gamepad_led(int code);
XBOX360_STICK * xbox360gamepad_get_stick_left(void);
XBOX360_STICK * xbox360gamepad_get_stick_right(void);

int xbox360gamepad_init(void);
void xbox360gamepad_process(void);

/* Button press and hold callback function. */
void xbox360gamepad_button_hold_callback(USBEH_SOF_COUNTER *q);

/* Onload callback function. */
int xbox360gamepad_onload_callback(int device, USBEH_deviceDescriptor *deviceDesc, USBEH_interfaceDescriptor **interfaceDesc);

void xbox360_chatpad_init(void);

#endif
