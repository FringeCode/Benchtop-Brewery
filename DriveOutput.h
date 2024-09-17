/* DriveOutput.h - Library for DriveOutput function in PID sketches
	Created by Dan Dodge, April 7, 2022
	Based on Sous-Viduino sketch
*/

#pragma once

#include "Arduino.h"
#define RelayPin 49

struct DriveOutputStruct {

	unsigned long windowStartTime;
	int WindowSize = 10000;
	volatile long onTime = 0;

	void DoDriveOutput(void);

};
