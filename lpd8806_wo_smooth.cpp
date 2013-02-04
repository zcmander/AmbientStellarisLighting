/*
 * lpd8806.cpp
 *
 *  Created on: 2.12.2012
 *      Author: Teijo Mursu
 *
 *  License: BSD (See LICENSE)
 *
 *  NOT USED - Old implementation
 */
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "lpd8806_wo_smooth.h"
#include <string.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_uart.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/usb.h"
#include "driverlib/rom.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "usb_serial_structs.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"

LPD8806_WOS::LPD8806_WOS(int numLEDs) : numLEDs(numLEDs) {
    //memset(leds, 0xFF, numLEDs*3*3);
}

void LPD8806_WOS::write(unsigned long value) {
	SSIDataPut(SSI0_BASE, (unsigned char)(value>>16));
	SSIDataPut(SSI0_BASE, (unsigned char)(value>>8));
	SSIDataPut(SSI0_BASE, (unsigned char)(value));
}

void LPD8806_WOS::begin() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);

    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 |
                       GPIO_PIN_2);
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, 2000000, 8);
    SSIEnable(SSI0_BASE);
	write(0x000000);
}

void LPD8806_WOS::show() {
	write(0x000000);
	int i=0;
	while (i < 32) {
		//GPIOPinWrite(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED, BLUE_LED);
		//SysCtlDelay(4000);
		write(leds[i] | 0x808080);
		i++;
	}
}

int LPD8806_WOS::numPixels() {
	return numLEDs;
}

void LPD8806_WOS::setPixelColor(const int &pos, const unsigned long &color) {
	leds[pos] = color;
}

void LPD8806_WOS::setPixelColor(const int &pos, const int &r, const int &g, const int &b) {
	leds[pos] = this->Color(r,g,b);
}

unsigned long LPD8806_WOS::Color(int r, int g, int b) {
	return 0x808080 | ((unsigned long)g << 16) | ((unsigned long)r << 8) | (unsigned long)b;
}
