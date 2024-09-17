#pragma once

#include <Adafruit_GFX.h>

void CheckBottomButtonPress ( void );
void CheckSettingButtonPress ( void );
void CheckIncDecButtonPress ( void );

void CheckPauseButtonPress (void);
void CheckRunButtonPress (void);
void CheckStrikeButtonPress (void);
void CheckMashButtonPress (void);

void CheckMTTimerCancelButtonPress (void);
void CheckBKTimerCancelButtonPress (void);
void CheckFirstAddCancelButtonPress (void);
void CheckSecondAddCancelButtonPress (void);
void CheckThirdAddCancelButtonPress (void);
void CheckTimeIncDecButtonPress (void);

bool update_button(Adafruit_GFX_Button*, bool);
bool update_button_list(Adafruit_GFX_Button**, size_t);
void draw_button_list(Adafruit_GFX_Button**, size_t);
