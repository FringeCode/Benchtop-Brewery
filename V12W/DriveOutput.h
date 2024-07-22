/* DriveOutput.h - Library for DriveOutput function in PID sketches
	Created by Dan Dodge, April 7, 2022
	Based on Sous-Viduino sketch
*/

#ifndef DriveOutput_h
#define DriveOutput_h

#include "Arduino.h"
#define RelayPin 49

struct DriveOutputStruct
{
public:

	unsigned long windowStartTime;
	int WindowSize = 10000;
	volatile long onTime = 0;
	void DoDriveOutput(void);

private:
	
};

# endif
