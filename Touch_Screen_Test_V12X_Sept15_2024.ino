/**************************************************
************** DODGE BREWING SYSTEMS **************
**************************************************/
// A simple and unified system with which to
//  add control and automation to your Homebrewing!

//Version history: V1-V3 created functions to draw each screen
//V8 added PID increment and decrement buttons
//V9 Combine PID and Recipe screen inputs, use Increment & decrement to enter Settings, added Mash Tun State buttons to Home Screen
//V10 adds temp sensor and PID libraries, strike temp calc
//V11 adds temp sensor input, relay output
//V11a adds state machine; fixes EEPROM paramter save and default values
//V11b adds PID control, transition between states
//V11c changes enum stateList to enum class stateList so use scope stateList::
// new syntax is stateList::RAMPOFF etc
//Also changed SettingVariable index const int's to say Index at end of variable name
// to hopefully clear up confusion and add compile errors if you were to just write
// MTKp instead of SettingVariable[MTKpIndex]
//   ( previous syntax was SettingVariable[MTKp] )
//Added reference variables so if you do write MTKp, it is equivalent to SettingVariable[MTKpIndex]
//Changed line ~574 myPID.SetTunings to use SettingVariable instead of MTSTKp i and d, not sure what effect previous code had but likely erroneous
//V11c4: Moved stateList enum outside of StateSystem class, still scoped so use stateList::RAMPUP etc.
//V11d: Added MASH PID Timer for beeping after in state for 1 minute
//V12: March 6, 2022
//   : Need to ensure that, when text output says the heater should be on, the heater is on
//   : What version of PID library is being used?
// MARCH 22 2022
// V12L1 corrects issue with Dallas Temp isCorrectionComplete issue, uses DriveOutput and ScreenDraw libraries
// V12Z adds Sounds, Lights & Counters libraries. Adds Mash & Boil Kettle times.
// V12X aligns all of the timer buttons, adds time increment buttons and spaces for time index variables (but no numbers yet)
// V12W adds timer defaults to EEPROM, adds actual values for timers, also fixes bug where buttons don't work
// V12V converts timer values to h:mm:ss
// V12T MOST RECENT AS OF JUNE182022
// JUNE 19 2022
// got timer to work in separate non-tempcontroller sketch, attempting to translate to this
// Mash Timer starts/resumes when MASH white button is pressed, pauses when PAUSE red button above MASH button pressed
// in code for MashPID state [around line 670 as of june 24 2022], I have the system set the timer to 10 seconds since I couldn't figure out how to do that 
// with just the buttons and EEPROM data retrieval, even though on settings screen I changed timer to 10 seconds it would just
// stay at 10 minutes without that line of code
// Added code for appropriate pausing effects in the if(Pause button pressed) section
// JUNE 24 2022
// Adding boil/add timer counting logic in loop()
// V12U Adding additional LED's and Buzzer for timers
// variables for replacing delay with millis()
// 2/22/23 V12V Add Variables for multi-color LEDs. Comment out until new hardware profile is available
// 2/23/23 V12W Move Mash Tun Status to middle of screen. Make Status display on one line. Move Boil timers to bottom. 

// SEPT 15 2024
// V12X reorganizes definitions and declarations of variables and functions

#include "DriveOutput.h"
#include "ScreenDraw_V12X_Sept16_2024.hpp"
#include "Variables.hpp"
#include "ButtonFunctions.hpp"
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <PID_v1.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

void setup() {

  DrawHomeScreenStaticText_flag = true;
  DrawSettingsScreenStaticText_flag = true;

  // opens serial port, sets data rate to 9600 bps
  Serial.begin(9600);
  Serial.println("CLEARDATA");
  Serial.println("LABEL, Time, Step, Temp (F), Power (%), Time (s), Sp, P, I, D,Millis(),MTStartTime, MTActTime, SettingVariable[MashTimeIndex],TimeIncludingPreviousPauses,TimeSinceUnPause,MashTime,ActualTimerOut,UnPaused,UnPauseTime");

  pinMode(RelayPin, OUTPUT); // Output mode to drive relay
  digitalWrite(RelayPin, LOW); // make sure it is off to start

  pinMode(ONE_WIRE_PWR, OUTPUT);
  digitalWrite(ONE_WIRE_PWR, HIGH); // 3/6/22 What's the difference between RelayPin and ONE_WIRE_PWR ?

  // Add BuzzerPin definition 1/16/23
  pinMode(BuzzerPin,OUTPUT);
  pinMode(MTLEDPin,OUTPUT);
  pinMode(BKLEDPin,OUTPUT);
  pinMode(BKFirstAddLEDPin,OUTPUT);
  pinMode(BKSecondAddLEDPin,OUTPUT);
  pinMode(BKThirdAddLEDPin,OUTPUT);

  sensors.begin();
  if (!sensors.getAddress(tempSensor, 0)) {
    // NO LCD OBJECT, THESE LINES COMMENTED OUT FOR NOW
    //lcd.setCursor(0, 1);
    //lcd.print("Sensor");
  }
  sensors.setResolution(tempSensor, 12);
  sensors.setWaitForConversion(false);

  tft.begin(tft.readID());
  tft.setRotation(0);     //0-Portrait, 1=Landscape

  // INITIALIZE BUTTONS
  Home_btn.initButton(&tft, 40, 460, 60, 25, TFT_BLACK, TFT_WHITE, TFT_BLACK, "HOME", 2);
  Pid_btn.initButton(&tft, 256, 460, 110, 25, TFT_BLACK, TFT_WHITE, TFT_BLACK, "SETTINGS", 2);
  MTRun_btn.initButton(&tft, 210, 80, 40, 25, TFT_BLACK, TFT_GREEN, TFT_BLACK, "RUN", 2);
  MTPause_btn.initButton(&tft, 280, 80, 70, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "PAUSE", 2);
  MTStrike_btn.initButton(&tft, 217, 110, 80, 25, TFT_BLACK, TFT_WHITE, TFT_BLACK, "STRIKE", 2);
  MTMash_btn.initButton(&tft, 290, 110, 60, 25, TFT_BLACK, TFT_WHITE, TFT_BLACK, "MASH", 2);
  MTTimerCancel_btn.initButton(&tft, 290, 230, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);

  BKTimerStart_btn.initButton(&tft, 245, 320, 35, 25, TFT_BLACK, TFT_GREEN, TFT_BLACK, "ON", 2);
  BKTimerCancel_btn.initButton(&tft, 290, 320, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
  BKFirstAddCancel_btn.initButton(&tft, 290, 350, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
  BKSecondAddCancel_btn.initButton(&tft, 290, 380, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
  BKThirdAddCancel_btn.initButton(&tft, 290, 410, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);

  SelMTSTKp_btn.initButton(&tft, 155, 73, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTSTKi_btn.initButton(&tft, 230, 73, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTSTKd_btn.initButton(&tft, 300, 73, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTKp_btn.initButton(&tft, 155, 93, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTKi_btn.initButton(&tft, 230, 93, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTKd_btn.initButton(&tft, 300, 93, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTSp_btn.initButton(&tft, 220, 145, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTWaterToGrainRatio_btn.initButton(&tft, 220, 165, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTGrainTemp_btn.initButton(&tft, 220, 185, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTInitialWaterTemp_btn.initButton(&tft, 220, 205, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTRampOffTemp_btn.initButton(&tft, 220, 225, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelMTPIDStartTemp_btn.initButton(&tft, 220, 245, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);

  Dec10_btn.initButton(&tft, 20, 270, 30, 20, TFT_BLACK, TFT_RED, TFT_BLACK, "-10", 1);
  Dec1_btn.initButton(&tft, 60, 270, 30, 20, TFT_BLACK, TFT_MAGENTA, TFT_BLACK, "-1", 1);
  Dectenth_btn.initButton(&tft, 100, 270, 30, 20, TFT_BLACK, TFT_YELLOW, TFT_BLACK, "-0.1", 1);
  Inctenth_btn.initButton(&tft, 220, 270, 30, 20, TFT_BLACK, TFT_YELLOW, TFT_BLACK, "+0.1", 1);
  Inc1_btn.initButton(&tft, 260, 270, 30, 20, TFT_BLACK, TFT_MAGENTA, TFT_BLACK, "+1", 1);
  Inc10_btn.initButton(&tft, 300, 270, 30, 20, TFT_BLACK, TFT_RED, TFT_BLACK, "+10", 1);

  SelMTTimer_btn.initButton(&tft, 145, 330, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelBKTimer_btn.initButton(&tft, 300, 330, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelBKFirstAdd_btn.initButton(&tft, 300, 350, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelBKSecondAdd_btn.initButton(&tft, 300, 370, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  SelBKThirdAdd_btn.initButton(&tft, 300, 390, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  
  Dec10Min_btn.initButton(&tft, 20, 415, 30, 20, TFT_BLACK, TFT_RED, TFT_BLACK, "-10m", 1);
  Dec1Min_btn.initButton(&tft, 60, 415, 30, 20, TFT_BLACK, TFT_MAGENTA, TFT_BLACK, "-1m", 1);
  Dec10Sec_btn.initButton(&tft, 100, 415, 30, 20, TFT_BLACK, TFT_YELLOW, TFT_BLACK, "-10s", 1);
  Dec1Sec_btn.initButton(&tft, 140, 415, 30, 20, TFT_BLACK, TFT_WHITE, TFT_BLACK, "-1s", 1);
  Inc1Sec_btn.initButton(&tft, 180, 415, 30, 20, TFT_BLACK, TFT_WHITE, TFT_BLACK, "+1s", 1);
  Inc10Sec_btn.initButton(&tft, 220, 415, 30, 20, TFT_BLACK, TFT_YELLOW, TFT_BLACK, "+10s", 1);
  Inc1Min_btn.initButton(&tft, 260, 415, 30, 20, TFT_BLACK, TFT_MAGENTA, TFT_BLACK, "+1m", 1);
  Inc10Min_btn.initButton(&tft, 300, 415, 30, 20, TFT_BLACK, TFT_RED, TFT_BLACK, "+10m", 1);

  draw_button_list(screen0, screen0_length);
  
  // Initialize the PID and related variables
  LoadParameters();
  myPID.SetTunings(SettingVariable[MTSTKpIndex], SettingVariable[MTSTKiIndex], SettingVariable[MTSTKdIndex]);

  myPID.SetSampleTime(1000);
  myPID.SetOutputLimits(0, DriveOutput.WindowSize);

  sys.MashTimerObj.tLengthS = SettingVariable[MashTimeIndex];
  sys.BoilTimerObj.tLengthS = SettingVariable[BoilTimeIndex];
  sys.AddTimerObj1.tLengthS = SettingVariable[FirstAddTimeIndex];
  sys.AddTimerObj2.tLengthS = SettingVariable[SecondAddTimeIndex];
  sys.AddTimerObj3.tLengthS = SettingVariable[ThirdAddTimeIndex];

}

void loop() {

  MTStrikeTemp = ((0.2 / SettingVariable[MTWaterToGrainRatioIndex]) * (SettingVariable[MTSpIndex] - SettingVariable[MTGrainTempIndex])) + SettingVariable[MTSpIndex];

  sys.checkState();
  sys.doState();

  static uint16_t color = TFT_WHITE;

  sensors.requestTemperatures();
  Input = sensors.getTempF(tempSensor);//switch from Celcius to Fahrenheit// READ TOUCH

  if (screen == 0) {
    bool BuzzerPinOn = false;

    bool MTLEDPinOn = false;
    if ( sys.MashTimerObj.get_time_count_down() == 0) {
      BuzzerPinOn = true;
      MTLEDPinOn  = true;
    }
    
    bool BKLEDPinOn  = false;
    if ( sys.BoilTimerObj.get_time_count_down() == 0 ) {
      BuzzerPinOn = true;
      BKLEDPinOn  = true;
    }

    bool BKLEDPinOnFirstAdd = false;
    if ( sys.AddTimerObj1.get_time_count_down() == 0 && SettingVariable[FirstAddTimeIndex] != 0 ) {
      BuzzerPinOn = true;
      BKLEDPinOnFirstAdd  = true;
    }

    bool BKLEDPinOnSecondAdd = false;
    if ( sys.AddTimerObj2.get_time_count_down() == 0 && SettingVariable[SecondAddTimeIndex] != 0 ) {
      BuzzerPinOn = true;
      BKLEDPinOnSecondAdd  = true;
    }

    bool BKLEDPinOnThirdAdd = false;
    if ( sys.AddTimerObj3.get_time_count_down() == 0 && SettingVariable[ThirdAddTimeIndex] != 0 ) {
      BuzzerPinOn = true;
      BKLEDPinOnThirdAdd  = true;
    }

    if ( MTLEDPinOn ) {
      digitalWrite(MTLEDPin,HIGH);
      BuzzTone = MTTone;
    } else {
      digitalWrite(MTLEDPin,LOW);
    }

    if ( BKLEDPinOn )  {
      digitalWrite(BKLEDPin,HIGH);
      BuzzTone = BKTone;
    } else {
      digitalWrite(BKLEDPin,LOW);
    }

    if ( BKLEDPinOnFirstAdd )  {
      digitalWrite(BKFirstAddLEDPin,HIGH);
      BuzzTone = FirstAddTone;
    } else {
      digitalWrite(BKFirstAddLEDPin,LOW);
    }

    if ( BKLEDPinOnSecondAdd )  {
      digitalWrite(BKSecondAddLEDPin,HIGH);
      BuzzTone = SecondAddTone;
    } else {
      digitalWrite(BKSecondAddLEDPin,LOW);
    }

    if ( BKLEDPinOnThirdAdd )  {
      digitalWrite(BKThirdAddLEDPin,HIGH);
      BuzzTone = ThirdAddTone;
    } else {
      digitalWrite(BKThirdAddLEDPin,LOW);
    }

    if ( BuzzerPinOn ) {
      tone( BuzzerPin , BuzzTone );
    } else {
      noTone( BuzzerPin );
    }

    DrawSettingsScreenStaticText_flag = true;
    LoadParameters();
    DrawHomeScreen( tft , SettingVariable , sys , Input , Output , MTActTime , BoilActTime , FirstAddActTime , SecondAddActTime , ThirdAddActTime );
    update_button_list(screen0, screen0_length);  //process all buttons
    CheckBottomButtonPress();
  }
  
  if ( screen == 1 ) {
    DrawHomeScreenStaticText_flag = true;
    DrawSettingsScreen( tft , SettingVariable , sys , Input , Output );
    update_button_list(screen1, screen1_length);  //process all buttons
    CheckBottomButtonPress();
    CheckSettingButtonPress();
    CheckIncDecButtonPress();
    CheckTimeIncDecButtonPress();
  }

}