char* Version = "12W";

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
///////
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
int period = 1000;
// unsigned long time_now=0;

// MASH TIMER VARIABLES
long unsigned int UnPauseTime = 0; // set to millis() when UnPause-ing -> when entering MashPID step
long unsigned int TimeSinceUnPause = 0; // counts up in ms from UnPauseTime
long unsigned int TimeIncludingPreviousPauses = 0; // Need to capture time counted in previous pause-unpause sequences
// ActualTimer counts down (SAME AS MTACTTIME), ManuallySetTime doesn't count down (SAME AS SETTINGVAR[MASHTIMEINDEX])
bool UnPaused = false; // true when begin MashPID or pressing RUN *AFTER* pausing
bool ActualTimerOut = false; // true when end of timer counting down is reached

// JUNE 22 2022
// similar variables as above for addition timers
bool BoilTimerStarted = false; // need this in addition to ActualTimerOutBoil
long unsigned int BoilActTime = 0;
long unsigned int FirstAddActTime = 0;
long unsigned int SecondAddActTime = 0;
long unsigned int ThirdAddActTime = 0;
long unsigned int UnPauseTimeBoil = 0;
long unsigned int UnPauseTime1add = 0; // Separate variables in case separate timing needed
long unsigned int UnPauseTime2add = 0;
long unsigned int UnPauseTime3add = 0;
long unsigned int TimeSinceUnPauseBoil = 0;
long unsigned int TimeSinceUnPause1add = 0;
long unsigned int TimeSinceUnPause2add = 0;
long unsigned int TimeSinceUnPause3add = 0;
long unsigned int TimeIncludingPreviousPausesBoil = 0;
long unsigned int TimeIncludingPreviousPauses1add = 0;
long unsigned int TimeIncludingPreviousPauses2add = 0;
long unsigned int TimeIncludingPreviousPauses3add = 0;
bool UnPausedBoil = false;
bool UnPaused1add = false;
bool UnPaused2add = false;
bool UnPaused3add = false;
bool ActualTimerOutBoil = false;
bool ActualTimerOut1add = false;
bool ActualTimerOut2add = false;
bool ActualTimerOut3add = false;

long unsigned int MTTone = 250;
long unsigned int BKTone = 250;
long unsigned int FirstAddTone = 250;
long unsigned int SecondAddTone = 250;
long unsigned int ThirdAddTone = 280;
long unsigned int BuzzTone = 250;

// JUNE 24 2022
// Am a little confused about the exact functionality of boil and additional timers you would like
// For now I'm gonna assume you want some kind of domino effect, where one timer starts after the previous one runs out
// If you have the first additional timer start with the same amount remaining on the boil timer,
// such that they run out at the same time, I don't see why an additional timer would be necessary
// thus, dominoes
// It would help to have separate buttons for each timer for pausing and starting them, as well, but if that's not something that
// you predict will be necessary, that makes things easier
// I don't really know how to add those buttons myself, too, so for now I'm going to try to make the timers start at a set time
// and at least display in the HH:MM:SS format like the MASH timer

// #include "Sounds.h"
// #include <Lights.h>
// #include <Counters.h>
#include "DriveOutput.h"
#include "ScreenDraw_V12W_Feb232023.h"
// #include <DTime.h>
#include <Adafruit_GFX.h>

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

#include <TouchScreen.h>

#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

// Libraries for the DS18B20 Temperature Sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// So we can save and retrieve settings
#include <EEPROM.h>

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// ************************************************
// Pin definitions
// ************************************************

// Output Relay (PowerSwitch Tail)
#define RelayPin 49

// One-Wire Temperature Sensor
// (Use GPIO (General Purpose In Out) pins for power (and ground) to simplify the wiring)
#define ONE_WIRE_BUS 37
#define ONE_WIRE_PWR 39

// Pin Definitions for Buzzer & Timer LED's 1/16/23
#define BuzzerPin 43
#define MTLEDPin 33
#define BKLEDPin 29
#define BKFirstAddLEDPin 27
#define BKSecondAddLEDPin 25
#define BKThirdAddLEDPin 23

/*
#define MTLEDRedPin 2
#define MTLEDGreenPin 3
#define MTLEDBluePin 4
#define BKLEDRedPin 5
#define BKLEDGreenPin 6
#define BKLEDBluePin 7
#define BKFirstAddLEDRedPin 8
#define BKFirstAddLEDGreenPin 9
#define BKFirstAddLEDBluePin 10
#define BKSecondAddLEDRedPin 11
#define BKSecondAddLEDGreenPin 12
#define BKSecondAddLEDBluePin 13
#define BKThirdAddLEDRedPin 44
#define BKThirdAddLEDGreenPin 45
#define BKThirdAddLEDBluePin 46

 */

// copy-paste results from TouchScreen_Calibr_native.ino
//const int XP=7,XM=A1,YP=A2,YM=6; //320x480 ID=0x6814
//const int TS_LEFT=134,TS_RT=906,TS_TOP=114,TS_BOT=924;
const int XP = 8, XM = A2, YP = A3, YM = 9; //320x480 ID=0x9486
const int TS_LEFT = 116, TS_RT = 905, TS_TOP = 936, TS_BOT = 93;


// 4/23/22 Got a weird "XP not defined in this scope" error when ScreenDraw was included
// but not when unincluded...
// I don't know what these values do
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);


// MARCH 22 2022
// in screendraw.h these variables exist for the functions to work in the header
// but they are preceded by 'extern'
// I think the following declarations still need to be here

//Define Variables 4/30/22 changed from 13 to 16 Setting Variables. 5/11/22 Changed this to 18, now the buttons work
double SettingVariable[17];
int SettingIndex;
// VARIABLES ASSOCIATED WITH EACH INDEX
// 0:  MTSTKpIndex
// 1:  MTSTKiIndex
// 2:  MSTSKdIndex
// 3:  MTKpIndex
// 4:  MTKiIndex
// 5:  MTKdIndex
// 6:  MTSpIndex
// 7:  MTWaterToGrainRatioIndex
// 8:  MTGrainTempIndex
// 9:  MTInitialWaterTempIndex
// 10: MTRampOffTempIndex
// 11: MTPIDStartTempIndex
// 12: MashTimeIndex 4/30/22 added timer variables
// 13: BoilTimeIndex
// 14: FirstAddTimeIndex
// 15: SecondAddTimeIndex
// 16: ThirdAddTimeIndex

//MTST PID Value INDICES
// V12i 4/5/22  screendraw.h contains #define statements

/***********************
  V11c
  REFERENCE VARIABLES
  SHOULD MAKE variablename equivalent to SettingVariable[variablenameIndex]

  should these be const double& ?
  const int& would mean you cannot use code to directly modify the reference variable
  going to keep as double& in case you want to use these references instead of SettingVariable array
************************/
double& MTSTKp =                SettingVariable[MTSTKpIndex];
double& MTSTKi =                SettingVariable[MTSTKiIndex];
double& MTSTKd =                SettingVariable[MTSTKdIndex];
double& MTKi =                  SettingVariable[MTKpIndex];
double& MTKp =                  SettingVariable[MTKiIndex];
double& MTKd =                  SettingVariable[MTKdIndex];
double& MTSp =                  SettingVariable[MTSpIndex];
double& MTWaterToGrainRatio =   SettingVariable[MTWaterToGrainRatioIndex];
double& MTGrainTemp =           SettingVariable[MTGrainTempIndex];
double& MTInitialWaterTemp =    SettingVariable[MTInitialWaterTempIndex];
double& MTRampOffTemp =         SettingVariable[MTRampOffTempIndex];
double& MTPIDStartTemp =        SettingVariable[MTPIDStartTempIndex];
double& MashTime =              SettingVariable[MashTimeIndex] = 1000;
double& BoilTime =              SettingVariable[BoilTimeIndex];
double& FirstAddTime =          SettingVariable[FirstAddTimeIndex];
double& SecondAddTime =         SettingVariable[SecondAddTimeIndex];
double& ThirdAddTime =          SettingVariable[ThirdAddTimeIndex];
long unsigned int MTStartTime = 0;
long unsigned int MTActTime = (long unsigned int) SettingVariable[MashTimeIndex];

double MTActT = 0; //Mash Tun Actual Temperature
// MTActTime = SettingVariable[MashTimeIndex] + millis();
//int MTPumpState;
//int MTPumpSpeed;
//int MTMixerState;
//int MTMixerSpeed;

// volatile long MTSTSteadyStateTime;
// volatile long MTMashTime;

double MTStrikeTemp;
double Input;
double Output;
// volatile long onTime = 0;
boolean time_state = false;
float elapsed = 0.00;
float time_hold;

  //MT=Mash Tun
  // double MTActTime;
  //BK=Boil Kettle
  double BKActBoilTime;
 // int BKPumpState;
 // int BKPumpSpeed;
  double BKActFirstAddTime;
  double BKActSecondAddTime;
  double BKActThirdAddTime;


// EEPROM addresses for persisted data
const int MTSTKpAddress = 0;

const int MTSTKiAddress = 8;
const int MTSTKdAddress = 16;
const int MTKpAddress = 24;
const int MTKiAddress = 32;
const int MTKdAddress = 40;
const int MTSpAddress = 48;
const int MTWaterToGrainRatioAddress = 56;
const int MTGrainTempAddress = 64;
const int MTInitialWaterTempAddress = 72;
const int MTRampOffTempAddress = 80;
const int MTPIDStartTempAddress = 88;
const int MTPumpSpeedAddress = 96;
const int MTMixerSpeedAddress = 104;
//added Timer values 4/30/22
const int MashTimeAddress = 112;
const int BoilTimeAddress = 120;
const int FirstAddTimeAddress = 128;
const int SecondAddTimeAddress = 136;
const int ThirdAddTimeAddress = 144;

/* MOVED TO DRIVEOUTPUT.H
  // 10 second Time Proportional Output window
 int WindowSize = 10000;
 // unsigned long windowStartTime;
*/

////////////////////////////////////////////////////////////////////////////////////
// SEPTEMBER 8 2022
// Mash timer displays 0:00:91 when going from :11 to :08
DriveOutputStruct DriveOutput;

const int logInterval = 1000; // log every 10 seconds
long lastLogTime = 0;
//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &MTStrikeTemp, SettingVariable[MTSTKpIndex], SettingVariable[MTSTKiIndex], SettingVariable[MTSTKdIndex], P_ON_M, DIRECT);

int pixel_x, pixel_y;     //Touch_getXY() updates global vars
bool Touch_getXY(void)
{
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);      //restore shared pins
  pinMode(XM, OUTPUT);
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  if (pressed) {
    // most apps use Portrait or Landscape. No need for all 4 cases
    switch (tft.getRotation()) {
      case 0:
        pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());
        pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
        break;
      case 1:
        pixel_x = map(p.y, TS_TOP, TS_BOT, 0, tft.width());
        pixel_y = map(p.x, TS_RT, TS_LEFT, 0, tft.height());
        break;
      case 2:
        pixel_x = map(p.x, TS_RT, TS_LEFT, 0, tft.width());
        pixel_y = map(p.y, TS_BOT, TS_TOP, 0, tft.height());
        break;
      case 3:
        pixel_x = map(p.y, TS_BOT, TS_TOP, 0, tft.width());
        pixel_y = map(p.y, TS_LEFT, TS_RT, 0, tft.height());
        break;
    }
  }
  return pressed;
}


//Button names in CamelCase, Screen names in numbers

//Navigation Buttons
Adafruit_GFX_Button Home_btn, Pid_btn;
//Pump_Mix_btn, Recipe_btn;

//Home Screen Buttons
Adafruit_GFX_Button MTRun_btn, MTPause_btn, MTStrike_btn, MTMash_btn;
//Adafruit_GFX_Button MTMixerOn_btn, MTMixerOff_btn, MTPumpOn_btn, MTPumpOff_btn;
Adafruit_GFX_Button MTTimerCancel_btn, BKTimerStart_btn, BKTimerCancel_btn, BKFirstAddCancel_btn, BKSecondAddCancel_btn, BKThirdAddCancel_btn;
//PID Screen Buttons
Adafruit_GFX_Button SelMTSTKp_btn, SelMTSTKi_btn, SelMTSTKd_btn;
Adafruit_GFX_Button SelMTKp_btn, SelMTKi_btn, SelMTKd_btn;
Adafruit_GFX_Button SelMTSp_btn, SelMTWaterToGrainRatio_btn, SelMTGrainTemp_btn, SelMTInitialWaterTemp_btn;
Adafruit_GFX_Button SelMTRampOffTemp_btn, SelMTPIDStartTemp_btn;
// Increment / Decrement buttons
Adafruit_GFX_Button Dec10_btn, Dec1_btn, Dectenth_btn, Inctenth_btn, Inc1_btn, Inc10_btn;

Adafruit_GFX_Button SelMTTimer_btn;
Adafruit_GFX_Button SelBKTimer_btn, SelBKFirstAdd_btn, SelBKSecondAdd_btn, SelBKThirdAdd_btn;
//Adafruit_GFX_Button SelMTPumpSpeed_btn, SelMTMixerSpeed_btn;

// Time Increment / Decrement buttons
Adafruit_GFX_Button Dec10Min_btn, Dec1Min_btn, Dec10Sec_btn, Dec1Sec_btn, Inc1Sec_btn, Inc10Sec_btn, Inc1Min_btn, Inc10Min_btn;
// Button Arrays
Adafruit_GFX_Button *screen0[] = {&Home_btn, &Pid_btn,
                                  //&Pump_Mix_btn, &Recipe_btn,
                                  &MTRun_btn, &MTPause_btn, &MTStrike_btn, &MTMash_btn,
                                  //&MTMixerOn_btn, &MTMixerOff_btn, &MTPumpOn_btn, &MTPumpOff_btn,
                                  &MTTimerCancel_btn, &BKTimerStart_btn, &BKTimerCancel_btn, &BKFirstAddCancel_btn,
                                  &BKSecondAddCancel_btn, &BKThirdAddCancel_btn,
                                  NULL
                                 };
Adafruit_GFX_Button *screen1[] = {&Home_btn, &Pid_btn,
                                  //&Pump_Mix_btn, &Recipe_btn,

                                  &SelMTSTKp_btn, &SelMTSTKi_btn, &SelMTSTKd_btn,
                                  &SelMTKp_btn, &SelMTKi_btn, &SelMTKd_btn,

                                  &SelMTSp_btn, &SelMTWaterToGrainRatio_btn, &SelMTGrainTemp_btn,
                                  &SelMTInitialWaterTemp_btn, &SelMTRampOffTemp_btn, &SelMTPIDStartTemp_btn,
                                  //&SelMTPumpSpeed_btn, &SelMTMixerSpeed_btn,
                                  &Dec10_btn, &Dec1_btn, &Dectenth_btn, &Inctenth_btn, &Inc1_btn, &Inc10_btn,
                                  &SelMTTimer_btn,
                                  &SelBKTimer_btn, &SelBKFirstAdd_btn, &SelBKSecondAdd_btn, &SelBKThirdAdd_btn,
                                  &Dec10Min_btn, &Dec1Min_btn, &Dec10Sec_btn, &Dec1Sec_btn, &Inc1Sec_btn, &Inc10Sec_btn, &Inc1Min_btn, &Inc10Min_btn,
                                  NULL
                                 };

//Adafruit_GFX_Button *screen2[] = {&Home_btn, &Pid_btn, &Pump_Mix_btn, &Recipe_btn, NULL };
//Adafruit_GFX_Button *screen3[] = {&Home_btn, &Pid_btn, &Pump_Mix_btn, &Recipe_btn, NULL };

//I don't know what this section does: (commented out 1/18/23)
// #define redled   10
// #define greenled 11
// #define blueled  12
int flag = 0, screen = 0;
//  GLOBAL VARIABLES, CONSTRUCTORS
// const int LED = A5;     //which pin for LED


//**********************************************************
//Code from PID_Controller_v20g
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress tempSensor;
//*********************************************************


// MARCH 22 2022
// The Draw Screen and Check Button functions have been moved to screendraw.h
void CheckBottomButtonPress ( void );
void CheckSettingButtonPress ( void );
void CheckIncDecButtonPress ( void );
//Check Button functions for Pause/Run; Strike/Mash
void CheckPauseButtonPress (void);
void CheckRunButtonPress (void);
void CheckStrikeButtonPress (void);
void CheckMashButtonPress (void);

// UPDATE OCT 12 2022 /////////////////////////////////////////////////////////////////////////////////////
// Check Button functions for Home ScreenTimer Cancel
// finally uncommenting this
void CheckMTTimerCancelButtonPress (void);
void CheckBKTimerCancelButtonPress (void);
void CheckFirstAddCancelButtonPress (void);
void CheckSecondAddCancelButtonPress (void);
void CheckThirdAddCancelButtonPress (void);
void CheckTimeIncDecButtonPress ( void );
//*********************************************************
// States for the State Machine
//*********************************************************
// 4/14/22 moved to screendraw.h
//*********************************************************

// 3/13/22 globally accessible opState MOVED TO SCREENDRAW.H
// stateList globalState = stateList::OFF;

/////////////////////// OCTOBER 2 2022 MOVED FUNCTION DEFINITIONS ABOVE THEIR USE IN MAIN CODE
// ************************************************
// Write floating point values to EEPROM
// ************************************************
void EEPROM_writeDouble(int address, double value) {
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address++, *p++);
  }
}

// ************************************************
// Read floating point values from EEPROM
// ************************************************
double EEPROM_readDouble(int address) {
  double value = 0.0;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    *p++ = EEPROM.read(address++);
  }
  return value;
}

// ************************************************
// Save any parameter changes to EEPROM
// ************************************************
void SaveParameters() {
  if (SettingVariable[MTSTKpIndex] != EEPROM_readDouble(MTSTKpAddress)) {
    EEPROM_writeDouble(MTSTKpAddress, SettingVariable[MTSTKpIndex]);
  }

  if (SettingVariable[MTSTKiIndex] != EEPROM_readDouble(MTSTKiAddress)) {
    EEPROM_writeDouble(MTSTKiAddress, SettingVariable[MTSTKiIndex]);
  }
  if (SettingVariable[MTSTKdIndex] != EEPROM_readDouble(MTSTKdAddress)) {
    EEPROM_writeDouble(MTSTKdAddress, SettingVariable[MTSTKdIndex]);
  }
  if (SettingVariable[MTKpIndex] != EEPROM_readDouble(MTKpAddress)) {
    EEPROM_writeDouble(MTKpAddress, SettingVariable[MTKpIndex]);
  }
  if (SettingVariable[MTKiIndex] != EEPROM_readDouble(MTKiAddress)) {
    EEPROM_writeDouble(MTKiAddress, SettingVariable[MTKiIndex]);
  }
  if (SettingVariable[MTKdIndex] != EEPROM_readDouble(MTKdAddress)) {
    EEPROM_writeDouble(MTKdAddress, SettingVariable[MTKdIndex]);
  }
  if (SettingVariable[MTSpIndex] != EEPROM_readDouble(MTSpAddress)) {
    EEPROM_writeDouble(MTSpAddress, SettingVariable[MTSpIndex]);
  }
  if (SettingVariable[MTWaterToGrainRatioIndex] != EEPROM_readDouble(MTWaterToGrainRatioAddress)) {
    EEPROM_writeDouble(MTWaterToGrainRatioAddress, SettingVariable[MTWaterToGrainRatioIndex]);
  }
  if (SettingVariable[MTGrainTempIndex] != EEPROM_readDouble(MTGrainTempAddress)) {
    EEPROM_writeDouble(MTGrainTempAddress, SettingVariable[MTGrainTempIndex]);
  }
  if (SettingVariable[MTInitialWaterTempIndex] != EEPROM_readDouble(MTInitialWaterTempAddress)) {
    EEPROM_writeDouble(MTInitialWaterTempAddress, SettingVariable[MTInitialWaterTempIndex]);
  }
  if (SettingVariable[MTRampOffTempIndex] != EEPROM_readDouble(MTRampOffTempAddress)) {
    EEPROM_writeDouble(MTRampOffTempAddress, SettingVariable[MTRampOffTempIndex]);
  }
  if (SettingVariable[MTPIDStartTempIndex] != EEPROM_readDouble(MTPIDStartTempAddress)) {
    EEPROM_writeDouble(MTPIDStartTempAddress, SettingVariable[MTPIDStartTempIndex]);
  }
  if (SettingVariable[MashTimeIndex] != EEPROM_readDouble(MashTimeAddress)) {
    EEPROM_writeDouble(MashTimeAddress, SettingVariable[MashTimeIndex]);
  }
  if (SettingVariable[BoilTimeIndex] != EEPROM_readDouble(BoilTimeAddress)) {
    EEPROM_writeDouble(BoilTimeAddress, SettingVariable[BoilTimeIndex]);

  } if (SettingVariable[FirstAddTimeIndex] != EEPROM_readDouble(FirstAddTimeAddress)) {
    EEPROM_writeDouble(FirstAddTimeAddress, SettingVariable[FirstAddTimeIndex]);

  } if (SettingVariable[SecondAddTimeIndex] != EEPROM_readDouble(SecondAddTimeAddress)) {
    EEPROM_writeDouble(SecondAddTimeAddress, SettingVariable[SecondAddTimeIndex]);

  } if (SettingVariable[ThirdAddTimeIndex] != EEPROM_readDouble(ThirdAddTimeAddress)) {
    EEPROM_writeDouble(ThirdAddTimeAddress, SettingVariable[ThirdAddTimeIndex]);
  }
}


// ************************************************
// Execute the control loop
// ************************************************
void DoControl()
{
  // Read the input:
  // DISCREPANCY BETWEEN IDE OR LIBRARY VERSIONS SEPTEMBER 8 2022
  if (sensors.isConversionAvailable(0)) { /////////////////////////////////////////////////////////////////////////////
    Input = sensors.getTempF(tempSensor);//switch from Celcius to Fahrenheit
    sensors.requestTemperatures(); // prime the pump for the next one - but don't wait
  }

  // if (tuning) { // run the auto-tuner
  //     if (aTune.Runtime()) { // returns 'true' when done
  //         FinishAutoTune();
  //     }
  //  } else { // Execute control algorithm
  myPID.Compute();
  // }

  // Time Proportional relay state is updated regularly via timer interrupt.
  DriveOutput.onTime = Output;

  if (time_state == true)
    elapsed = (millis() / 60000.00) - time_hold;
  /*
     if (elapsed > set_time) {
         alarm = true;
         if (sound_state = true) {
             tone(sound_pin, 800, 200);
             delay(200);
             sound_state = false;
         } else {
             sound_state = true;
             delay(200);
         }

     }
  */
}

StateSystem sys;

stateList StateSystem::checkState() {
  if ( MTRun_btn.justPressed() )
    opState = stateList::RAMPUP;
  if ( MTPause_btn.justPressed() ){
    MashTimerObj.pause();
    opState = stateList::OFF;
    //UnPaused = false;
    //TimeIncludingPreviousPauses += TimeSinceUnPause;
    //TimeSinceUnPause = 0;
  }
  if ( MTStrike_btn.justPressed() ) {
    opState = stateList::STRIKEPID;
  }
  if ( MTMash_btn.justPressed() ) {
    MashTimerObj.start_from_beginning();
    opState = stateList::MASHPID;
  }
  if ( MTTimerCancel_btn.isPressed() ){
     noTone(43) ;
     // MTActTime = SettingVariable[MashTimeIndex];
     opState = stateList::OFF;
     ActualTimerOut = false;
   }

// OCTOBER 2 2022 NOT DOMINO EFFECT
  if ( BKTimerStart_btn.justPressed() ) {
    BoilTimerObj.start_from_beginning();
    AddTimerObj1.start_from_beginning();
    AddTimerObj2.start_from_beginning();
    AddTimerObj3.start_from_beginning();
  }

MTTimerCancel_btn.initButton(&tft, 290, 230, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
  BKTimerStart_btn.initButton(&tft, 245, 313, 35, 25, TFT_BLACK, TFT_GREEN, TFT_BLACK, "ON", 2);
  BKTimerCancel_btn.initButton(&tft, 290, 313, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
  BKFirstAddCancel_btn.initButton(&tft, 290, 343, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
  BKSecondAddCancel_btn.initButton(&tft, 290, 373, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
  BKThirdAddCancel_btn.initButton(&tft, 290, 403, 45, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);

// OCT 12 2022
// Boil and Addtl Timer "OFF" buttons now achieve same effect as Mash Timer "PAUSE" when timer is done
  if ( MTTimerCancel_btn.justPressed() ) {
    MashTimerObj.pause();
    opState = stateList::OFF;
  }
  if ( BKTimerCancel_btn.justPressed() ) {
    BoilTimerObj.pause();
    AddTimerObj1.pause();
    AddTimerObj2.pause();
    AddTimerObj3.pause();
  }
  if ( BKFirstAddCancel_btn.justPressed() ) {
    AddTimerObj1.pause();
  }
  if ( BKSecondAddCancel_btn.justPressed() ) {
    AddTimerObj2.pause();
  }
  if ( BKThirdAddCancel_btn.justPressed() ) {
    AddTimerObj3.pause();
  }
  /*
  if ( BoilTimerObj.doneNotReset && AddTimerObj1.tStateNow == 0 ) {
    //BoilTimerObj.doneNotReset = false;
    BoilTimerObj.reset_timer();
    AddTimerObj1.start_from_beginning();
  }
  if ( AddTimerObj1.doneNotReset && AddTimerObj2.tStateNow == 0 ) {
    //AddTimerObj1.doneNotReset = false;
    AddTimerObj1.reset_timer();
    AddTimerObj2.start_from_beginning();
  }
  if ( AddTimerObj2.doneNotReset && AddTimerObj3.tStateNow == 0 ) {
    //AddTimerObj2.doneNotReset = false;
    AddTimerObj2.reset_timer();
    AddTimerObj3.start_from_beginning();
  }
  if ( AddTimerObj3.doneNotReset ) {
  //  BoilTimerObj.doneNotReset = false;
  //  AddTimerObj1.start_from_beginning();
    AddTimerObj3.reset_timer();
  } */
  
  return opState;
}

void StateSystem::doState() {
  // V11d: Change MASHStartTime to -1 when not in MashPID state
  // when doing V12 overhaul of state system, this needs to be changed drastically
  switch (opState) {
    case stateList::OFF:
      MASHStartTime = -1;
      Off();
      break;

    case stateList::RAMPUP:
      MASHStartTime = -1;
      RampUp();
      break;

    case stateList::RAMPOFF:
      MASHStartTime = -1;
      RampOff();
      break;

    case stateList::STRIKEPID:
      MASHStartTime = -1;
      StrikePID();
      break;

    case stateList::MASHPID:
      MashPID();
      break;

  }

  

}

void StateSystem::Off() {
  /*
    tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    tft.setCursor(130, 410); //setCursor(int16_t x, int16_t y)
    tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    tft.print("OFF");
  */

  digitalWrite(RelayPin, LOW);
}

void StateSystem::RampUp() {
  SaveParameters();
  myPID.SetMode(MANUAL);
  //testing to see if the commands below activate PID control
  // myPID.SetMode(AUTOMATIC);
  // DoControl();
  //DriveOutput();
  // This section works for Ramp Up
  /*
    tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    tft.setCursor(100, 410); //setCursor(int16_t x, int16_t y)
    tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    tft.print("Ramp Up Step");
  */

  if ( (MTStrikeTemp - Input) >= SettingVariable[MTRampOffTempIndex] ) { // Should another variable other than MTStrikeTemp be used?
    opState = stateList::RAMPUP;
    digitalWrite(RelayPin, HIGH);
  } else {
    opState = stateList::RAMPOFF;
    digitalWrite(RelayPin, LOW);
  }

  // periodically log to serial port in csv format
  if (millis() - lastLogTime > logInterval) {
    lastLogTime = millis();

    Serial.print("DATA,TIME,");
    Serial.print("Ramp Up");
    Serial.print(",");
    Serial.print(Input);
    Serial.print(",");
    Serial.print(Output / 100); // 3/6/22 LOOK AT EXCEL SHEET TO SEE WHAT OUTPUT really is
    Serial.print(",");
    Serial.print(millis() / 1000);
    Serial.print(",");
    Serial.print(MTStrikeTemp);
    Serial.print(",");
    Serial.print(0);
    Serial.print(",");
    Serial.print(0);
    Serial.print(",");
    Serial.print(0);
    Serial.print(",");
    Serial.print(millis());
Serial.print(",");
    Serial.print(MTStartTime);
    Serial.print(",");
    Serial.print(MTActTime);
    Serial.print(",");
    Serial.print(SettingVariable[MashTimeIndex]);
    Serial.print(",");
    Serial.print(TimeIncludingPreviousPauses);
    Serial.print(",");
    Serial.print(TimeSinceUnPause);
    Serial.print(",");
    Serial.print(MashTime);
    Serial.print(",");
    Serial.print(ActualTimerOut);
    Serial.print(",");
    Serial.print(UnPaused);
    Serial.print(",");
    Serial.println(UnPauseTime);
  }
delay(500);
}


void StateSystem::RampOff() {
  /*
    tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    tft.setCursor(100, 410); //setCursor(int16_t x, int16_t y)
    tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    tft.print("Ramp Off Step");
  */

  // JULY 17 2022 TED
  // Logic to switch back into RampUp if 1 degree or more has been lost from Max Temperature
  // Want temp to coast upward, not down
  if ( sensors.getTempF(tempSensor) > MaxRampOffTemp ) {
    // Increase MaxRampOffTemp if necessary
    MaxRampOffTemp = sensors.getTempF(tempSensor);
  }
  if ( sensors.getTempF(tempSensor) < MaxRampOffTemp - 1 ) {
    opState = stateList::RAMPUP;
    digitalWrite(RelayPin, HIGH);
  }
  
  myPID.SetMode(MANUAL);
  digitalWrite(RelayPin, LOW);
  if ((MTStrikeTemp - Input) <= SettingVariable[MTPIDStartTempIndex]) { // Should this just be Input lhs in comparison?
    opState = stateList::STRIKEPID;
  }

  // periodically log to serial port in csv format
  if (millis() - lastLogTime > logInterval) {
    lastLogTime = millis();

    Serial.print("DATA,TIME,");
    Serial.print("Ramp Off");
    Serial.print(",");
    Serial.print(Input);
    Serial.print(",");
    Serial.print(Output / 100);
    Serial.print(",");
    Serial.print(millis() / 1000);
    Serial.print(",");
    Serial.print(MTStrikeTemp);
    Serial.print(",");
    Serial.print(0);
    Serial.print(",");
    Serial.print(0);
    Serial.print(",");
    Serial.print(0);
    
    Serial.print(",");
    Serial.print(millis());
Serial.print(",");
    Serial.print(MTStartTime);
    Serial.print(",");
    Serial.print(MTActTime);
    Serial.print(",");
    Serial.print(SettingVariable[MashTimeIndex]);
    Serial.print(",");
    Serial.print(TimeIncludingPreviousPauses);
    Serial.print(",");
    Serial.print(TimeSinceUnPause);
    Serial.print(",");
    Serial.print(MashTime);
    Serial.print(",");
    Serial.print(ActualTimerOut);
    Serial.print(",");
    Serial.print(UnPaused);
    Serial.print(",");
    Serial.println(UnPauseTime);
  }
  delay(500);
}

void StateSystem::StrikePID()
{
  // 3/6/22 It appears the issue of not turning on relaypin is here
  /*
    tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    tft.setCursor(80, 410); //setCursor(int16_t x, int16_t y)
    tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    tft.print("Strike PID Step");
  */
  myPID.SetMode(AUTOMATIC); // Adding back in for 12L1. removing from 12c Trial 2 to see if this gets PID control to turn on

  DoControl();
  //DriveOutput(); Removing in 12c as it was not in sketches prior to touchscreen

  // 3/6/22 Need a way to determine at what Power (variable: Output) to turn RelayPin on/off
  // 3/8 Changing threshold to 55
  //Commenting out this section in 12c to see if removing DriveOutput from StrikePID results in sinusoidal variation around setpoint
  /*if ( Output > 55 )
      digitalWrite( RelayPin , HIGH );
    else
      digitalWrite( RelayPin , LOW );
  */
  // 3/8/22 Need to convert Output into timing on/Off
  // Need to tell time between last change in Output
  // For display purposes that's the logInterval
  // But output is constantly updated otherwise
  // Need to take an integral?
  // Sum across some span of time
  // Above code already turns plate on and off as fast as it can go!

  // periodically log to serial port in csv format

  if (millis() - lastLogTime > logInterval) {
    lastLogTime = millis();
long unsigned int MTStartTime = millis();
    Serial.print("DATA,TIME,");
    Serial.print("Strike PID");
    Serial.print(",");
    Serial.print(Input);
    Serial.print(",");
    Serial.print(Output); // Output appears to already be in percent format
    Serial.print(",");
    Serial.print(millis() / 1000);
    Serial.print(",");
    Serial.print(MTStrikeTemp);
    Serial.print(",");
    Serial.print(SettingVariable[MTSTKpIndex]);
    Serial.print(",");
    Serial.print(SettingVariable[MTSTKiIndex]);
    Serial.print(",");
    Serial.print(SettingVariable[MTSTKdIndex]);
Serial.print(",");
    Serial.print(millis());
Serial.print(",");
    Serial.print(MTStartTime);
    Serial.print(",");
    Serial.print(MTActTime);
    Serial.print(",");
    Serial.print(SettingVariable[MashTimeIndex]);
    Serial.print(",");
    Serial.print(TimeIncludingPreviousPauses);
    Serial.print(",");
    Serial.print(TimeSinceUnPause);
    Serial.print(",");
    Serial.print(MashTime);
    Serial.print(",");
    Serial.print(ActualTimerOut);
    Serial.print(",");
    Serial.print(UnPaused);
    Serial.print(",");
    Serial.println(UnPauseTime);
  }
  delay(500);
}
void StateSystem::MashPID() {

  // JUNE 19 2022
  // New variables who dis
  // Manually set Mash timer to 10 seconds for debugging stuff
  // MashTime = 10000;

  ////// SEPTEMBER 26 2022
  // Commenting this out because time is handled by Timer struct
  /*
  if ( !UnPaused && !ActualTimerOut ) { // if paused and the timer hasn't yet run out
    UnPaused = true;
    UnPauseTime = millis();
  }

  if ( UnPaused && !ActualTimerOut ) { // if timer running and not yet run out
    TimeSinceUnPause = millis() - UnPauseTime; // Time Since MOST RECENT UnPause
    // TimeIncludingPreviousPauses = TimeSinceUnPause;
    // TimeIncludingPreviousPauses += millis() - UnPauseTime;
    //MTActTime = SettingVariable[MashTimeIndex] - TimeIncludingPreviousPauses - TimeSinceUnPause;
    if ( MTActTime < 333 || MTActTime > (MashTime+10000) ) {
      ActualTimerOut = true;
      UnPaused = false;
      //MTActTime = 0;
    }
  }
  if ( MTActTime < 333 ) { // random value under one second just to check if we should zero out values
    // should this be altered such that random auto-ending of timer when MTActTime set below 333 does not occur? is this an issue?      
    ActualTimerOut = true;
    UnPaused = false;
    //MTActTime = 0;
    MashEndSignal();
  /*  if ( MTTimerCancel_btn.justPressed() )
   noTone(43) ;
   return; ///  ?
  } */

// const long unsigned int MTStartTime = millis();
  // V11d changes:
  // if first time reaching MashPID state (if MASHStartTime == -1)
  // set MASHStartTime to millis()
  /* if ( MASHStartTime == -1 ) {
    MASHStartTime = millis();
  } */
// const long unsigned int MTStartTime = millis();
  // if 1 minute has elapsed since MASHStartTime was set, BEEP
  /*
   * if ( millis() - MASHStartTime > MASHTimer ) {
    // DO BEEPS Somehow
    // for now, Serial println the time elapsed since MASHStartTime
    Serial.println( millis() - MASHStartTime );

  } */
  
// const long unsigned int MTStartTime = millis();
// 5/23/22 Add Mash Time Count Down
// MTActTime = SettingVariable[MashTimeIndex];
  //Switch to Mash parameters. MTSp needs to be changed to double to work in this function
  myPID.SetTunings( SettingVariable[MTKpIndex], SettingVariable[MTKiIndex], SettingVariable[MTKpIndex] );
  myPID.SetMode(AUTOMATIC);
  DoControl();
  DriveOutput.DoDriveOutput();
  // periodically log to serial port in csv format
// MTActTime=SettingVariable[MashTimeIndex]-millis();
  if (millis() - lastLogTime > logInterval) {
    lastLogTime = millis();
// const long unsigned int MTStartTime = millis(); 
   
 // MTActTime=SettingVariable[MashTimeIndex]-millis();
 // MTActTime=SettingVariable[MashTimeIndex]-(millis()-MTStartTime);
 //MTActTime=SettingVariable[MashTimeIndex]-500;
// DrawHomeScreen( tft , SettingVariable , sys , Input , Output );
    Serial.print("DATA,TIME,");
    Serial.print("Mash PID");
    Serial.print(",");
    Serial.print(Input); // 3/6/22 Input is temperature measured
    Serial.print(",");
    Serial.print(Output / 100); // 3/6/22 Output / 100 is percentage of power sent to pin?
    Serial.print(",");
    Serial.print(millis() / 1000);
    Serial.print(",");
    Serial.print(SettingVariable[MTSpIndex]);
    Serial.print(",");
    Serial.print(SettingVariable[MTKpIndex]);
    Serial.print(",");
    Serial.print(SettingVariable[MTKiIndex]);
    Serial.print(",");
    Serial.print(SettingVariable[MTKdIndex]);
    Serial.print(",");
    Serial.print(millis());
    Serial.print(",");
    Serial.print(MTStartTime);
    Serial.print(",");
    Serial.print(MTActTime);
    Serial.print(",");
    Serial.print(SettingVariable[MashTimeIndex]);
    Serial.print(",");
    Serial.print(TimeIncludingPreviousPauses);
    Serial.print(",");
    Serial.print(TimeSinceUnPause);
    Serial.print(",");
    Serial.print(MashTime);
    Serial.print(",");
    Serial.print(ActualTimerOut);
    Serial.print(",");
    Serial.print(UnPaused);
    Serial.print(",");
    Serial.println(UnPauseTime);
  }
  // OCT 2 2022 REMOVING DELAY(500)
  //delay(500);
}



void setup() {
  // put your setup code here, to run once:
  // opens serial port, sets data rate to 9600 bps
  Serial.begin(9600);
  Serial.println("CLEARDATA");
  Serial.println("LABEL, Time, Step, Temp (F), Power (%), Time (s), Sp, P, I, D,Millis(),MTStartTime, MTActTime, SettingVariable[MashTimeIndex],TimeIncludingPreviousPauses,TimeSinceUnPause,MashTime,ActualTimerOut,UnPaused,UnPauseTime");
  //************************************
  //Code from PID_Controller_v20g
  // Initialize Relay Control:


  pinMode(RelayPin, OUTPUT); // Output mode to drive relay
  digitalWrite(RelayPin, LOW); // make sure it is off to start

  // Set up Ground & Power for the sensor from GPIO pins
  //pinMode(ONE_WIRE_GND, OUTPUT); // COMMENTED OUT TO ACCOMONDATE SPEAKER
  //digitalWrite(ONE_WIRE_GND, LOW);

  pinMode(ONE_WIRE_PWR, OUTPUT);
  digitalWrite(ONE_WIRE_PWR, HIGH); // 3/6/22 What's the difference between RelayPin and ONE_WIRE_PWR ?

// Add BuzzerPin definition 1/16/23
pinMode(BuzzerPin,OUTPUT);
pinMode(MTLEDPin,OUTPUT);
pinMode(BKLEDPin,OUTPUT);
pinMode(BKFirstAddLEDPin,OUTPUT);
pinMode(BKSecondAddLEDPin,OUTPUT);
pinMode(BKThirdAddLEDPin,OUTPUT);

/*
pinMode (MTLEDRedPin,OUTPUT);
pinMode (MTLEDGreenPin,OUTPUT);
pinMode (MTLEDBluePin,OUTPUT);
pinMode (BKLEDRedPin,OUTPUT);
pinMode (BKLEDGreenPin,OUTPUT);
pinMode (BKLEDBluePin,OUTPUT);
pinMode (BKFirstAddLEDRedPin,OUTPUT);
pinMode (BKFirstAddLEDGreenPin,OUTPUT);
pinMode (BKFirstAddLEDBluePin,OUTPUT);
pinMode (BKSecondAddLEDRedPin,OUTPUT);
pinMode (BKSecondAddLEDGreenPin,OUTPUT);
pinMode (BKSecondAddLEDBluePin,OUTPUT);
pinMode (BKThirdAddLEDRedPin,OUTPUT);
pinMode (BKThirdAddLEDGreenPin,OUTPUT);
pinMode (BKThirdAddLEDBluePin,OUTPUT);

 */
/*
// 1/19/23  Boil Timer Logic
 if (BoilActTime == 0 ) {
  //tone(BuzzerPin,250);
  digitalWrite(BKLEDPin,HIGH);
}
*/
  // Start up the DS18B20 One Wire Temperature Sensor

  sensors.begin();
  if (!sensors.getAddress(tempSensor, 0)) {
    // NO LCD OBJECT, THESE LINES COMMENTED OUT FOR NOW
    //lcd.setCursor(0, 1);
    //lcd.print("Sensor");
  }
  sensors.setResolution(tempSensor, 12);
  sensors.setWaitForConversion(false);
  //*************************************

  tft.begin(tft.readID());
  tft.setRotation(0);     //0-Portrait, 1=Landscape

 // pinMode(LED, OUTPUT);
 // pinMode(redled, OUTPUT);
 // pinMode(greenled, OUTPUT);
 // pinMode(blueled, OUTPUT);

  // INITIALIZE BUTTONS
  Home_btn.initButton(&tft, 40, 460, 60, 25, TFT_BLACK, TFT_WHITE, TFT_BLACK, "HOME", 2);
  Pid_btn.initButton(&tft, 256, 460, 110, 25, TFT_BLACK, TFT_WHITE, TFT_BLACK, "SETTINGS", 2);
  //Pump_Mix_btn.initButton(&tft, 192, 470, 30, 20, TFT_BLACK, TFT_WHITE, TFT_BLACK, "Pump/Mix", 1);
  //Recipe_btn.initButton(&tft, 266, 470, 30, 20, TFT_BLACK, TFT_WHITE, TFT_BLACK, "Recipe", 1);
  MTRun_btn.initButton(&tft, 210, 80, 40, 25, TFT_BLACK, TFT_GREEN, TFT_BLACK, "RUN", 2);
  MTPause_btn.initButton(&tft, 280, 80, 70, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "PAUSE", 2);
  MTStrike_btn.initButton(&tft, 217, 110, 80, 25, TFT_BLACK, TFT_WHITE, TFT_BLACK, "STRIKE", 2);
  MTMash_btn.initButton(&tft, 290, 110, 60, 25, TFT_BLACK, TFT_WHITE, TFT_BLACK, "MASH", 2);
  //MTMixerOn_btn.initButton(&tft, 250, 140, 40, 25, TFT_BLACK, TFT_GREEN, TFT_BLACK, "ON", 2);
  //MTMixerOff_btn.initButton(&tft, 295, 140, 40, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
  //MTPumpOn_btn.initButton(&tft, 250, 170, 41, 25, TFT_BLACK, TFT_GREEN, TFT_BLACK, "ON", 2);
  //MTPumpOff_btn.initButton(&tft, 295, 170, 41, 25, TFT_BLACK, TFT_RED, TFT_BLACK, "OFF", 2);
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
  // SelMTPumpSpeed_btn.initButton(&tft, 215, 313, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
  // SelMTMixerSpeed_btn.initButton(&tft, 215, 333, 30, 15, TFT_BLACK, TFT_ORANGE, TFT_BLACK, "SEL", 1);
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

  tft.fillScreen(TFT_LIGHTGREY);
  //I'm not sure what these 2 lines below do
  //led_btn.drawButton(false);
  //RGB_btn.drawButton(false);

  Home_btn.drawButton(false);
  Pid_btn.drawButton(false);


  // FIXES BUG IN WHICH NO HOME SCREEN BUTTONS WERE DRAWN UNTIL HOME BUTTON WAS PRESSED
  draw_button_list(screen0);
  // Initialize the PID and related variables
  LoadParameters();
  myPID.SetTunings(SettingVariable[MTSTKpIndex], SettingVariable[MTSTKiIndex], SettingVariable[MTSTKdIndex]);
  //Should this be SettingVariable[MTSTKpIndex] ?
  //How is this affected by changing MTSTKp to int& = SettingVariable[MTSTKpIndex] ?
  //Changed to SettingVariable[...Index]

  myPID.SetSampleTime(1000);
  myPID.SetOutputLimits(0, DriveOutput.WindowSize);

  sys.MashTimerObj.tLengthS = SettingVariable[MashTimeIndex];
  sys.BoilTimerObj.tLengthS = SettingVariable[BoilTimeIndex];
  sys.AddTimerObj1.tLengthS = SettingVariable[FirstAddTimeIndex];
  sys.AddTimerObj2.tLengthS = SettingVariable[SecondAddTimeIndex];
  sys.AddTimerObj3.tLengthS = SettingVariable[ThirdAddTimeIndex];

  //*********************************************
  //Copied from Peltier_PID v12

  // Run timer2 interrupt every 15 ms
  TCCR2A = 0;
  TCCR2B = 1 << CS22 | 1 << CS21 | 1 << CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 |= 1 << TOIE2;
  
}

// ************************************************
// Timer Interrupt Handler
// ************************************************

// previous argument to SIGNAL was : TIMER2_OVF_vect
SIGNAL(TIMER2_OVF_vect ) // function definition?

{
  //  if (opState == OFF) {
  if (globalState == stateList::OFF) {
    digitalWrite(RelayPin, LOW); // make sure relay is off
  }
  else if (globalState == stateList::RAMPUP) {
    digitalWrite(RelayPin, HIGH); // make sure relay is off
  }
  else if
  (globalState == stateList::RAMPOFF) {
    digitalWrite(RelayPin, LOW); // make sure relay is off
  }
  else
  {
     DriveOutput.DoDriveOutput();
  }

}

//*********************End copy from Peltier_PID v12

void loop()
{
  MTStrikeTemp = ((0.2 / SettingVariable[MTWaterToGrainRatioIndex]) * (SettingVariable[MTSpIndex] - SettingVariable[MTGrainTempIndex])) + SettingVariable[MTSpIndex];

  sys.checkState();
  sys.doState();

  static uint16_t color = TFT_WHITE;
  // should probably move this to setup / global variable

  sensors.requestTemperatures();
  Input = sensors.getTempF(tempSensor);//switch from Celcius to Fahrenheit// READ TOUCH
/*
// 1/19/23  Boil Timer Logic
 if (BoilActTime == 0 ) {
  //tone(BuzzerPin,250);
  digitalWrite(BKLEDPin,HIGH);
}
*/

  if (screen == 0) {
    // JUNE 24 2022
    /* if ( !UnPausedBoil && !ActualTimerOutBoil ) { // if paused and the timer hasn't yet run out
      UnPaused = true;
      UnPauseTime = millis();
    } */
    // Unlike MASHPID and mash timer, unpausing of boil and add. timers needs to be controlled some other way
    /*
    if ( !BoilTimerStarted && MTActTime<60000 ) {
        BoilTimerStarted = true;
        UnPausedBoil = true;
    }

    if ( UnPausedBoil && !ActualTimerOutBoil && BoilTimerStarted ) { // if BOIL timer started, running, and not yet run out
      TimeSinceUnPauseBoil = millis() - UnPauseTimeBoil; // Time Since MOST RECENT UnPause
      BoilActTime = SettingVariable[BoilTimeIndex] - TimeIncludingPreviousPausesBoil - TimeSinceUnPauseBoil;
      if ( BoilActTime < 333 || BoilActTime > (MashTime+10000) ) {
        ActualTimerOutBoil = true;
        UnPausedBoil = false;
        BoilActTime = 0;
      }
    }
    if ( BoilActTime < 333 && !ActualTimerOutBoil && BoilTimerStarted ) { // random value under one second just to check if we should zero out values
      ActualTimerOutBoil = true;
      UnPausedBoil = false;
      BoilActTime = 0;
      //MashEndSignal();
    }
    */
// 1/19/23  Boil Timer Logic IN LOOP()
// if (BoilActTime == 0 ) {
/*
if ( sys.MashTimerObj.get_time_count_down() == 0 ){
    tone(BuzzerPin,250);
    digitalWrite(BKLEDPin,HIGH);
  }
  else {digitalWrite(BKLEDPin,LOW);
  noTone(BuzzerPin);
  }
  */

  // FEB 1 2023
  //  make two booleans defaulting to false each loop
  //   conditional statements determine whether to switch to true
  //   Do tone() and digitalWrite() at the end after the conditions!
    bool BuzzerPinOn = false;

// Feb 11 2023 Add Mash Timer LED
    bool MTLEDPinOn = false;
    if ( sys.MashTimerObj.get_time_count_down() == 0) {
      // tone(BuzzerPin, 250);
      BuzzerPinOn = true;
      MTLEDPinOn  = true;
      /*
      analogWrite(MTLEDRedPin,255);
      analogWrite(MTLEDGreenPin,0);
      analogWrite(MTLEDBluePin,0);
      */
    }
    
    bool BKLEDPinOn  = false;
    if ( sys.BoilTimerObj.get_time_count_down() == 0 ){
      // tone(BuzzerPin,250);
      BuzzerPinOn = true;
      BKLEDPinOn  = true;
      /*
      analogWrite(BKLEDRedPin,255);
      analogWrite(BKLEDGreenPin,0);
      analogWrite(BKLEDBluePin,0);
      */
      // digitalWrite(BKLEDPin,HIGH);
    }
    // else {
    //   digitalWrite(BKLEDPin,LOW);
    //   noTone(BuzzerPin);
    // }

    // JAN 31 2023
    // Add check for whether Timers are set to length 0s

    bool BKLEDPinOnFirstAdd = false;
    if ( sys.AddTimerObj1.get_time_count_down() == 0 && SettingVariable[FirstAddTimeIndex] != 0 ){
      // tone(BuzzerPin,250);
      // digitalWrite(BKFirstAddLEDPin,HIGH);
      BuzzerPinOn = true;
      BKLEDPinOnFirstAdd  = true;
      /*
      analogWrite(BKFirstAddLEDRedPin,255);
      analogWrite(BKFirstAddLEDGreenPin,0);
      analogWrite(BKFirstAddLEDBluePin,0);
      */
    }
    // else {
      //   digitalWrite(BKFirstAddLEDPin,LOW);
      //   noTone(BuzzerPin);
    //   } 

    bool BKLEDPinOnSecondAdd = false;
    if ( sys.AddTimerObj2.get_time_count_down() == 0 && SettingVariable[SecondAddTimeIndex] != 0 ){
      // tone(BuzzerPin,250);
      // digitalWrite(BKSecondAddLEDPin,HIGH);
      BuzzerPinOn = true;
      BKLEDPinOnSecondAdd  = true;
      /*
      analogWrite(BKSecondAddLEDRedPin,255);
      analogWrite(BKSecondAddLEDGreenPin,0);
      analogWrite(BKSecondAddLEDBluePin,0);
      */
    }
    //  else {
      //    digitalWrite(BKSecondAddLEDPin,LOW);
      //    noTone(BuzzerPin);
    //    }

    bool BKLEDPinOnThirdAdd = false;
    if ( sys.AddTimerObj3.get_time_count_down() == 0 && SettingVariable[ThirdAddTimeIndex] != 0 ){
      // tone(BuzzerPin,250);
      // digitalWrite(BKThirdAddLEDPin,HIGH);
      BuzzerPinOn = true;
      BKLEDPinOnThirdAdd  = true;
      /*
      analogWrite(BKThirdAddLEDRedPin,255);
      analogWrite(BKThirdAddLEDGreenPin,0);
      analogWrite(BKThirdAddLEDBluePin,0);
      */
    }
    // else {
    //   digitalWrite(BKThirdAddLEDPin,LOW);
    //   noTone(BuzzerPin);
    // }

    // FEB 1 2023 
    // check truthfulness of BuzzerPinOn and BKLEDPinOn, buzz and light up if true
    if ( BuzzerPinOn ) {
      tone( BuzzerPin , BuzzTone );
    } else {
      noTone( BuzzerPin );
    }
    if ( MTLEDPinOn )  {
      digitalWrite(MTLEDPin,HIGH);
      BuzzTone = MTTone;
    } else {
      digitalWrite(MTLEDPin,LOW);
    //BuzzTone = 0;
    }
    if ( BKLEDPinOn )  {
      digitalWrite(BKLEDPin,HIGH);
      BuzzTone = BKTone;
    } else {
      digitalWrite(BKLEDPin,LOW);
    //  BuzzTone = 0;
    }

    if ( BKLEDPinOnFirstAdd )  {
      digitalWrite(BKFirstAddLEDPin,HIGH);
      BuzzTone = FirstAddTone;
    } else {
      digitalWrite(BKFirstAddLEDPin,LOW);
    //  BuzzTone = 0;
    }

    if ( BKLEDPinOnSecondAdd )  {
      digitalWrite(BKSecondAddLEDPin,HIGH);
      BuzzTone = SecondAddTone;
    } else {
      digitalWrite(BKSecondAddLEDPin,LOW);
     // BuzzTone = 0;
    }

    if ( BKLEDPinOnThirdAdd )  {
      digitalWrite(BKThirdAddLEDPin,HIGH);
      BuzzTone = ThirdAddTone;
    } else {
      digitalWrite(BKThirdAddLEDPin,LOW);
     // BuzzTone = 0;
    }
    
    LoadParameters();
    DrawHomeScreen( tft , SettingVariable , sys , Input , Output , MTActTime , BoilActTime , FirstAddActTime , SecondAddActTime , ThirdAddActTime );
    update_button_list(screen0);  //process all buttons
    CheckBottomButtonPress();
  }
  
  if ( screen == 1 ) {
    DrawSettingsScreen( tft , SettingVariable , sys , Input , Output );
    update_button_list(screen1);  //process all buttons
    CheckBottomButtonPress();
    CheckSettingButtonPress();
    CheckIncDecButtonPress();
    CheckTimeIncDecButtonPress();
  }

}




// main program can use isPressed(), justPressed() etc

bool update_button(Adafruit_GFX_Button *b, bool down)
{
  b->press(down && b->contains(pixel_x, pixel_y));
  if (b->justReleased())
    b->drawButton(false);
  if (b->justPressed())
    b->drawButton(true);
  return down;
}

// most screens have different sets of buttons
// life is easier if you process whole list in one go

bool update_button_list(Adafruit_GFX_Button **pb)
{
  bool down = Touch_getXY();
  for (int i = 0 ; pb[i] != NULL; i++) {
    update_button(pb[i], down);
  }
  return down;
}

void draw_button_list(Adafruit_GFX_Button **pb)
{
  tft.fillScreen(TFT_LIGHTGREY);
  for (int i = 0 ; pb[i] != NULL; i++) {
    pb[i]->drawButton(false);
  }
}

// ************************************************
// Load parameters from EEPROM
// ************************************************
void LoadParameters() {
  // Load from EEPROM
  SettingVariable[MTSTKpIndex] = EEPROM_readDouble(MTSTKpAddress);
  SettingVariable[MTSTKiIndex] = EEPROM_readDouble(MTSTKiAddress);
  SettingVariable[MTSTKdIndex] = EEPROM_readDouble(MTSTKdAddress);
  SettingVariable[MTKpIndex] = EEPROM_readDouble(MTKpAddress);
  SettingVariable[MTKiIndex] = EEPROM_readDouble(MTKiAddress);
  SettingVariable[MTKdIndex] = EEPROM_readDouble(MTKdAddress);
  SettingVariable[MTSpIndex] = EEPROM_readDouble(MTSpAddress);
  SettingVariable[MTWaterToGrainRatioIndex] = EEPROM_readDouble(MTWaterToGrainRatioAddress);
  SettingVariable[MTGrainTempIndex] = EEPROM_readDouble(MTGrainTempAddress);
  SettingVariable[MTInitialWaterTempIndex] = EEPROM_readDouble(MTInitialWaterTempAddress);
  SettingVariable[MTRampOffTempIndex] = EEPROM_readDouble(MTRampOffTempAddress);
  SettingVariable[MTPIDStartTempIndex] = EEPROM_readDouble(MTPIDStartTempAddress);
  SettingVariable[MashTimeIndex] = EEPROM_readDouble(MashTimeAddress);
  SettingVariable[BoilTimeIndex] = EEPROM_readDouble(BoilTimeAddress);
  SettingVariable[FirstAddTimeIndex] = EEPROM_readDouble(FirstAddTimeAddress);
  SettingVariable[SecondAddTimeIndex] = EEPROM_readDouble(SecondAddTimeAddress);
  SettingVariable[ThirdAddTimeIndex] = EEPROM_readDouble(ThirdAddTimeAddress);
  //  }

  //Use defaults if EEPROM values are invalid
  if (isnan(SettingVariable[MTSTKpIndex])) {
    SettingVariable[MTSTKpIndex] = 100;
  }
  if (isnan(SettingVariable[MTSTKiIndex])) {
    SettingVariable[MTSTKiIndex] = 50;
  }
  if (isnan(SettingVariable[MTSTKdIndex])) {
    SettingVariable[MTSTKdIndex] = 10;
  }
  if (isnan(SettingVariable[MTKpIndex])) {
    SettingVariable[MTKpIndex] = 100;
  }
  if (isnan(SettingVariable[MTKiIndex])) {
    SettingVariable[MTKiIndex] = 50;
  }
  if (isnan(SettingVariable[MTKdIndex])) {
    SettingVariable[MTKdIndex] = 10;
  }
  if (isnan(SettingVariable[MTSpIndex])) {
    SettingVariable[MTSpIndex] = 150;
  }
  if (isnan(SettingVariable[MTWaterToGrainRatioIndex])) {
    SettingVariable[MTWaterToGrainRatioIndex] = 1.5;
  }
  if (isnan(SettingVariable[MTGrainTempIndex])) {
    SettingVariable[MTGrainTempIndex] = 70;
  }
  if (isnan(SettingVariable[MTInitialWaterTempIndex])) {
    SettingVariable[MTInitialWaterTempIndex] = 65;
  }
  if (isnan(SettingVariable[MTRampOffTempIndex])) {
    SettingVariable[MTRampOffTempIndex] = 10;
  }
  if (isnan(SettingVariable[MTPIDStartTempIndex])) {
    SettingVariable[MTPIDStartTempIndex] = 1;
  }
  if (isnan(SettingVariable[MashTimeIndex])) {
    SettingVariable[MashTimeIndex] = 10000;    //// UPDATE SEPTEMBER 19 2022
  }
  if (isnan(SettingVariable[BoilTimeIndex])) {
    SettingVariable[BoilTimeIndex] = 10000;
  }
  if (isnan(SettingVariable[FirstAddTimeIndex])) {
    SettingVariable[FirstAddTimeIndex] = 60000;
  }
  if (isnan(SettingVariable[SecondAddTimeIndex])) {
    SettingVariable[SecondAddTimeIndex] = 60000;
  }
  if (isnan(SettingVariable[ThirdAddTimeIndex])) {
    SettingVariable[ThirdAddTimeIndex] = 60000;
  }

}


void CheckBottomButtonPress () {
  if (Home_btn.justPressed()) {
    screen = 0;
    /*
    sys.MashTimerObj.add_time(sys.MashTimerObj.tLengthS -= SettingVariable[MashTimeIndex] );//-= EEPROM_readDouble(MashTimeAddress));
    sys.BoilTimerObj.add_time(sys.MashTimerObj.tLengthS -= SettingVariable[BoilTimeIndex] );//-= EEPROM_readDouble(BoilTimeAddress));
    sys.AddTimerObj1.add_time(sys.MashTimerObj.tLengthS -= SettingVariable[FirstAddTimeIndex] );//-= EEPROM_readDouble(FirstAddTimeAddress));
    sys.AddTimerObj2.add_time(sys.MashTimerObj.tLengthS -= SettingVariable[SecondAddTimeIndex] );//-= EEPROM_readDouble(SecondAddTimeAddress));
    sys.AddTimerObj3.add_time(sys.MashTimerObj.tLengthS -= SettingVariable[ThirdAddTimeIndex] );// -= EEPROM_readDouble(ThirdAddTimeAddress));
    */
    sys.MashTimerObj.tLengthS = SettingVariable[MashTimeIndex];
    sys.BoilTimerObj.tLengthS = SettingVariable[BoilTimeIndex];
    sys.AddTimerObj1.tLengthS = SettingVariable[FirstAddTimeIndex];
    sys.AddTimerObj2.tLengthS = SettingVariable[SecondAddTimeIndex];
    sys.AddTimerObj3.tLengthS = SettingVariable[ThirdAddTimeIndex];
    SaveParameters();
    
    draw_button_list(screen0);
    /*
   if (BoilActTime == 0 ) {
  //tone(BuzzerPin,250);
  digitalWrite(BKLEDPin,HIGH);
}
*/
  }

  if (Pid_btn.justPressed()) {
    screen = 1;
    draw_button_list(screen1);
  }

  return;
}

// Check whether the different "Select" buttons were pressed
void CheckSettingButtonPress () {
  if (SelMTSTKp_btn.justPressed())
    SettingIndex = MTSTKpIndex;

  if (SelMTSTKi_btn.justPressed())
    SettingIndex = MTSTKiIndex;

  if (SelMTSTKd_btn.justPressed())
    SettingIndex = MTSTKdIndex;

  if (SelMTKp_btn.justPressed())
    SettingIndex = MTKpIndex;

  if (SelMTKi_btn.justPressed())
    SettingIndex = MTKiIndex;

  if (SelMTKd_btn.justPressed())
    SettingIndex = MTKdIndex;

  if (SelMTSp_btn.justPressed())
    SettingIndex = MTSpIndex;

  if (SelMTWaterToGrainRatio_btn.justPressed())
    SettingIndex = MTWaterToGrainRatioIndex;

  if (SelMTGrainTemp_btn.justPressed())
    SettingIndex = MTGrainTempIndex;

  if (SelMTInitialWaterTemp_btn.justPressed())
    SettingIndex = MTInitialWaterTempIndex;

  if (SelMTRampOffTemp_btn.justPressed())
    SettingIndex = MTRampOffTempIndex;

  if (SelMTPIDStartTemp_btn.justPressed())
    SettingIndex = MTPIDStartTempIndex;

  if (SelMTTimer_btn.justPressed())
    SettingIndex = MashTimeIndex;

  if (SelBKTimer_btn.justPressed())
    SettingIndex = BoilTimeIndex;

  if (SelBKFirstAdd_btn.justPressed())
    SettingIndex = FirstAddTimeIndex;

  if (SelBKSecondAdd_btn.justPressed())
    SettingIndex = SecondAddTimeIndex;

  if (SelBKThirdAdd_btn.justPressed())
    SettingIndex = ThirdAddTimeIndex;

  return;
}

void CheckIncDecButtonPress () {
  if (Dec10_btn.justPressed() && SettingVariable[SettingIndex] > 9.9)
    SettingVariable[SettingIndex] -= 10.0;

  if (Dec1_btn.justPressed() && SettingVariable[SettingIndex] > .9)
    SettingVariable[SettingIndex] -= 1.0;

  if (Dectenth_btn.justPressed() && SettingVariable[SettingIndex] > 0)
    SettingVariable[SettingIndex] -= 0.1;

  if (Inctenth_btn.justPressed() && SettingVariable[SettingIndex] < 999.9)
    SettingVariable[SettingIndex] += 0.1;

  if (Inc1_btn.justPressed() && SettingVariable[SettingIndex] < 999)
    SettingVariable[SettingIndex] += 1.0;

  if (Inc10_btn.justPressed() && SettingVariable[SettingIndex] < 990)
    SettingVariable[SettingIndex] += 10.0;

  return;

}
// Cap Decrements so time can't be less than zero, cap increments so time can't exceed 10 hours
void CheckTimeIncDecButtonPress () {
  if (Dec10Min_btn.justPressed() && SettingVariable[SettingIndex] > 599999)
    SettingVariable[SettingIndex] -= 600000;

  if (Dec1Min_btn.justPressed() && SettingVariable[SettingIndex] > 59999)
    SettingVariable[SettingIndex] -= 60000;

  if (Dec10Sec_btn.justPressed() && SettingVariable[SettingIndex] > 9999)
    SettingVariable[SettingIndex] -= 10000;
  
  if (Dec1Sec_btn.justPressed() && SettingVariable[SettingIndex] > 999)
    SettingVariable[SettingIndex] -= 1000;

  if (Inc1Sec_btn.justPressed() && SettingVariable[SettingIndex] < 35999000)
    SettingVariable[SettingIndex] += 1000;
    
  if (Inc10Sec_btn.justPressed() && SettingVariable[SettingIndex] < 35990000)
    SettingVariable[SettingIndex] += 10000;

  if (Inc1Min_btn.justPressed() && SettingVariable[SettingIndex] < 35940000)
    SettingVariable[SettingIndex] += 60000;

  if (Inc10Min_btn.justPressed() && SettingVariable[SettingIndex] < 35400000)
    SettingVariable[SettingIndex] += 600000;

  return;

}
void MashEndSignal() { // will these delays affect the whole system? do we need another way of keeping time
unsigned long time_now=0;

 
 if(millis() > time_now + period) {
  time_now = millis();
  tone(43,250);
  MTActTime = SettingVariable[MashTimeIndex];
 }
else {noTone(43);}
 
  /*
  // 1/19/23  Boil Timer Logic
 if (BoilActTime == 0 ) {
  //tone(BuzzerPin,250);
  digitalWrite(BKLEDPin,HIGH);
}
  */
  
  /*pinMode(43,OUTPUT);   
  tone(43,250);
   delay(1000);
  noTone(43);
   delay(1000);
  tone(43,250);
   delay(1000);
  noTone(43);
   delay(1000);
  tone(43,250);
   delay(1000);
  noTone(43);
   delay(1000);
  tone(43,250);
   delay(1000);
  noTone(43);
   delay(1000);
   */
 
}
