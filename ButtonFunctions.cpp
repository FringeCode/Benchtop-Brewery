#include "ButtonFunctions.hpp"
#include "Variables.hpp"
#include <Adafruit_GFX.h>
#include <TouchScreen.h>

bool Touch_getXY(TouchScreen& ts, MCUFRIEND_kbv& tft) {

  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);
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

bool update_button(Adafruit_GFX_Button* b, bool down) {

  b->press(down && b->contains(pixel_x, pixel_y));
  if (b->justReleased())
    b->drawButton(false);
  if (b->justPressed())
    b->drawButton(true);
  return down;

}

bool update_button_list(Adafruit_GFX_Button* btn_list[], size_t list_length) {

  bool down = Touch_getXY(ts, tft);
  for (size_t i = 0 ; i < list_length; ++i)
    update_button(btn_list[i], down);

  return down;

}

void draw_button_list(Adafruit_GFX_Button* btn_list[], size_t list_length) {

  tft.fillScreen(TFT_LIGHTGREY);
  for (size_t i = 0; i < list_length; ++i)
    btn_list[i]->drawButton(false);

}

void CheckBottomButtonPress () {

  if (Home_btn.justPressed()) {
    DrawHomeScreenStaticText_flag = true;
    screen = 0;

    sys.MashTimerObj.tLengthS = SettingVariable[MashTimeIndex];
    sys.BoilTimerObj.tLengthS = SettingVariable[BoilTimeIndex];
    sys.AddTimerObj1.tLengthS = SettingVariable[FirstAddTimeIndex];
    sys.AddTimerObj2.tLengthS = SettingVariable[SecondAddTimeIndex];
    sys.AddTimerObj3.tLengthS = SettingVariable[ThirdAddTimeIndex];

    SaveParameters();
    draw_button_list(screen0, screen0_length);
  }

  if (Pid_btn.justPressed()) {
    DrawSettingsScreenStaticText_flag = true;
    screen = 1;
    draw_button_list(screen1, screen1_length);
  }

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

}