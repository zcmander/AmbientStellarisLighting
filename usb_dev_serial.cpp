//*****************************************************************************
//
// usb_dev_serial.c - Main routines for the USB CDC serial example.
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

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
#include "utils/ustdlib.h"
#include "utils/cmdline.h"

#include "cbc_handler.h"
#include "lpd8806.h"


//*****************************************************************************
//
// Configuration and tuning parameters.
//
//*****************************************************************************

//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 100
#define SYSTICK_PERIOD_MS (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Variables tracking transmit and receive counts.
//
//*****************************************************************************
volatile unsigned long g_ulUARTTxCount = 0;
volatile unsigned long g_ulUARTRxCount = 0;
#ifdef DEBUG
unsigned long g_ulUARTRxErrors = 0;
#endif

//*****************************************************************************
//
// The base address, peripheral ID and interrupt ID of the UART that is to
// be redirected.
//
//*****************************************************************************


//*****************************************************************************
//
// Default line coding settings for the redirected UART.
//
//*****************************************************************************
#define DEFAULT_BIT_RATE        115200
//#define DEFAULT_BIT_RATE        9600
#define DEFAULT_UART_CONFIG     (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | \
                                 UART_CONFIG_STOP_ONE)

//*****************************************************************************
//
// GPIO peripherals and pins muxed with the redirected UART.  These will depend
// upon the IC in use and the UART selected in USB_UART_BASE.  Be careful that
// these settings all agree with the hardware you are using.
//
//*****************************************************************************

//*****************************************************************************
//
// Defines required to redirect UART0 via USB.
//
//*****************************************************************************
#define TX_GPIO_BASE            GPIO_PORTD_BASE
#define TX_GPIO_PERIPH          SYSCTL_PERIPH_GPIOD
#define TX_GPIO_PIN             GPIO_PIN_7

#define RX_GPIO_BASE            GPIO_PORTD_BASE
#define RX_GPIO_PERIPH          SYSCTL_PERIPH_GPIOD
#define RX_GPIO_PIN             GPIO_PIN_6


//*****************************************************************************
//
// Global system tick counter
//
//*****************************************************************************
//volatile unsigned long g_ulSysTickCount = 0;

//*****************************************************************************
//
// Flags used to pass commands from interrupt context to the main loop.
//
//*****************************************************************************

// LEDS

static LPD8806 strip(32);

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    while(1)
    {
    }
}
#endif


//*****************************************************************************
//
// Read as many characters from the UART FIFO as we can and move them into
// the CDC transmit buffer.
//
// \return Returns UART error flags read during data reception.
//
//*****************************************************************************

#define ADA_ZONES 32

#define ADA_BUFFER_SIZE 3+(3*ADA_ZONES)

static const char magic_code[] = {'A', 'd', 'a'};
static unsigned char header_buffer[ADA_BUFFER_SIZE]; // Checksum + 3 channels (RGB bytes)

static unsigned int getAdaHeader = 0;
static unsigned long bytesReadAda = 0;
static const unsigned int ada_buffer_size = ADA_BUFFER_SIZE;

unsigned int bytesToRead = 0;

static char g_cInput[128];

int CMD_smooth(int argc, char **argv);
int CMD_rainbow(int argc, char **argv);
int CMD_rainbow2(int argc, char **argv);
unsigned int Wheel(long WheelPos);

extern "C" tCmdLineEntry g_sCmdTable[] =
{
		{"smooth",     CMD_smooth,      " : Set led strip smoothness" },
		{"rainbow",     CMD_rainbow,      " : Rainbow" },
		{"rainbow2",     CMD_rainbow2,      " : Rainbow2" }
};

int CMD_smooth(int argc, char **argv) {
	strip.setSmoothSlowdown(ustrtoul(argv[1], 0, 10));
	return 0;
}

int CMD_rainbow(int argc, char **argv) {
  int i, j;

  strip.setSmoothSlowdown(0);
  for (j=0; j < 384; j++) {     // 3 cycles of all 384 colors in the wheel
	for (i=0; i < strip.numPixels(); i++) {
	  strip.setPixelColor(i, Wheel( (i + j) % 384));
	}
	strip.show();   // write all the pixels out
	SysCtlDelay(ustrtoul(argv[1], 0, 10));
  }

  strip.setSmoothSlowdown(30);
  return 0;
}

int CMD_rainbow2(int argc, char **argv) {
	int i, j;

	strip.setSmoothSlowdown(0);
	for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
	  for (i=0; i < strip.numPixels(); i++) {
		// tricky math! we use each pixel as a fraction of the full 384-color wheel
		// (thats the i / strip.numPixels() part)
		// Then add in j which makes the colors go around per pixel
		// the % 384 is to make the wheel cycle around
		strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
	  }
	  strip.show();   // write all the pixels out
	  SysCtlDelay(ustrtoul(argv[1], 0, 10));
	}

	strip.setSmoothSlowdown(30);
	return 0;
}



unsigned int Wheel(long WheelPos)
{
  unsigned int r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break;
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break;
    case 2:
      b = 127 - WheelPos % 128;  //blue down
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break;
  }
  return(strip.Color(r,g,b));
}


//*****************************************************************************
//
// Take as many bytes from the transmit buffer as we have space for and move
// them into the USB UART's transmit FIFO.
//
//*****************************************************************************
char hexData[2];

void USBUARTPrimeTransmit()
{
	UARTprintf("USBUARTPrimeTransmit: USBUARTPrimeTransmit\n");

    unsigned long ulRead;
    unsigned char ucChar;

    //
    // If we are currently sending a break condition, don't receive any
    // more data. We will resume transmission once the break is turned off.
    //
    if(g_bSendingBreak)
    {
        return;
    }


    while (1) {
		ulRead = USBBufferRead((tUSBBuffer *)&g_sRxBuffer, &ucChar, 1);

		//
		// Did we get a character?
		//
		if(!ulRead)
		{
			break;
		}


		//
		// Update our count of bytes transmitted via the UART.
		//
		g_ulUARTTxCount++;
		bytesToRead = 0;

		if (getAdaHeader == 0) {
			if (ucChar == magic_code[0]) {
				getAdaHeader = 1;
				bytesToRead = 1;
			}

		} else if (getAdaHeader == 1) {
			if (ucChar == magic_code[1]) {
				getAdaHeader = 2;
				bytesToRead = 1;
			}
		} else if (getAdaHeader == 2) {
			if (ucChar == magic_code[2]) {
				getAdaHeader = 3;
				bytesReadAda = 0;
				bytesToRead = 1;
			}
		} else if (getAdaHeader == 3) {
			bytesToRead = ada_buffer_size - bytesReadAda;
			if (bytesReadAda < ada_buffer_size) {
				header_buffer[bytesReadAda] = ucChar;
				bytesReadAda += 1;
			}
			if (bytesReadAda >= ada_buffer_size) {
				getAdaHeader = 0;
				bytesReadAda = 0;
				bytesToRead = 0;


				int a;
				int i;
				for (a=0;a<ADA_ZONES;a++) {
					i = a;
					strip.setPixelColor(i, header_buffer[3+a*3]>>1, header_buffer[3+a*3+1]>>1, header_buffer[3+a*3+2]>>1);
					//strip.setPixelColor(i+1, header_buffer[3+a*3], header_buffer[3+a*3+1], header_buffer[3+a*3+2]);
				}
				strip.show();
				strip.show(); // Fixed Last-LED problem, Needs more investigation
			}
		}
    }
}

//*****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//*****************************************************************************
extern "C" void
SysTickIntHandler(void)
{
    //
    // Update our system time.
    //
    //g_ulSysTickCount++;
}

//*****************************************************************************
//
// Interrupt handler for the UART which we are redirecting via USB.
//
//*****************************************************************************
extern "C" void
USBUARTIntHandler(void)
{
    // Are we being interrupted because the TX FIFO has space available?
    //
    //if(ulInts & UART_INT_TX)
    //{
	//
	// Move as many bytes as we can into the transmit FIFO.
	//
	USBUARTPrimeTransmit();

	//
	// If the output buffer is empty, turn off the transmit interrupt.
	//
	if(USBBufferDataAvailable(&g_sRxBuffer))
	{
		USBUARTIntHandler();
	}
    //}
}

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
int main(void)
{
    unsigned long ulTxCount;
    unsigned long ulRxCount;
	unsigned long ulLoop;
	long lCommandStatus;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    FPULazyStackingEnable();

    //
    // Set the clocking to run from the PLL at 50MHz
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);


    //
    // Configure the required pins for USB operation.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_5 | GPIO_PIN_4);

	//
    // Enable the GPIO port that is used for the on-board LED.
	//
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3|GPIO_PIN_2);

    //
    // Not configured initially.
    //
    g_bUSBConfigured = false;

    //
    // Enable the system tick.
    //
    SysTickPeriodSet(SysCtlClockGet() / SYSTICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Initialize the transmit and receive buffers.
    //
    USBBufferInit((tUSBBuffer *)&g_sTxBuffer);
    USBBufferInit((tUSBBuffer *)&g_sRxBuffer);
    USBStackModeSet(0, USB_MODE_DEVICE, 0);
    USBDCDCInit(0, (tUSBDCDCDevice *)&g_sCDCDevice);

    // Initialize LED strip
    strip = LPD8806(32);
    strip.begin();

    IntMasterEnable();

    //
    // Clear our local byte counters.
    //
    ulRxCount = 0;
    ulTxCount = 0;

	// Enable UART
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTStdioInit(0);


    UARTprintf("\033[2J");
    UARTprintf("Welcome to the RGB LED Strip control panel!\n");
    UARTprintf("Type 'help' for list of commands.\n");
    //UARTFlushTx(false);

    //TimerEnable(TIMER0_BASE, TIMER_A); // Enable timer AFTER strip.begin()

    while(1)
    {
    	UARTprintf("> ");
    	while(UARTPeek('\r') == -1)
		{
			//
			// millisecond delay.  A SysCtlSleep() here would also be OK.
			//
			SysCtlDelay(SysCtlClockGet() / (1000 / 3));

	        //
	        // Have we been asked to update the status display?
	        //
	        if(g_ulFlags & COMMAND_STATUS_UPDATE)
	        {
	        	UARTprintf("main(): COMMAND_STATUS_UPDATE\n");
	            g_ulFlags &= ~COMMAND_STATUS_UPDATE;
	        }

	        if(ulTxCount != g_ulUARTTxCount)
	        {
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
			    for(ulLoop = 0; ulLoop < 150000; ulLoop++) {}
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
	            ulTxCount = g_ulUARTTxCount;
			}

	        if(ulRxCount != g_ulUARTRxCount)
	        {
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
			    for(ulLoop = 0; ulLoop < 150000; ulLoop++)
			    {
			    }
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
	            ulRxCount = g_ulUARTRxCount;
	        }

	        strip.show();
		}

		//
		// a '\r' was detected get the line of text from the user.
		//
		UARTgets(g_cInput,sizeof(g_cInput));

        lCommandStatus = CmdLineProcess(g_cInput);

		//
		// Handle the case of bad command.
		//
		if(lCommandStatus == CMDLINE_BAD_CMD)
		{
			UARTprintf("Bad command!\n");
		}

		//
		// Handle the case of too many arguments.
		//
		else if(lCommandStatus == CMDLINE_TOO_MANY_ARGS)
		{
			UARTprintf("Too many arguments for command processor!\n");
		}
    }
}
