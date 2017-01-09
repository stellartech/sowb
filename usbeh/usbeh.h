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
 
#ifndef USBEH_H
#define USBEH_H

#include "sowb.h"

/* Definitions */
#define USBEH_HcRevision            LPC_USB->HcRevision
#define USBEH_HcControl             LPC_USB->HcControl
#define USBEH_HcCommandStatus       LPC_USB->HcCommandStatus
#define USBEH_HcInterruptStatus     LPC_USB->HcInterruptStatus
#define USBEH_HcInterruptEnable     LPC_USB->HcInterruptEnable
#define USBEH_HcInterruptDisable    LPC_USB->HcInterruptDisable
#define USBEH_HcHCCA                LPC_USB->HcHCCA
#define USBEH_entED                 LPC_USB->entED
#define USBEH_HcControlHeadED       LPC_USB->HcControlHeadED
#define USBEH_HcControlCurrentED    LPC_USB->HcControlCurrentED
#define USBEH_HcBulkHeadED          LPC_USB->HcBulkHeadED
#define USBEH_HcBulkCurrentED       LPC_USB->HcBulkCurrentED
#define USBEH_HcDoneHead            LPC_USB->HcDoneHead
#define USBEH_HcFmInterval          LPC_USB->HcFmInterval
#define USBEH_HcFmRemaining         LPC_USB->HcFmRemaining
#define USBEH_HcFmNumber            LPC_USB->HcFmNumber
#define USBEH_HcPeriodicStart       LPC_USB->HcPeriodicStart
#define USBEH_HcLSTreshold          LPC_USB->HcLSTreshold
#define USBEH_HcRhDescriptorA       LPC_USB->HcRhDescriptorA
#define USBEH_HcRhDescriptorB       LPC_USB->HcRhDescriptorB
#define USBEH_HcRhStatus            LPC_USB->HcRhStatus
#define USBEH_HcRhPortStatus1       LPC_USB->HcRhPortStatus1
#define USBEH_HcRhPortStatus2       LPC_USB->HcRhPortStatus2
#define USBEH_Module_ID             LPC_USB->Module_ID
#define USBEH_OTGIntSt              LPC_USB->OTGIntSt
#define USBEH_OTGIntEn              LPC_USB->OTGIntEn
#define USBEH_OTGIntSet             LPC_USB->OTGIntSet
#define USBEH_OTGIntClr             LPC_USB->OTGIntClr
#define USBEH_OTGStCtrl             LPC_USB->OTGStCtrl
#define USBEH_OTGTmr                LPC_USB->OTGTmr
#define USBEH_USBDevIntSt           LPC_USB->USBDevIntSt
#define USBEH_USBDevIntEn           LPC_USB->USBDevIntEn
#define USBEH_USBDevIntClr          LPC_USB->USBDevIntClr
#define USBEH_USBDevIntSet          LPC_USB->USBDevIntSet
#define USBEH_USBCmdCode            LPC_USB->USBCmdCode
#define USBEH_USBCmdData            LPC_USB->USBCmdData
#define USBEH_USBRxData             LPC_USB->USBRxData
#define USBEH_USBTxData             LPC_USB->USBTxData
#define USBEH_USBRxPLen             LPC_USB->USBRxPLen
#define USBEH_USBTxPLen             LPC_USB->USBTxPLen
#define USBEH_USBCtrl               LPC_USB->USBCtrl
#define USBEH_USBDevIntPri          LPC_USB->USBDevIntPri
#define USBEH_USBEpIntSt            LPC_USB->USBEpIntSt
#define USBEH_USBEpIntEn            LPC_USB->USBEpIntEn
#define USBEH_USBEpIntClr           LPC_USB->USBEpIntClr
#define USBEH_USBEpIntSet           LPC_USB->USBEpIntSet
#define USBEH_USBEpIntPri           LPC_USB->USBEpIntPri
#define USBEH_USBReEp               LPC_USB->USBReEp
#define USBEH_USBEpInd              LPC_USB->USBEpInd
#define USBEH_USBMaxPSize           LPC_USB->USBMaxPSize
#define USBEH_USBDMARSt             LPC_USB->USBDMARSt
#define USBEH_USBDMARClr            LPC_USB->USBDMARClr
#define USBEH_USBDMARSet            LPC_USB->USBDMARSet
#define USBEH_USBUDCAH              LPC_USB->USBUDCAH
#define USBEH_USBEpDMASt            LPC_USB->USBEpDMASt
#define USBEH_USBEpDMAEn            LPC_USB->USBEpDMAEn
#define USBEH_USBEpDMADis           LPC_USB->USBEpDMADis
#define USBEH_USBDMAIntSt           LPC_USB->USBDMAIntSt
#define USBEH_USBDMAIntEn           LPC_USB->USBDMAIntEn
#define USBEH_USBEoTIntSt           LPC_USB->USBEoTIntSt
#define USBEH_USBEoTIntClr          LPC_USB->USBEoTIntClr
#define USBEH_USBEoTIntSet          LPC_USB->USBEoTIntSet
#define USBEH_USBNDDRIntSt          LPC_USB->USBNDDRIntSt
#define USBEH_USBNDDRIntClr         LPC_USB->USBNDDRIntClr
#define USBEH_USBNDDRIntSet         LPC_USB->USBNDDRIntSet
#define USBEH_USBSysErrIntSt        LPC_USB->USBSysErrIntSt
#define USBEH_USBSysErrIntClr       LPC_USB->USBSysErrIntClr
#define USBEH_USBSysErrIntSet       LPC_USB->USBSysErrIntSet
#define USBEH_I2C_RX                LPC_USB->I2C_RX
#define USBEH_I2C_WO                LPC_USB->I2C_WO
#define USBEH_I2C_STS               LPC_USB->I2C_STS
#define USBEH_I2C_CTL               LPC_USB->I2C_CTL
#define USBEH_I2C_CLKHI             LPC_USB->I2C_CLKHI
#define USBEH_I2C_CLKLO             LPC_USB->I2C_CLKLO
#define USBEH_USBClkCtrl            LPC_USB->USBClkCtrl
#define USBEH_OTGClkCtrl            LPC_USB->OTGClkCtrl
#define USBEH_USBClkSt              LPC_USB->USBClkSt
#define USBEH_OTGClkSt              LPC_USB->OTGClkSt

void user_wait_ms(uint32_t ms);
//#define USBEH_OS_DELAY_MS(x)    wait_ms(x);
#define USBEH_OS_DELAY_MS(x)    user_wait_ms(x);

#define USBEH_U32   uint32_t
#define USBEH_U16   unsigned short int
#define USBEH_U08   unsigned char
#define USBEH_S32   int32_t
#define USBEH_S16   short int
#define USBEH_S08   char

#define USBEH_MAX_ENDPOINTS_TOTAL       16
#define USBEH_MAX_DEVICES               8
#define USBEH_MAX_ENDPOINTS_PER_DEVICE  8

#define USBEH_ENDPOINT_CONTROL 0
#define USBEH_ENDPOINT_ISOCRONOUS 1
#define USBEH_ENDPOINT_BULK 2
#define USBEH_ENDPOINT_INTERRUPT 3

#define USBEH_DESCRIPTOR_TYPE_DEVICE        1
#define USBEH_DESCRIPTOR_TYPE_CONFIGURATION 2
#define USBEH_DESCRIPTOR_TYPE_STRING        3
#define USBEH_DESCRIPTOR_TYPE_INTERFACE     4
#define USBEH_DESCRIPTOR_TYPE_ENDPOINT      5

#define USBEH_DESCRIPTOR_TYPE_HID         0x21
#define USBEH_DESCRIPTOR_TYPE_REPORT      0x22
#define USBEH_DESCRIPTOR_TYPE_PHYSICAL    0x23
#define USBEH_DESCRIPTOR_TYPE_HUB         0x29

#define USBEH_HOST_CLK_EN                       (1 << 0)
#define USBEH_PORTSEL_CLK_EN                    (1 << 3)
#define USBEH_AHB_CLK_EN                        (1 << 4)
#define USBEH_CLOCK_MASK                        (USBEH_HOST_CLK_EN | USBEH_PORTSEL_CLK_EN | USBEH_AHB_CLK_EN)
#define USBEH_FRAMEINTERVAL                     (12000-1)    // 1ms
#define USBEH_DEFAULT_FMINTERVAL                ((((6 * (USBEH_FRAMEINTERVAL - 210)) / 7) << 16) | USBEH_FRAMEINTERVAL)
#define USBEH_HOST_CONTROLLER_RESET             0x01
#define USBEH_HOST_CONTROLLER_FUNCTIONAL_STATE  0xC0
#define USBEH_OPERATIONAL_MASK                  0x80
#define USBEH_SET_GLOBAL_POWER                  0x00010000

#define USBEH_WRITEBACK_DONE_HEAD       0x00000002
#define USBEH_START_OF_FRAME            0x00000004
#define USBEH_RESUME_DETECTED           0x00000008
#define USBEH_UNRECOVERABLE_ERROR       0x00000010
#define USBEH_FRAME_NUMBER_OVERFLOW     0x00000020
#define USBEH_ROOT_HUB_STATUS_CHANGE    0x00000040
#define USBEH_OWNERSHIP_CHANGE          0x00000080
#define USBEH_MASTER_IRQ_ENABLE         0x80000000
    
enum USB_CLASS_CODE {
    CLASS_DEVICE,
    CLASS_AUDIO,
    CLASS_COMM_AND_CDC_CONTROL,
    CLASS_HID,
    CLASS_PHYSICAL = 0x05,
    CLASS_STILL_IMAGING,
    CLASS_PRINTER,
    CLASS_MASS_STORAGE,
    CLASS_HUB,
    CLASS_CDC_DATA,
    CLASS_SMART_CARD,
    CLASS_CONTENT_SECURITY      = 0x0D,
    CLASS_VIDEO                 = 0x0E,
    CLASS_DIAGNOSTIC_DEVICE     = 0xDC,
    CLASS_WIRELESS_CONTROLLER   = 0xE0,
    CLASS_MISCELLANEOUS         = 0xEF,
    CLASS_APP_SPECIFIC          = 0xFE,
    CLASS_VENDOR_SPECIFIC       = 0xFF
};

#define USBEH_DEVICE_TO_HOST         0x80
#define USBEH_HOST_TO_DEVICE         0x00
#define USBEH_REQUEST_TYPE_CLASS     0x20
#define USBEH_RECIPIENT_DEVICE       0x00
#define USBEH_RECIPIENT_INTERFACE    0x01
#define USBEH_RECIPIENT_ENDPOINT     0x02
#define USBEH_RECIPIENT_OTHER        0x03

#define USBEH_SETUP_TYPE_MASK_STANDARD  0x00
#define USBEH_SETUP_TYPE_MASK_CLASS     0x20
#define USBEH_SETUP_TYPE_MASK_VENDOR    0x40
#define USBEH_SETUP_TYPE_MASK_RESERVED  0x60

#define USBEH_GET_STATUS            0
#define USBEH_CLEAR_FEATURE         1
#define USBEH_SET_FEATURE           3
#define USBEH_SET_ADDRESS           5
#define USBEH_GET_DESCRIPTOR        6
#define USBEH_SET_DESCRIPTOR        7
#define USBEH_GET_CONFIGURATION     8
#define USBEH_SET_CONFIGURATION     9
#define USBEH_GET_INTERFACE         10
#define USBEH_SET_INTERFACE         11
#define USBEH_SYNCH_FRAME           11

//    Status flags from hub
#define USBEH_PORT_CONNECTION       0
#define USBEH_PORT_ENABLE           1
#define USBEH_PORT_SUSPEND          2
#define USBEH_PORT_OVER_CURRENT     3
#define USBEH_PORT_RESET            4
#define USBEH_PORT_POWER            8
#define USBEH_PORT_LOW_SPEED        9

#define USBEH_C_PORT_CONNECTION     16
#define USBEH_C_PORT_ENABLE         17
#define USBEH_C_PORT_SUSPEND        18
#define USBEH_C_PORT_OVER_CURRENT   19
#define USBEH_C_PORT_RESET          20

#define USBEH_IO_PENDING                -100
#define USBEH_ERR_ENDPOINT_NONE_LEFT    -101
#define USBEH_ERR_ENDPOINT_NOT_FOUND    -102
#define USBEH_ERR_DEVICE_NOT_FOUND      -103
#define USBEH_ERR_DEVICE_NONE_LEFT      -104
#define USBEH_ERR_HUB_INIT_FAILED       -105
#define USBEH_ERR_INTERFACE_NOT_FOUND   -106

#define USBEH_TOKEN_SETUP   0
#define USBEH_TOKEN_IN      1
#define USBEH_TOKEN_OUT     2

#define USBEH_TD_ROUNDING      (USBEH_U32)0x00040000
#define USBEH_TD_SETUP         (USBEH_U32)0x00000000
#define USBEH_TD_IN            (USBEH_U32)0x00100000
#define USBEH_TD_OUT           (USBEH_U32)0x00080000
#define USBEH_TD_DELAY_INT(x)  (USBEH_U32)((x) << 21)
#define USBEH_TD_TOGGLE_0      (USBEH_U32)0x02000000
#define USBEH_TD_TOGGLE_1      (USBEH_U32)0x03000000
#define USBEH_TD_CC            (USBEH_U32)0xF0000000

typedef struct {
    USBEH_U08   bLength;
    USBEH_U08   bDescriptorType;
    USBEH_U16   bcdUSB;
    USBEH_U08   bDeviceClass;
    USBEH_U08   bDeviceSubClass;
    USBEH_U08   bDeviceProtocol;
    USBEH_U08   bMaxPacketSize;
    USBEH_U16   idVendor;
    USBEH_U16   idProduct;
    USBEH_U16   bcdDevice;    
    USBEH_U08   iManufacturer;
    USBEH_U08   iProduct;
    USBEH_U08   iSerialNumber;
    USBEH_U08   bNumConfigurations;
} USBEH_deviceDescriptor;

typedef struct {
    USBEH_U08   bLength;
    USBEH_U08   bDescriptorType;
    USBEH_U16   wTotalLength;
    USBEH_U08   bNumInterfaces;
    USBEH_U08   bConfigurationValue;
    USBEH_U08   iConfiguration;
    USBEH_U08   bmAttributes;
    USBEH_U08   bMaxPower;
} USBEH_configurationDescriptor;

typedef struct {
    USBEH_U08   bLength;
    USBEH_U08   bDescriptorType;
    USBEH_U08   bInterfaceNumber;
    USBEH_U08   bAlternateSetting;
    USBEH_U08   bNumEndpoints;
    USBEH_U08   bInterfaceClass;
    USBEH_U08   bInterfaceSubClass;
    USBEH_U08   bInterfaceProtocol;
    USBEH_U08   iInterface;
} USBEH_interfaceDescriptor;

typedef struct {
    USBEH_U08   bLength;
    USBEH_U08   bDescriptorType;
    USBEH_U08   bEndpointAddress;
    USBEH_U08   bmAttributes;
    USBEH_U16   wMaxPacketSize;
    USBEH_U08   bInterval;
} USBEH_endpointDescriptor;

typedef struct {
  USBEH_U08 bLength;
  USBEH_U08 bDescriptorType;
  USBEH_U16 bcdHID;
  USBEH_U08 bCountryCode;
  USBEH_U08 bNumDescriptors;
  USBEH_U08 bDescriptorType2;
  USBEH_U16 wDescriptorLength;
} USBEH_HIDDescriptor;

typedef struct {
    volatile USBEH_U32  control;
    volatile USBEH_U32  tailTd;
    volatile USBEH_U32  headTd;
    volatile USBEH_U32  next;
} USBEH_HCED;

typedef struct {
    volatile USBEH_U32  control;
    volatile USBEH_U32  currentBufferPointer;
    volatile USBEH_U32  next;
    volatile USBEH_U32  bufferEnd;
} USBEH_HCTD;

typedef struct {
    volatile USBEH_U32  interruptTable[32];
    volatile USBEH_U16  frameNumber;
    volatile USBEH_U16  frameNumberPad;
    volatile USBEH_U32  doneHead;
    volatile USBEH_U08  Reserved[120];
} USBEH_HCCA;

typedef struct {
    USBEH_U08   bm_request_type;
    USBEH_U08   b_request;
    USBEH_U16   w_value;
    USBEH_U16   w_index;
    USBEH_U16   w_length;
} USBEH_Setup;

typedef void (*USBEH_callback)(int device, int endpoint, int status, USBEH_U08* data, int len, void* userData);

#define USBEH_SOF_COUNTER_INC               1
#define USBEH_SOF_COUNTER_DEC               2
#define USBEH_SOF_COUNTER_DEC_HALT_AT_ZERO  4
#define USBEH_SOF_COUNTER_RELOAD            8

typedef struct _sof_counter {
    USBEH_U08       mode;
    USBEH_U08       flag;
    USBEH_U32       counter;
    USBEH_U32       reload;
    USBEH_U32       userData;
    void            (*callback)(struct _sof_counter *);
    _sof_counter    *next;
} USBEH_SOF_COUNTER;

//typedef void (*USBCallback)(int device, int endpoint, int status, USBEH_U08* data, int len, void* userData);


// Macros.

#define USBEH_CURRENT_CONNECT_STATUS    0x01
#define USBEH_CONNECT_STATUS_CHANGE     (USBEH_CURRENT_CONNECT_STATUS << 16)
#define USBEH_PORT_RESET_STATUS         0x10
#define USBEH_PORT_RESET_STATUS_CAHNGE  (USBEH_PORT_RESET_STATUS << 16)
#define USBEH_CONTROL_LIST_ENABLE       0x10
#define USBEH_CONTROL_LIST_FILLED       0x02;
#define USBEH_BULK_LIST_ENABLE          0x20
#define USBEH_BULK_LIST_FILLED          0x04
#define USBEH_PERIODIC_LIST_ENABLE      0x04
#define USBEH_PORT_RESET_STATUS         0x10
#define USBEH_PORT_RESET_STATUS_CHANGE  (USBEH_PORT_RESET_STATUS << 16)
#define USBEH_LOW_SPEED_DEVICE          0x200
#define USBEH_HIGH_SPEED_DEVICE         0x400

#endif