/*
 * lpd8806.h
 *
 *  Created on: 8.12.2012
 *      Author: Teijo Mursu
 *
 *  License: BSD (See LICENSE)
 *
 *  NOT USED - Old implementation
 */

class LPD8806_WOS {
private:
	int numLEDs;
	unsigned long leds[32];

	void write(unsigned long value);
public:
	LPD8806_WOS(int numLEDs);
	void begin();
	void show();
	void setPixelColor(const int &pos, const unsigned long &color);
	void setPixelColor(const int &pos, const int &r, const int &g, const int &b);

	unsigned long Color(int r, int g, int b);

	int numPixels();
};
