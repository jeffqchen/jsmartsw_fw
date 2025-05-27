#include "mydebug.h"

#include <Arduino.h>

#include "gsw_hardware.h"

#ifndef MYPICO_H
#define MYPICO_H

#define PICO_LED_DELAY_AUTO   (unsigned long)1000
#define PICO_LED_DELAY_MANUAL (unsigned long)250
#define PICO_LED_DELAY_FAULTY (unsigned long)125
#define PICO_LED_DELAY_SETUP  (unsigned long)125

#define BUTTON_SHORT_HOLD_THRESHOLD  (unsigned long)250
#define BUTTON_LONG_HOLD_THRESHOLD (unsigned long)2000
#define BUTTON_ABNORMAL_HOLD_THRESHOLD (unsigned long)30000

#define SETUPMODE_AUTO_QUIT_WAIT  10000

//pico & gsw input selection mode
#define GSW_MODE_AUTO     true
#define GSW_MODE_MANUAL   false

#define PICO_WATCH_DOG_TIMEOUT 2000

#define PICO_SAVEDATA_ADDR  0x3FC00 // end of 2MB minus 4KB
#define PICO_SAVEDATA_MAGICWORD   "JSMARTSW"
#define PICO_SAVEDATA_VERSION     "20250413"

//jSmartSW Running Modes
enum
{
  JSMARTSW_BOOT_SEQUENCE = 0,
  JSMARTSW_POST_BOOT,
  JSMARTSW_NORMAL_MODE,
  JSMARTSW_ENTERING_SETUP_MODE,
  JSMARTSW_SETUP_MODE,
  JSMARTSW_EXITING_SETUP_MODE
};

//pico power state
enum
{
  PICO_POWER_STATE_ASLEEP = 0,
  PICO_POWER_STATE_DOZE,
  PICO_POWER_STATE_AWAKE
};

// input icon customization data
typedef struct {
  unsigned int inputSize;
  unsigned int inputIconCategory;
  unsigned int inputIconId;
} input_number_spec;

//savedata data structure
typedef struct {
  char magicWord[8];  //magic word
  char version[8];  //version
  input_number_spec inputIconCustomizationData[INPUT_END];
  unsigned int rgbled_colorScheme;
  unsigned int rgbled_brightnessIndex;
  unsigned int screen_rotation;
} jsmartsw_savedata;

//button reading
typedef uint8_t keypad_reading;

typedef struct _keypad_data{
  keypad_reading reading;
  int buttonState;
  bool shiftOn;

  _keypad_data()
  {
    reading = KEYPAD_VALUE_READING_NONE;
    buttonState = BUTTON_UP;
    shiftOn = false;
  }
} keypad_data;

#define keypad_data()    _keypad_data()

#define CONTROLLER_REPEAT_WAIT      400
#define CONTROLLER_REPEAT_INTERVAL  40
#define CONTROLLER_TICK_EVERY_TIMES 10

#endif

////////////////////////////////////////
// Main Functions
////////////////////////////////////////

// Setup for different cores
void jsmartsw_core1Setup();
void jsmartsw_core2Setup();

// Main loop for different cores
void jsmartsw_core1Loop();
void jsmartsw_core2Loop();

////////////////////////////////////////
// jSmartSW Input Modes & Logic
////////////////////////////////////////

void pico_setMainProgramMode(int);
int pico_getMainProgramMode();

unsigned int pico_getGswMode();
unsigned int pico_getCurrentInput();
void pico_setNewInput(unsigned int);

////////////////////////////////////////
// Control interface functions
////////////////////////////////////////

bool pico_isKeypadShiftOn();

bool pico_isThereInputActivity();
int pico_currentControlDevice();

////////////////////////////////////////
// Setup Mode Logic
////////////////////////////////////////

unsigned int pico_getSetupModeInputNumber();
unsigned long pico_getSetupModeRemainingTime();

////////////////////////////////////////
// Energy-saving related
////////////////////////////////////////

unsigned int pico_getPowerState();

////////////////////////////////////////
// Savedata Related
////////////////////////////////////////

unsigned int pico_getLedColorSchemeFromSavedata();
unsigned int pico_getLedBrightnessFromSavedata();
unsigned int pico_getScreenOrientation();
input_number_spec * pico_getIconCustomizationDataPointer();
void pico_setSaveDataDirty();
void pico_saveDataReset();

////////////////////////////////////////
// Built-in LED functions
////////////////////////////////////////

void pico_ledSetPeriod(unsigned long);
void routine_blinkLed();

////////////////////////////////////////
// Notification of RT4K Control Interface Changes
////////////////////////////////////////

void pico_notifyControlInterfaceUsbc();
void pico_notifyControlInterfaceHd15();

////////////////////////////////////////
// Misc functions
////////////////////////////////////////
void pico_mcuReboot();