// screendraw.h
// Dodge Temperature Controller / PID Libary
// Screen Display Functions here
// March 22 2022

// If we only included function declarations,
// would I have to have a .cpp file with definitions? or a .ino file?
// Made version number a variable called Version, located in .ino file
// Added Cancel buttons for timers on Home Screen, Sel buttons for 
// Timers on Settings Screen 4/30/2022
// Removed Hot Liquor Tank PID Values, sorted Settings Screen, added 
// Boil Time Start button to Home Screen
// Adding real time display for Additional Boil Timers
// 2/23/2023 Move Mash Tun Status to middle of screen. Make Status
// display on one line. Move Boil timers to bottom.
#ifndef SCREEN_DRAW_H
#define SCREEN_DRAW_H

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

#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

inline constexpr char* Version = "V12X";

long unsigned int g_BoilActTime{};
long unsigned int g_FirstAddActTime{};
long unsigned int g_SecondAddActTime{};
long unsigned int g_ThirdAddActTime{};
inline long unsigned int g_MTActTime{};

namespace Tones {
  constexpr long unsigned int MTTone = 250;
  constexpr long unsigned int BKTone = 250;
  constexpr long unsigned int FirstAddTone = 250;
  constexpr long unsigned int SecondAddTone = 250;
  constexpr long unsigned int ThirdAddTone = 280;
}
long unsigned int g_BuzzTone = Tones::MTTone;

inline constexpr int g_period = 1000;
inline MCUFRIEND_kbv g_tft{};

// copy-paste results from TouchScreen_Calibr_native.ino
constexpr int XP = 8, XM = A2, YP = A3, YM = 9; //320x480 ID=0x9486
constexpr int TS_LEFT = 116, TS_RT = 905, TS_TOP = 936, TS_BOT = 93;
inline auto ts = TouchScreen(XP, YP, XM, YM, 300);

// UPDATE 5/13/22
// Function accepts time in milliseconds, outputs string? "HH:MM:SS"
// Or output 3-item array of double H, M, S
String GetClockTime ( double ms ) {
  String time{};
  int hours = ms / 3600000;
  int minutes = ( ms - ( hours * 3600000 ) ) / 60000;
  int seconds = ( ms - ( hours * 3600000 + minutes * 60000 ) ) / 1000;
  
  return time + hours + ":" + minutes + ":" + seconds;  
}

enum class stateList { OFF = 0, RAMPUP, RAMPOFF, STRIKEPID, MASHPID };

extern stateList globalState = stateList::OFF;
extern double MTStrikeTemp;

// SEPTEMBER 19 2022
// timer class encapsulating its own state?
template<typename T>
struct LoopingTimerStateList {
  // ?
};

// using typename lint = long unsigned int;
typedef long unsigned int lint;

struct Timer {
  // tTimer ( lint l , lint s , int i = 0 ) : tLengthS(l) , tStartMS(s), tStateNow(i) {}
  lint tLengthS; // seconds? ms?
  lint tStartMS , tCurrentMS; //MS: milliseconds , CU: COUNTING UP
  lint tPauseStart; //-1 means UNPAUSED, RUNNING

  lint tCurrentCU () { return millis() - tPauseLength() - tStartMS; }
  // int tState {beginningPaused = 0, beginningRunning = 1, middleRunning = 2, middlePaused = 3, endRunning = 4, endPaused = 5};
  // six states
  int tStates[6] = { 0 , 1 , 2 , 3 , 4 , 5 };
  int tStateNow = 0;

  // UPDATE SEPT 20 2022
  bool doneNotReset = false;

  bool reset_timer () {
    tStateNow = 0;
    doneNotReset = false;
  }

  bool add_time ( lint t ) {
    tLengthS += t;
  }
  
  lint tPauseLength () {
    if ( tStateNow == 3 )
      return millis() - tPauseStart;
    else
      return 0;
  }

  lint pause () {
    // tPauseStart = millis();
    if ( tStateNow == 2 || tStateNow == 1 )
      tStateNow = 3;
    if ( tStateNow == 5 )
      tStateNow = 0;
    return tPauseStart = millis(); // assignments
  }

  lint get_time_count_down () {
    switch(tStateNow) {
      case 0:
        doneNotReset = false;
        return tLengthS;
        break;
      case 1:
        tStateNow = 2; // middleRunning
        doneNotReset = false;
        return tLengthS - tCurrentCU(); //? is state 1 needed? beginningRunning
        break;
      case 2:
        if ( tLengthS <= tCurrentCU() ) {
          tStateNow = 5;
          return 0;
        }
        return tLengthS - tCurrentCU();
        break;
      case 3:
        return tLengthS - tCurrentCU();
        break;
      case 4:
        tStateNow = 5;
        doneNotReset = true;
        return tCurrentCU();
        break;
      case 5:
        doneNotReset = true;
        return 0;
        break;
      
    }
    return tCurrentCU();
  }

  lint start_from_beginning () {
    // tCurrentCU = 0;
    doneNotReset = false;
    
    tStateNow = 1;
    tStartMS = millis();
    tCurrentMS = millis();
    tPauseStart = -1;
  }
};

// UPDATE 4/14/22
struct StateSystem {

    stateList getState () const { // does it matter that the first const is there?
      return opState;             // 3/6/22 removed initial const in case that affects mutability of returned stateList
    }
    void doState();
    
    // SEPT 19 2022
    Timer MashTimerObj;
    Timer BoilTimerObj;
    Timer AddTimerObj1, AddTimerObj2, AddTimerObj3;
    
    stateList checkState();
  private:
    // stateList opState = stateList::OFF; // initial value
    // 3/13/22 Setting opState to be a reference to globalState - hopefully this doesn't break anything
    stateList& opState = globalState;

    void Off();
    void RampUp();
    void RampOff();
    void StrikePID();
    void MashPID();

    const int MASHTimer = 10000; // 1 minute in milliseconds
    int MASHStartTime = -1; // Set to current time when beginning MASHPID, set to -1 when out of state
    double MaxRampOffTemp = 0; // JULY 17 2022 keep track of max temp and dips during Ramp Off
};



/*********************
  UPDATE 3/28
  Took color #define statements from adafruit pdf page 6 of 26
**********************/
#define TFT_BLACK 0x0000
#define TFT_LIGHTGREY 0x5555
#define TFT_BLUE 0x001F
#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0
#define TFT_DARKGREEN 0x0320
#define TFT_CYAN 0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW 0xFFE0
#define TFT_WHITE 0xFFFF
#define TFT_ORANGE 0xFD20
#define TFT_GREENYELLOW 0xAFE5

// V12i 4/5/22 SettingVariable Indices
#define MTSTKpIndex 0
#define MTSTKiIndex 1
#define MTSTKdIndex 2

#define MTKpIndex 3
#define MTKiIndex 4
#define MTKdIndex 5

#define MTSpIndex 6 //MT Temperature Set Point;
#define MTWaterToGrainRatioIndex 7
#define MTGrainTempIndex 8
#define MTInitialWaterTempIndex 9
#define MTRampOffTempIndex 10
#define MTPIDStartTempIndex 11

#define MashTimeIndex	12
#define BoilTimeIndex	13
#define FirstAddTimeIndex	14
#define SecondAddTimeIndex	15
#define ThirdAddTimeIndex	16
// double g_MTActTime;
// JUNE 22 2022
// CONFLICTING DECLARATION with long unsigned int g_MTActTime

// These declarations can be removed
// FUNCTION DECLARATIONS, DEFINITIONS ARE BELOW LOOP FUNCTION
// void DrawHomeScreen ( MCUFRIEND_kbv& , double* , StateSystem& , double& , double& ); // MCUFRIEND_kbv & );
// void DrawSettingsScreen ( MCUFRIEND_kbv& , double* , StateSystem& , double& , double& ); // MCUFRIEND_kbv & );


// Draw the Home Screen (screen == 0)
void DrawHomeScreen ( MCUFRIEND_kbv& g_tft , double* SettingVariable , StateSystem& sys , double& Input , double& Output ) { // MCUFRIEND_kbv &g_tft ) {
  g_tft.setRotation(0); //setRotation(uint8_t r) This code rotates the screen. 0=0 , 1=90, 2=180, 3=270
  g_tft.setCursor(100, 10); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1.75); //setTextSize(uint8_t s) =1-5
  g_tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY); //setTextColor(uint16_t t, uint16_t b) t=text color, b=background color
  g_tft.print("DODGE BREWING SYSTEMS        ");
  g_tft.print(Version);
  g_tft.drawFastHLine(0, 30, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)
  g_tft.setCursor(130, 40); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("HOME");
  g_tft.drawFastHLine(0, 60, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)
  g_tft.setCursor(10, 70); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("Mash Tun State:");
  g_tft.setCursor(10, 103); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("Mash Tun Step:");
  //g_tft.setCursor(10, 133); //setCursor(int16_t x, int16_t y)
  //g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  //g_tft.print("MT Mixer (On/Off):");
  //g_tft.setCursor(10, 163); //setCursor(int16_t x, int16_t y)
  //g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  //g_tft.print("MT Pump (On/Off):");
  g_tft.setCursor(10, 133); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("MT Set Point:");
  g_tft.print(SettingVariable[MTSpIndex], 1);
  g_tft.setCursor(10, 163); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("MT Actual Temp:");
  g_tft.print(Input, 1);
  g_tft.setCursor(10, 193); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("MT Strike Temp:");
  g_tft.print(MTStrikeTemp, 1);
// Text for Timer Buttons on Home Screen
  g_tft.setCursor(10, 223); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("Mash Timer:");
  //g_tft.print(g_MTActTime, 1);

 
// Print Hours

// UPDATED WITH MASHTIMEROBJ SEPT 19 2022
  g_MTActTime = sys.MashTimerObj.get_time_count_down();
  g_tft.print(GetClockTime(g_MTActTime));

  g_tft.setCursor(10, 253); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("STATUS:");
  g_tft.drawFastHLine(0, 245, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)
  g_tft.drawFastHLine(0, 275, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)

  if ( sys.getState() == stateList::MASHPID ) {
    g_tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    g_tft.setCursor(100, 253); //setCursor(int16_t x, int16_t y)
    g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    g_tft.print("MASH PID STEP  ");
  } else if ( sys.getState() == stateList::STRIKEPID ) {
    g_tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    g_tft.setCursor(100, 253); //setCursor(int16_t x, int16_t y)
    g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    g_tft.print("STRIKE PID STEP");
  } else if ( sys.getState() == stateList::RAMPUP ) {
    g_tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    g_tft.setCursor(100, 253); //setCursor(int16_t x, int16_t y)
    g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    g_tft.print("RAMP UP STEP   ");
  } else if ( sys.getState() == stateList::RAMPOFF ) {
    g_tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    g_tft.setCursor(100, 253); //setCursor(int16_t x, int16_t y)
    g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    g_tft.print("RAMP OFF STEP");
  } else {
    g_tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    g_tft.setCursor(100, 253); //setCursor(int16_t x, int16_t y)
    g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
    g_tft.print("OFF            ");
  }


  g_tft.setCursor(10, 313); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("Boil Timer:");
  g_BoilActTime = sys.BoilTimerObj.get_time_count_down();
  g_tft.print(GetClockTime(g_BoilActTime));

  // 1st Add. Timer Real Time Display
  g_FirstAddActTime = sys.AddTimerObj1.get_time_count_down();
  g_tft.setCursor(10, 343); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("1st Addition:");
  g_tft.print(GetClockTime(g_FirstAddActTime));

// 2nd Add. Timer Real Time Display
  g_SecondAddActTime = sys.AddTimerObj2.get_time_count_down();
  g_tft.setCursor(10, 373); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("2nd Addition:");
  g_tft.print(GetClockTime(g_SecondAddActTime));

// 3rd Add. Timer Real Time Display
g_ThirdAddActTime = sys.AddTimerObj3.get_time_count_down();
  g_tft.setCursor(10, 403); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("3rd Addition:");
  g_tft.print(GetClockTime(g_ThirdAddActTime));
  return;
}

// Draw the Settings Screen (screen == 1)
void DrawSettingsScreen ( MCUFRIEND_kbv& g_tft , double* SettingVariable , StateSystem& sys , double& Input , double& Output ) { // MCUFRIEND_kbv &g_tft ) {
  g_tft.setRotation(0); //setRotation(uint8_t r) This code rotates the screen. 0=0 , 1=90, 2=180, 3=270
  g_tft.setCursor(100, 10); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1.75); //setTextSize(uint8_t s) =1-5
  g_tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY); //setTextColor(uint16_t t, uint16_t b) t=text color, b=background color
  g_tft.print("DODGE BREWING SYSTEMS        ");
  g_tft.print(Version);
	
  g_tft.drawFastHLine(0, 30, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)
  g_tft.setCursor(90, 40); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("PID SETTINGS");
  g_tft.drawFastHLine(0, 60, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)
  
  g_tft.setCursor(15, 70); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("STRIKE TEMP");
  g_tft.setCursor(90, 70); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("P:");
  g_tft.setCursor(105, 70); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTSTKpIndex], 1);
  g_tft.setCursor(180, 70); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("I:");
  g_tft.setCursor(195, 70); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTSTKiIndex], 1);
  g_tft.setCursor(250, 70); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("D:");
  g_tft.setCursor(265, 70); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTSTKdIndex], 1);

  g_tft.setCursor(15, 90); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("MASH TEMP");
  g_tft.setCursor(90, 90); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("P:");
  g_tft.setCursor(105, 90); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTKpIndex], 1);
  g_tft.setCursor(180, 90); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("I:");
  g_tft.setCursor(195, 90); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTKiIndex], 1);
  g_tft.setCursor(250, 90); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("D:");
  g_tft.setCursor(265, 90); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTKdIndex], 1);
  g_tft.drawFastHLine(0, 105, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)
  g_tft.setCursor(80, 115); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("MASH TUN SETTINGS");
  g_tft.drawFastHLine(0, 135, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)

  //g_tft.setCursor(15, 190); //setCursor(int16_t x, int16_t y)
  //g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  //g_tft.print("MASH RECIPE SETTINGS");
  g_tft.setCursor(15, 142); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("MASH TEMP SETPOINT:");
  g_tft.setCursor(170, 142); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTSpIndex], 1);
  g_tft.setCursor(15, 162); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("WATER TO GRAIN RATIO:");
  g_tft.setCursor(170, 162); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTWaterToGrainRatioIndex], 1);
  g_tft.setCursor(15, 182); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("INITIAL GRAIN TEMP:");
  g_tft.setCursor(170, 182); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTGrainTempIndex], 1);
  g_tft.setCursor(15, 202); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("INITIAL WATER TEMP:");
  g_tft.setCursor(170, 202); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTInitialWaterTempIndex], 1);
  //g_tft.setCursor(15, 290); //setCursor(int16_t x, int16_t y)
  //g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  //g_tft.print("MASH TIME:");
  //g_tft.setCursor(170, 290); //setCursor(int16_t x, int16_t y)
  //g_tft.print(MTMashTime,1);
  g_tft.setCursor(15, 222); //setCursor(int16_t x, int16_t y)
  g_tft.print("MASH TUN RAMP OFF TEMP:");
  g_tft.setCursor(170, 222); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTRampOffTempIndex], 1);
  g_tft.setCursor(15, 242); //setCursor(int16_t x, int16_t y)
  g_tft.print("MASH TUN PID START TEMP:");
  g_tft.setCursor(170, 242); //setCursor(int16_t x, int16_t y)
  g_tft.print(SettingVariable[MTPIDStartTempIndex], 1);
  g_tft.drawFastHLine(0, 283, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)
  g_tft.setCursor(80, 293); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(2); //setTextSize(uint8_t s) =1-5
  g_tft.print("TIMER SETTINGS");
  g_tft.drawFastHLine(0, 313, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)
  g_tft.setCursor(15, 327); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("MASH TIME:");
  g_tft.setCursor(75, 327); //setCursor(int16_t x, int16_t y)
  //g_tft.print(SettingVariable[MashTimeIndex], 1);
// Print Hours
if (SettingVariable[MashTimeIndex]>3599000) {
g_tft.print(round(trunc(SettingVariable[MashTimeIndex]/3600000)*3600000)/3600000);
g_tft.print(":");}
else {
g_tft.print("0:");}

//Print Minutes
if((SettingVariable[MashTimeIndex]>3599000) &&((SettingVariable[MashTimeIndex]-(trunc(SettingVariable[MashTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[MashTimeIndex]-(round(trunc( SettingVariable[MashTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":");}
else if((SettingVariable[MashTimeIndex]>3599000) &&((SettingVariable[MashTimeIndex]-(trunc(SettingVariable[MashTimeIndex]/3600000)*3600000))<600000)) {
g_tft.print("0");
g_tft.print(round(SettingVariable[MashTimeIndex]-(round(trunc( SettingVariable[MashTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":"); }
 else if ((SettingVariable[MashTimeIndex]<3600000) && ((SettingVariable[MashTimeIndex]-(trunc(SettingVariable[MashTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[MashTimeIndex]-(round(trunc( SettingVariable[MashTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }
 else {
 g_tft.print("0");
g_tft.print(round(SettingVariable[MashTimeIndex]-(round(trunc( SettingVariable[MashTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }

// Print Seconds
if((SettingVariable[MashTimeIndex]-(trunc(SettingVariable[MashTimeIndex]/60000)*60000))>9000) {
g_tft.print(round(SettingVariable[MashTimeIndex]-
(trunc(SettingVariable[MashTimeIndex]/60000)*60000))/1000); }
else if ((SettingVariable[MashTimeIndex]-(trunc(SettingVariable[MashTimeIndex]/60000)*60000))<10000) {
g_tft.print("0");
g_tft.print(round(SettingVariable[MashTimeIndex]-
(trunc(SettingVariable[MashTimeIndex]/60000)*60000))/1000); }

  //g_tft.setCursor(170, 330); //setCursor(int16_t x, int16_t y)
  //g_tft.print(MTMashTime,1);
  g_tft.setCursor(170, 327); //setCursor(int16_t x, int16_t y)
  g_tft.setTextSize(1); //setTextSize(uint8_t s) =1-5
  g_tft.print("BOIL TIME:");
g_tft.setCursor(230, 327); //setCursor(int16_t x, int16_t y)
/*
// Print Boil Time Hours
 if(SettingVariable[BoilTimeIndex]>3599000) {
  g_tft.print(round(SettingVariable[BoilTimeIndex]/3600000));
// g_tft.print( GetClockTime( SettingVariable[BoilTimeIndex])[0] );
 g_tft.print(":");}
 else
 {g_tft.print("0:");}

// Print Boil Time Minutes
 if(SettingVariable[BoilTimeIndex]>3599000) 
{
  g_tft.print(round(SettingVariable[BoilTimeIndex]-(round(trunc( SettingVariable[BoilTimeIndex]/3600000)*3600000)))/60000);
 // g_tft.print( GetClockTime( SettingVariable[BoilTimeIndex])[1] );
 g_tft.print(":");
} 
else if (SettingVariable[BoilTimeIndex]>59000) {
g_tft.print(round(trunc(SettingVariable[BoilTimeIndex]/60000)*60000)/60000);
g_tft.print(":");

 } 
 else { g_tft.print("00:"); }

// 5/13/22 CHANGING THIS TO USE GETCLOCKTIME FUNCTION
//Print Boil Time Seconds

 if(SettingVariable[BoilTimeIndex]>3599000) {
 g_tft.print(round(SettingVariable[BoilTimeIndex]
-((round(round(SettingVariable[BoilTimeIndex]/3600000)*3600000))
+(round(round(SettingVariable[BoilTimeIndex]/60000)*60000)))
/1000));

  // g_tft.print( GetClockTime( SettingVariable[BoilTimeIndex])[2] );
 } else if(SettingVariable[BoilTimeIndex]>59000) {
   g_tft.print(round(SettingVariable[BoilTimeIndex]-(round(SettingVariable[BoilTimeIndex]/60000)*60000))/1000);
 } else {
 g_tft.print(round(SettingVariable[BoilTimeIndex]/1000));
// g_tft.print( GetClockTime( SettingVariable[BoilTimeIndex])[0] );
 }

if(SettingVariable[BoilTimeIndex]>59000) {
g_tft.print(round(SettingVariable[BoilTimeIndex]-
(trunc(SettingVariable[BoilTimeIndex]/60000)*60000))/1000); }
else {

g_tft.print(round(SettingVariable[BoilTimeIndex]/1000));}
*/

// Print Hours
if (SettingVariable[BoilTimeIndex]>3599000) {
g_tft.print(round(trunc(SettingVariable[BoilTimeIndex]/3600000)*3600000)/3600000);
g_tft.print(":");}
else {
g_tft.print("0:");}

//Print Minutes
if((SettingVariable[BoilTimeIndex]>3599000) &&((SettingVariable[BoilTimeIndex]-(trunc(SettingVariable[BoilTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[BoilTimeIndex]-(round(trunc( SettingVariable[BoilTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":");}
else if((SettingVariable[BoilTimeIndex]>3599000) &&((SettingVariable[BoilTimeIndex]-(trunc(SettingVariable[BoilTimeIndex]/3600000)*3600000))<600000)) {
g_tft.print("0");
g_tft.print(round(SettingVariable[BoilTimeIndex]-(round(trunc( SettingVariable[BoilTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":"); }
 else if ((SettingVariable[BoilTimeIndex]<3600000) && ((SettingVariable[BoilTimeIndex]-(trunc(SettingVariable[BoilTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[BoilTimeIndex]-(round(trunc( SettingVariable[BoilTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }
 else {
 g_tft.print("0");
g_tft.print(round(SettingVariable[BoilTimeIndex]-(round(trunc( SettingVariable[BoilTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }

// Print Seconds
if((SettingVariable[BoilTimeIndex]-(trunc(SettingVariable[BoilTimeIndex]/60000)*60000))>9999) {
g_tft.print(round(SettingVariable[BoilTimeIndex]-
(trunc(SettingVariable[BoilTimeIndex]/60000)*60000))/1000); }
else if ((SettingVariable[BoilTimeIndex]-(trunc(SettingVariable[BoilTimeIndex]/60000)*60000))<10000) {
g_tft.print("0");
g_tft.print(round(SettingVariable[BoilTimeIndex]-
(trunc(SettingVariable[BoilTimeIndex]/60000)*60000))/1000); }
/*
if (SettingVariable[BoilTimeIndex]>9000) {
g_tft.print(round(SettingVariable[BoilTimeIndex]/1000)); }
else {
g_tft.print("0");
g_tft.print(round(SettingVariable[BoilTimeIndex]/1000)); }
*/
// code may not work completely as expected since not all parts of the nested if statements use the GetClockTime function, only certain conditions
// this can be changed easily though, since the function should not need any if-else statements to display the appropriate time


  //g_tft.setCursor(170, 380); //setCursor(int16_t x, int16_t y)
  //g_tft.print(MTPumpSpeed, 1);
  g_tft.setCursor(170, 347); //setCursor(int16_t x, int16_t y)
  g_tft.print("1st Add:");
g_tft.setCursor(230, 347); //setCursor(int16_t x, int16_t y)
 // g_tft.print(SettingVariable[FirstAddTimeIndex], 1);
// Print Hours
if (SettingVariable[FirstAddTimeIndex]>3599000) {
g_tft.print(round(trunc(SettingVariable[FirstAddTimeIndex]/3600000)*3600000)/3600000);
g_tft.print(":");}
else {
g_tft.print("0:");}

//Print Minutes
if((SettingVariable[FirstAddTimeIndex]>3599000) &&((SettingVariable[FirstAddTimeIndex]-(trunc(SettingVariable[FirstAddTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[FirstAddTimeIndex]-(round(trunc( SettingVariable[FirstAddTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":");}
else if((SettingVariable[FirstAddTimeIndex]>3599000) &&((SettingVariable[FirstAddTimeIndex]-(trunc(SettingVariable[FirstAddTimeIndex]/3600000)*3600000))<600000)) {
g_tft.print("0");
g_tft.print(round(SettingVariable[FirstAddTimeIndex]-(round(trunc( SettingVariable[FirstAddTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":"); }
 else if ((SettingVariable[FirstAddTimeIndex]<3600000) && ((SettingVariable[FirstAddTimeIndex]-(trunc(SettingVariable[FirstAddTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[FirstAddTimeIndex]-(round(trunc( SettingVariable[FirstAddTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }
 else {
 g_tft.print("0");
g_tft.print(round(SettingVariable[FirstAddTimeIndex]-(round(trunc( SettingVariable[FirstAddTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }

// Print Seconds
if((SettingVariable[FirstAddTimeIndex]-(trunc(SettingVariable[FirstAddTimeIndex]/60000)*60000))>9000) {
g_tft.print(round(SettingVariable[FirstAddTimeIndex]-
(trunc(SettingVariable[FirstAddTimeIndex]/60000)*60000))/1000); }
else if ((SettingVariable[FirstAddTimeIndex]-(trunc(SettingVariable[FirstAddTimeIndex]/60000)*60000))<10000) {
g_tft.print("0");
g_tft.print(round(SettingVariable[FirstAddTimeIndex]-
(trunc(SettingVariable[FirstAddTimeIndex]/60000)*60000))/1000); }

  //g_tft.setCursor(170, 400); //setCursor(int16_t x, int16_t y)
  //g_tft.print(MTMixerSpeed, 1);
  g_tft.setCursor(170, 367); //setCursor(int16_t x, int16_t y)
  g_tft.print("2nd Add:");
g_tft.setCursor(230, 367); //setCursor(int16_t x, int16_t y)
 // g_tft.print(SettingVariable[SecondAddTimeIndex], 1);
// Print Hours
if (SettingVariable[SecondAddTimeIndex]>3599000) {
g_tft.print(round(trunc(SettingVariable[SecondAddTimeIndex]/3600000)*3600000)/3600000);
g_tft.print(":");}
else {
g_tft.print("0:");}

//Print Minutes
if((SettingVariable[SecondAddTimeIndex]>3599000) &&((SettingVariable[SecondAddTimeIndex]-(trunc(SettingVariable[SecondAddTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[SecondAddTimeIndex]-(round(trunc( SettingVariable[SecondAddTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":");}
else if((SettingVariable[SecondAddTimeIndex]>3599000) &&((SettingVariable[SecondAddTimeIndex]-(trunc(SettingVariable[SecondAddTimeIndex]/3600000)*3600000))<600000)) {
g_tft.print("0");
g_tft.print(round(SettingVariable[SecondAddTimeIndex]-(round(trunc( SettingVariable[SecondAddTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":"); }
 else if ((SettingVariable[SecondAddTimeIndex]<3600000) && ((SettingVariable[SecondAddTimeIndex]-(trunc(SettingVariable[SecondAddTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[SecondAddTimeIndex]-(round(trunc( SettingVariable[SecondAddTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }
 else {
 g_tft.print("0");
g_tft.print(round(SettingVariable[SecondAddTimeIndex]-(round(trunc( SettingVariable[SecondAddTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }

// Print Seconds
if((SettingVariable[SecondAddTimeIndex]-(trunc(SettingVariable[SecondAddTimeIndex]/60000)*60000))>9000) {
g_tft.print(round(SettingVariable[SecondAddTimeIndex]-
(trunc(SettingVariable[SecondAddTimeIndex]/60000)*60000))/1000); }
else if ((SettingVariable[SecondAddTimeIndex]-(trunc(SettingVariable[SecondAddTimeIndex]/60000)*60000))<10000) {
g_tft.print("0");
g_tft.print(round(SettingVariable[SecondAddTimeIndex]-
(trunc(SettingVariable[SecondAddTimeIndex]/60000)*60000))/1000); }
  g_tft.setCursor(170, 387); //setCursor(int16_t x, int16_t y)
  g_tft.print("3rd Add:");
g_tft.setCursor(230, 387); //setCursor(int16_t x, int16_t y)
  //g_tft.print(SettingVariable[ThirdAddTimeIndex], 1);
// Print Hours
if (SettingVariable[ThirdAddTimeIndex]>3599000) {
g_tft.print(round(trunc(SettingVariable[ThirdAddTimeIndex]/3600000)*3600000)/3600000);
g_tft.print(":");}
else {
g_tft.print("0:");}

//Print Minutes
if((SettingVariable[ThirdAddTimeIndex]>3599000) &&((SettingVariable[ThirdAddTimeIndex]-(trunc(SettingVariable[ThirdAddTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[ThirdAddTimeIndex]-(round(trunc( SettingVariable[ThirdAddTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":");}
else if((SettingVariable[ThirdAddTimeIndex]>3599000) &&((SettingVariable[ThirdAddTimeIndex]-(trunc(SettingVariable[ThirdAddTimeIndex]/3600000)*3600000))<600000)) {
g_tft.print("0");
g_tft.print(round(SettingVariable[ThirdAddTimeIndex]-(round(trunc( SettingVariable[ThirdAddTimeIndex]/3600000)*3600000)))/60000);
g_tft.print(":"); }
 else if ((SettingVariable[ThirdAddTimeIndex]<3600000) && ((SettingVariable[ThirdAddTimeIndex]-(trunc(SettingVariable[ThirdAddTimeIndex]/3600000)*3600000))>599000)) {
g_tft.print(round(SettingVariable[ThirdAddTimeIndex]-(round(trunc( SettingVariable[ThirdAddTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }
 else {
 g_tft.print("0");
g_tft.print(round(SettingVariable[ThirdAddTimeIndex]-(round(trunc( SettingVariable[ThirdAddTimeIndex]/3600000)*3600000)))/60000);
 g_tft.print(":"); }

// Print Seconds
if((SettingVariable[ThirdAddTimeIndex]-(trunc(SettingVariable[ThirdAddTimeIndex]/60000)*60000))>9000) {
g_tft.print(round(SettingVariable[ThirdAddTimeIndex]-
(trunc(SettingVariable[ThirdAddTimeIndex]/60000)*60000))/1000); }
else if ((SettingVariable[ThirdAddTimeIndex]-(trunc(SettingVariable[ThirdAddTimeIndex]/60000)*60000))<10000) {
g_tft.print("0");
g_tft.print(round(SettingVariable[ThirdAddTimeIndex]-
(trunc(SettingVariable[ThirdAddTimeIndex]/60000)*60000))/1000); }
  g_tft.drawFastHLine(0, 440, 320, TFT_BLACK); //drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t t)


  return;
}

#endif
