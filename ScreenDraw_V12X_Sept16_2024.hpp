// screendraw.h
// Dodge Temperature Controller / PID Libary
// Screen Display Functions here
#pragma once

#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include "Variables.hpp"

// GetClockTime
// Return a string in the format "HH:MM:SS" for timer displays
String GetClockTime (unsigned long timer_ms) {

  // Hours
  // NO LEADING 0
  uint8_t h = timer_ms  / 3600000;
  String hour_string(h);

  // Minutes
  String minute_string = "";
  if (h > 0)
    timer_ms -= h * 3600000;
  uint8_t m = timer_ms / 60000;
  if (m < 10)
    minute_string += String(0);
  minute_string += String(m);

  // Seconds
  String second_string = "";
  if (m > 0)
    timer_ms -= m * 60000;
  uint8_t s = timer_ms / 1000;
  if (s < 10)
    second_string += String(0);
  second_string += String(s);

  String colon = ":";

  return hour_string + colon + minute_string + colon + second_string;

}

// SEPTEMBER 19 2022
// timer class encapsulating its own state?
template<typename T>
struct LoopingTimerStateList {
  // ?
};


/*********************
  UPDATE 3/28
  Took color #define statements from adafruit pdf page 6 of 26
**********************/

void DrawSettingsScreenStaticText (MCUFRIEND_kbv& tft) {

  tft.setRotation(0);
  tft.setCursor(100, 10);
  tft.setTextSize(1.75);
  tft.print("DODGE BREWING SYSTEMS        ");
  tft.print(Version);
	
  tft.drawFastHLine(0, 30, 320, TFT_BLACK);
  tft.setCursor(90, 40);
  tft.setTextSize(2);
  tft.print("PID SETTINGS");
  tft.drawFastHLine(0, 60, 320, TFT_BLACK);
  
  tft.setCursor(15, 70);
  tft.setTextSize(1);
  tft.print("STRIKE TEMP");

  tft.setCursor(90, 70);
  tft.setTextSize(1);
  tft.print("P:");

  tft.setCursor(180, 70);
  tft.setTextSize(1);
  tft.print("I:");

  tft.setCursor(250, 70);
  tft.setTextSize(1);
  tft.print("D:");

  tft.setCursor(15, 90);
  tft.setTextSize(1);
  tft.print("MASH TEMP");

  tft.setCursor(90, 90);
  tft.setTextSize(1);
  tft.print("P:");

  tft.setCursor(180, 90);
  tft.setTextSize(1);
  tft.print("I:");

  tft.setCursor(250, 90);
  tft.setTextSize(1);
  tft.print("D:");

  tft.drawFastHLine(0, 105, 320, TFT_BLACK);
  tft.setCursor(80, 115);
  tft.setTextSize(2);
  tft.print("MASH TUN SETTINGS");
  tft.drawFastHLine(0, 135, 320, TFT_BLACK);

  tft.setCursor(15, 142);
  tft.setTextSize(1);
  tft.print("MASH TEMP SETPOINT:");

  tft.setCursor(15, 162);
  tft.setTextSize(1);
  tft.print("WATER TO GRAIN RATIO:");

  tft.setCursor(15, 182);
  tft.setTextSize(1);
  tft.print("INITIAL GRAIN TEMP:");

  tft.setCursor(15, 202);
  tft.setTextSize(1);
  tft.print("INITIAL WATER TEMP:");

  tft.setCursor(15, 222);
  tft.print("MASH TUN RAMP OFF TEMP:");

  tft.setCursor(15, 242);
  tft.print("MASH TUN PID START TEMP:");

  tft.drawFastHLine(0, 283, 320, TFT_BLACK);
  tft.setCursor(80, 293);
  tft.setTextSize(2);
  tft.print("TIMER SETTINGS");
  tft.drawFastHLine(0, 313, 320, TFT_BLACK);

  tft.setCursor(15, 327);
  tft.setTextSize(1);
  tft.print("MASH TIME:");

  tft.setCursor(170, 327);
  tft.setTextSize(1);
  tft.print("BOIL TIME:");

  tft.setCursor(170, 347);
  tft.print("1st Add:");
  
  tft.setCursor(170, 367);
  tft.print("2nd Add:");
      
  tft.setCursor(170, 387);
  tft.print("3rd Add:");
  
  tft.drawFastHLine(0, 440, 320, TFT_BLACK);

}

void DrawHomeScreenStaticText (MCUFRIEND_kbv& tft) {

  tft.setRotation(0);
  tft.setCursor(100, 10);
  tft.setTextSize(1.75);
  tft.print("DODGE BREWING SYSTEMS           ");
  tft.print(Version);

  tft.drawFastHLine(0, 30, 320, TFT_BLACK);
  tft.setCursor(130, 40);
  tft.setTextSize(2);
  tft.print("HOME");
  tft.drawFastHLine(0, 60, 320, TFT_BLACK);

  tft.setCursor(10, 70);
  tft.setTextSize(2);
  tft.print("Mash Tun State:");

  tft.setCursor(10, 103);
  tft.setTextSize(2);
  tft.print("Mash Tun Step:");

  tft.setCursor(10, 133);
  tft.setTextSize(2);
  tft.print("MT Set Point:");

  tft.setCursor(10, 163);
  tft.setTextSize(2);
  tft.print("MT Actual Temp:");

  tft.setCursor(10, 193);
  tft.setTextSize(2);
  tft.print("MT Strike Temp:");

  tft.setCursor(10, 223);
  tft.setTextSize(2);
  tft.print("Mash Timer:");

  tft.setCursor(10, 253);
  tft.setTextSize(2);
  tft.print("STATUS:");
  tft.drawFastHLine(0, 245, 320, TFT_BLACK);
  tft.drawFastHLine(0, 275, 320, TFT_BLACK);

  tft.setCursor(10, 313);
  tft.setTextSize(2);
  tft.print("Boil Timer:");

  tft.setCursor(10, 343);
  tft.setTextSize(2);
  tft.print("1st Addition:");

  tft.setCursor(10, 373);
  tft.setTextSize(2);
  tft.print("2nd Addition:");

  tft.setCursor(10, 403);
  tft.setTextSize(2);
  tft.print("3rd Addition:");

}

// Draw the Home Screen (screen == 0)
void DrawHomeScreen ( MCUFRIEND_kbv& tft , double* SettingVariable , StateSystem& sys , double& Input , double& Output, long unsigned int& MTActTime ,
                      long unsigned int& BoilActTime , long unsigned int& FirstAddActTime , long unsigned int& SecondAddActTime , long unsigned int& ThirdAddActTime ) {
  
  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  if (DrawHomeScreenStaticText_flag) {
    DrawHomeScreenStaticText(tft);
    DrawHomeScreenStaticText_flag = false;
  }

  tft.setCursor(200, 133);
  tft.print(SettingVariable[MTSpIndex], 1);

  tft.setCursor(200, 163);
  tft.print(Input, 1);

  tft.setCursor(200, 193);
  tft.print(MTStrikeTemp, 1);

  tft.setCursor(150, 223);
  MTActTime = sys.MashTimerObj.get_time_count_down();
  tft.print(GetClockTime(MTActTime));

  if ( sys.getState() == stateList::MASHPID ) {
    tft.setCursor(100, 253);
    tft.print("MASH PID STEP  ");
  } else if ( sys.getState() == stateList::STRIKEPID ) {
    tft.setCursor(100, 253);
    tft.print("STRIKE PID STEP");
  } else if ( sys.getState() == stateList::RAMPUP ) {
    tft.setCursor(100, 253);
    tft.print("RAMP UP STEP   ");
  } else if ( sys.getState() == stateList::RAMPOFF ) {
    tft.setCursor(100, 253);
    tft.print("RAMP OFF STEP");
  } else {
    tft.setCursor(100, 253);
    tft.print("OFF            ");
  }

  tft.setCursor(142, 313);
  BoilActTime = sys.BoilTimerObj.get_time_count_down();
  tft.print(GetClockTime(BoilActTime));

  FirstAddActTime = sys.AddTimerObj1.get_time_count_down();
  tft.setCursor(175, 343);
  tft.print(GetClockTime(FirstAddActTime));
  
  SecondAddActTime = sys.AddTimerObj2.get_time_count_down();
  tft.setCursor(175, 373);
  tft.print(GetClockTime(SecondAddActTime));

  ThirdAddActTime = sys.AddTimerObj3.get_time_count_down();
  tft.setCursor(175, 403);
  tft.print(GetClockTime(ThirdAddActTime));

}

// Draw the Settings Screen (screen == 1)
void DrawSettingsScreen ( MCUFRIEND_kbv& tft , double* SettingVariable , StateSystem& sys , double& Input , double& Output ) {

  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  if (DrawSettingsScreenStaticText_flag) {
    DrawSettingsScreenStaticText(tft);
    DrawSettingsScreenStaticText_flag = false;
  }

  tft.setTextSize(1);
  
  tft.setCursor(105, 70);
  tft.print(SettingVariable[MTSTKpIndex], 1);

  tft.setCursor(195, 70);
  tft.print(SettingVariable[MTSTKiIndex], 1);

  tft.setCursor(265, 70);
  tft.print(SettingVariable[MTSTKdIndex], 1);

  tft.setCursor(105, 90);
  tft.print(SettingVariable[MTKpIndex], 1);

  tft.setCursor(195, 90);
  tft.print(SettingVariable[MTKiIndex], 1);

  tft.setCursor(265, 90);
  tft.print(SettingVariable[MTKdIndex], 1);

  tft.setCursor(170, 142);
  tft.print(SettingVariable[MTSpIndex], 1);

  tft.setCursor(170, 162);
  tft.print(SettingVariable[MTWaterToGrainRatioIndex], 1);

  tft.setCursor(170, 182);
  tft.print(SettingVariable[MTGrainTempIndex], 1);

  tft.setCursor(170, 202);
  tft.print(SettingVariable[MTInitialWaterTempIndex], 1);

  tft.setCursor(170, 222);
  tft.print(SettingVariable[MTRampOffTempIndex], 1);

  tft.setCursor(170, 242);
  tft.print(SettingVariable[MTPIDStartTempIndex], 1);

  tft.setCursor(75, 327);
  tft.print(GetClockTime(SettingVariable[MashTimeIndex]));

  tft.setCursor(230, 327);
  tft.print(GetClockTime(SettingVariable[BoilTimeIndex]));

  tft.setCursor(230, 347);
  tft.print(GetClockTime(SettingVariable[FirstAddTimeIndex]));

  tft.setCursor(230, 367);
  tft.print(GetClockTime(SettingVariable[SecondAddTimeIndex]));

  tft.setCursor(230, 387);
  tft.print(GetClockTime(SettingVariable[ThirdAddTimeIndex]));

}

