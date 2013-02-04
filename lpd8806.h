/*
 * lpd8806.h
 *
 *  Created on: 8.12.2012
 *      Author: Teijo Mursu
 *
 *  License: BSD (See LICENSE)
 */

class LPD8806 {
private:
	int numLEDs;

	unsigned long leds_start[32];
	unsigned long leds_end[32];
	unsigned long leds_current[32];
	unsigned int leds_smoothIndex[32];
	unsigned int smoothSlowdown;

	void write(unsigned long value);
	void updateSmoothLeds();

public:
	LPD8806(int numLEDs);
	void begin();
	void show();
	void setPixelColor(const int &pos, const unsigned long &color);
	void setPixelColor(const int &pos, const int &r, const int &g, const int &b);

	unsigned long Color(int r, int g, int b);

	void setSmoothSlowdown(const unsigned int &value);

	int numPixels();
};
