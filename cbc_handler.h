/*
 * cbc_handler.h
 *
 *  Created on: 8.12.2012
 *      Author: Teijo Mursu
 *      Based on usb_dev_serial-example
 *
 *  License: BSD (See LICENSE)
 */

#define COMMAND_PACKET_RECEIVED 0x00000001
#define COMMAND_STATUS_UPDATE   0x00000002


#ifndef CBC_HANDLER_H_
#define CBC_HANDLER_H_

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "usb_serial_structs.h"
#include "driverlib/interrupt.h"

extern void SetControlLineState(unsigned short usState);
extern tBoolean SetLineCoding(tLineCoding *psLineCoding);
extern void GetLineCoding(tLineCoding *psLineCoding);
extern void SendBreak(tBoolean bSend);
extern unsigned long ControlHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue, void *pvMsgData);
extern unsigned long TxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue, void *pvMsgData);
extern unsigned long RxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue, void *pvMsgData);

extern void USBUARTPrimeTransmit();


extern volatile unsigned long g_ulFlags;
extern char *g_pcStatus;
extern tBoolean g_bSendingBreak;
extern unsigned int bytesToRead;

//*****************************************************************************
//
// Global flag indicating that a USB configuration has been set.
//
//*****************************************************************************
extern volatile tBoolean g_bUSBConfigured;


#endif /* CBC_HANDLER_H_ */
