#include <SoftwareSerial.h>
#include "Jonas.h"
#include "U8glib.h"
#include <Wire.h>

U8GLIB_SSD1306_128X64 oled(U8G_I2C_OPT_NONE);
DigitalOutput led(13, false);


const int bufferLen = 1024;
char buffer[bufferLen];
int pos = 0;
bool finding_start = true;


const int avgBufferSize = 10;
double avgBuffer[avgBufferSize];
int avgIdx = 0; // next position for entering average value
int avgLen = 0; // how many average values we have
double averageSpeed = 0;

// If new speed sample is received outside this window tolerance, the average buffer is discarded.
// It makes the solution more responsive during acceleration.
double validAverageSpeedTolerance = 1.0;

// This is just used to make the on board LED indicate when the 
// code reaches a certain line (where we set the value)
// Thus mainly used for dynamic head-less debugging:
unsigned long ledtimestamp = millis();


void setup() {
	Serial.begin(9600);
}


void loop(void) {

	// led is an instance of DigitalOutput, where I use some implicit operators for bool.
	// assigning it sets the output. See Jonas.h
	led = millis() < ledtimestamp + 100;

	if (!SerialDequeueSentence())
		return;

	double speed = ParseSpeedMessage();
	if (speed == -1)
		return;

	if (abs(speed - averageSpeed) > validAverageSpeedTolerance) {
		NewAverage(speed);
	}
	else {
		FeedAverage(speed);
	}

	averageSpeed = GetAverageSpeed();

	if (averageSpeed < validAverageSpeedTolerance) {
		// we are probably not moving:
		averageSpeed = 0.0;
	}

	ledtimestamp = millis();

	char speedBuffer[8];
	dtostrf(averageSpeed, 6, 1, speedBuffer);

	led = true;
	oled.firstPage();
	do {
		draw(speedBuffer);
	} while (oled.nextPage());
	led = false;
}

bool SerialDequeueSentence() {
	// Example: 
	// $GNVTG,,T,,M,0.312,N,0.578,K,A*37
	// Fill buffer until we have the whole sentence:
	int count = Serial.available();
	while (count-- > 0) {
		char chr = Serial.read();
		if (finding_start && chr == '$')
		{
			finding_start = false;
			pos = 0;
		}

		if (!finding_start)
		{
			if (chr != '\r' && pos < bufferLen)
			{
				buffer[pos++] = chr;
			}
			else
			{
				finding_start = true;
				if (chr != '\r')
				{
					// naah...
					break;
				}
				return true;
			}
		}
	}
	return false;
}


double ParseSpeedMessage() {
	// https://www.trimble.com/OEM_ReceiverHelp/V4.44/en/NMEA-0183messages_VTG.html

	// $GNVTG,,T,,M,0.045,N,0.083,K,A*37
	// $GNVTG,,T,,M,0.045,N,0.084,K,A*30
	// $GNVTG,,T,,M,0.068,N,0.126,K,A*36
	// $GNVTG,,T,,M,0.076,N,0.141,K,A*38
	// $GNVTG,,T,,M,0.077,N,0.143,K,A*3B
	// $GNVTG,,T,,M,0.083,N,0.153,K,A*31
	// $GNVTG,,T,,M,0.101,N,0.187,K,A*33
	// $GNVTG,,T,,M,0.110,N,0.204,K,A*3B
	// $GNVTG,,T,,M,0.116,N,0.215,K,A*3D
	// $GNVTG,,T,,M,0.142,N,0.263,K,A*3D
	// $GNVTG,,T,,M,0.239,N,0.442,K,A*37

	// $GNVTG,,T,,M,0.312,N,0.578,K,A*37
	// the speed above is:  0.578 km/h   

	// $GPVTG,,T,,M,0.00,N,0.00,K*4E
	// 
	// VTG message fields
	// Field  Meaning
	// 0      Message ID $GPVTG
	// 1      Track made good (degrees true)
	// 2      T: track made good is relative to true north
	// 3      Track made good (degrees magnetic)
	// 4      M: track made good is relative to magnetic north
	// 5      Speed, in knots
	// 6      N: speed is measured in knots
	// 7      Speed over ground in kilometers/hour (kph)
	// 8      K: speed over ground is measured in kph
	// 9      The checksum data, always begins with *

	const char* messagePrefix = "$GNVTG,";
	int field = 0;
	int fieldstart = 0;
	double result = 0;
	for (int x = 0; x < pos; x++)
	{
		//SerialDebug("x: ", x);
		//SerialDebug("buffer[x]: ", buffer[x]);

		switch (field) {

		case 0:
			if (buffer[x] != messagePrefix[x]) {
				return -1;
			}
			if (x == 6) {
				// message with velocity found.
				field = 1;
			}
			break;

		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			if (buffer[x] == ',') {
				// next field:
				field++;
			}
			break;
		
		case 7:
			// OK so this code is not the most to the point (pun intended) it could be.
			// The speed provided as a string is converted to a rounded integer value of 
			// deci-km/h, which is then divided by 10 to get the double result.
			// It should be enough to just extract the string and convert it to double
			// , but the code is based on an earlier experiment which involved two arduinos which 
			// sent the speed value using integer from one to the other.

			result = 0;
			bool foundDecimalPoint = false;
			while (x < pos)
			{
				if (buffer[x] == '.') {
					foundDecimalPoint = true;
					// Move beyond decimal point:
					x++;
					continue;
				}

				if (!isdigit(buffer[x])) {
					// This was unexpected;
					return -1;
				}
				result = result * 10 + (buffer[x] - '0');

				if (foundDecimalPoint) {
					// Now the result should be in deci-km/h, that is, km/h * 10
					// let's check for another digit, to see if we should round up:
					x++;
					if (isdigit(buffer[x]) && buffer[x] >= '5') {
						result += 1;
					}
					// And divide by 10 to get our speed in km/h:
					return (double)result / 10.0;
				}
				x++;
			}
			break;
		}
	}
	return -1;
}

double NewAverage(double speed) {
	avgIdx = 0;
	avgBuffer[avgIdx] = speed;
	avgLen = 1;
}

void FeedAverage(double speed) {
	avgIdx = (avgIdx + 1) % avgBufferSize;
	avgBuffer[avgIdx] = speed;
	avgLen = max(avgIdx + 1, avgLen);
}

double GetAverageSpeed() {
	double accumulator = 0;
	if (avgLen == 0)
		return 0;

	for (int i = 0; i < avgLen; i++)
	{
		accumulator += avgBuffer[i];
	}
	return accumulator / (double)avgLen;
}

void draw(char* buffer) {
	// This is not optimized at all. The characters displayed on screen are not centered.
	// will improve as more information is added to the display. 
	// Now it's just the speed in km/h.

	oled.setFontPosCenter();

	oled.setFont(u8g_font_helvB24r);
	oled.drawStr(0, 31, buffer);
}
