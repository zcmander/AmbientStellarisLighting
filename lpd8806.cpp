/*
 * lpd8806.cpp
 *
 *  Created on: 8.12.2012
 *      Author: Teijo Mursu
 *
 *  License: BSD (See LICENSE)
 */

#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "lpd8806.h"
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
#include "utils/ustdlib.h"
#include "usb_serial_structs.h"
#include "utils/uartstdio.h"

static void Unpack(const unsigned long &color, unsigned char &r, unsigned char &g, unsigned char &b);

LPD8806::LPD8806(int numLEDs) : numLEDs(numLEDs) {

	int i;
	for (i=0; i < this->numLEDs; i++) {
		leds_smoothIndex[i] = 0;
		leds_current[i] = 0;
		leds_end[i] = 0;
		leds_start[i] = 0;
	}
	this->smoothSlowdown = 30;
}

void LPD8806::write(unsigned long value) {
	SSIDataPut(SSI0_BASE, (unsigned char)(value>>8));
	SSIDataPut(SSI0_BASE, (unsigned char)(value>>16));
	SSIDataPut(SSI0_BASE, (unsigned char)(value));
}

void LPD8806::begin() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);

    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 |
                       GPIO_PIN_2);
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, 1000000, 8);
    SSIEnable(SSI0_BASE);
	write(0x000000);
}

void LPD8806::updateSmoothLeds() {
	int i;
	unsigned char current_r = 0;
	unsigned char current_g = 0;
	unsigned char current_b = 0;
	unsigned char end_r = 0;
	unsigned char end_g = 0;
	unsigned char end_b = 0;
	unsigned char start_r = 0;
	unsigned char start_g = 0;
	unsigned char start_b = 0;
	unsigned long coefEnd;
	unsigned long coefStart;

	for (i=0; i < this->numLEDs; i++) {
		if (leds_smoothIndex[i] >= this->smoothSlowdown) {
			leds_current[i] = leds_start[i] = leds_end[i];
		} else {
			coefEnd = ((unsigned long)(leds_smoothIndex[i] << 8)) / smoothSlowdown;
			coefStart = (1UL << 8) - coefEnd;

			Unpack(leds_start[i], start_r, start_g, start_b);
			Unpack(leds_current[i], current_r,current_g, current_b);
			Unpack(leds_end[i], end_r, end_g, end_b);

			current_r = (coefStart * start_r + coefEnd*end_r) >> 8;
			current_g = (coefStart * start_g + coefEnd*end_g) >> 8;
			current_b = (coefStart * start_b + coefEnd*end_b) >> 8;

			leds_current[i] = Color(current_r, current_g, current_b);

			leds_smoothIndex[i]++;
		}
	}
}

void LPD8806::show() {
	updateSmoothLeds();
	write(0x000000);
	int i=0;
	while (i < this->numLEDs) {
		//GPIOPinWrite(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED, BLUE_LED);
		//SysCtlDelay(4000);
		write(leds_current[i] | 0x808080);
		i++;
	}
}

void LPD8806::setSmoothSlowdown(const unsigned int &value) {
	smoothSlowdown = value;
}

int LPD8806::numPixels() {
	return numLEDs;
}

void LPD8806::setPixelColor(const int &pos, const unsigned long &color) {
	leds_start[pos] = leds_current[pos];
	leds_end[pos] = color;
	if (leds_end[pos] != leds_start[pos]) {
		leds_smoothIndex[pos] = 0;
	}
}

void LPD8806::setPixelColor(const int &pos, const int &r, const int &g, const int &b) {
	this->setPixelColor(pos, this->Color(r,g,b));
}

unsigned long LPD8806::Color(int r, int g, int b) {
	return ((unsigned long)r << 16) | ((unsigned long)g << 8) | (unsigned long)b;
}

static void Unpack(const unsigned long &color, unsigned char &r, unsigned char &g, unsigned char &b) {
	r = (unsigned char)(color>>16);
	g = (unsigned char)(color>>8);
	b = (unsigned char)(color);
}
