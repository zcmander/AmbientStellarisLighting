
Ambient lighting for PC using LPD8806 led strip and adalight-protocol
=====================================================================
Straight forward adalight implementation for Stellaris LaunchPad. Developed for LM4F120H5QR.


See full article: http://juippi.kapsi.fi/p/558

Orginal adalight for arduino: http://learn.adafruit.com/adalight-diy-ambient-tv-lighting/overview

Features
---------
 * Led smoothing (similar implementation to lightpack's arduino-based software)
 * Debug interface using Debug UART interface (requires connecting DEBUG USB to PC)
 * Supports Adalight-protocol
 * Support for LPD8806 led driver
 
Limitations
---------
 * Led count to 32 (easy to change...)
 * Fixed ledstrip pins:
	* SSI0-port for SPI data transfer
	* PA2 for Clock IN (CI)
	* PA5 for Data IN (DI)
 
Requirements
---------
 * StellarisWare: usblib, utils, uart
 * Code Composer Studio (startup_css.c only for CCS, but should be easily to port to gcc and others)
 

Note: lpd8806_wo_smooth.cpp/h are not used. Old implementation, just for reference only, because smoothing-operations shouldn't be coupled to led strip driver.

*Warning:* Code is very messy :( I might refactor whole project, if I ever need to touch this anymore