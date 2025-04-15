#include "mydebug.h"

#ifndef KEYPAD_H
#define KEYPAD_H

#define BUTTON_DEBOUNCE_TIME  (unsigned long)35
#define BUTTON_POLL_DELAY     (unsigned long)8

#define PICO_BUTTON_ERROR_BLINK_CYCLE_COUNT 14
#endif

//routine
void keypad_updateButtonReading();

//external function
keypad_reading keypad_currentReading();
bool keypad_isShiftOn();
bool keypad_isLongHold();
bool keypad_isThereButtonActivity();

//special function
//void keypad_bootCheck();
