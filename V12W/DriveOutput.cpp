/* 	DriveOutput.cpp - Library for DriveOutput function in PID sketches
	Created by Dan Dodge, April 7, 2022
	Based on Sous-Viduino sketch
*/

#include "Arduino.h"
#include "DriveOutput.h"


// ************************************************
// Called by ISR every 15ms to drive the output
// ************************************************

void DriveOutputStruct::DoDriveOutput( void ) {
  long now = millis();
  // Set the output.  "on time" is proportional to the PID output

  if (now - windowStartTime > WindowSize) { //time to shift the Relay Window
      windowStartTime += WindowSize;
  }

  if ((onTime > 100) && (onTime > (now - windowStartTime))) { // 3/6/22 WHAT DOES THIS DO
      digitalWrite(RelayPin, HIGH);
  } else {
      digitalWrite(RelayPin, LOW);
  }
}
