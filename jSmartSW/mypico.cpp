#include "mydebug.h"
#include "gsw_hardware.h"

#include <Arduino.h>
#include "pico/multicore.h"
#include "hardware/flash.h"

#include "mypico.h"

#include "rt4k.h"
#include "rgbled.h"
#include "usb_host.h"
#include "screen.h"
#include "keypad.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

//core synchronization
static bool core1SetupFinish = false;
static bool core2SetupFinish = false;
static bool stopCore2 = false;
static bool restartCore2 = false;

//jSmartSW main program running mode
static int mainProgramMode = JSMARTSW_NORMAL_MODE;

//pico built-in LED state
static bool picoLedState = HIGH;
static unsigned long picoLedDelay = PICO_LED_DELAY_AUTO;
static unsigned long lastPicoLedToggleTime = 0;

//Pico running state
static unsigned int gswMode = GSW_MODE_AUTO;  // Default running state = AUTO

//// gscartsw input management
static unsigned int newInput = NO_INPUT;
static unsigned int currentInput = NO_INPUT;

// energy saver
static unsigned int picoPowerState = PICO_POWER_STATE_AWAKE;
static bool energySaverEnabled = true;
static unsigned long lastWakeTime = millis();

// flash operations
jsmartsw_savedata mainSaveData;
static bool saveDataDirty;
static const uint32_t writeFlashAddr = PICO_SAVEDATA_ADDR;
static const uint32_t readFlashAddr = XIP_BASE + writeFlashAddr;
static const String magicWord = PICO_SAVEDATA_MAGICWORD;
static const String saveDataVersion = PICO_SAVEDATA_VERSION;

//keypad data
static keypad_data currentKeypadData, previousKeypadData;
static unsigned long lastButtonChangeTime = 0;
static unsigned long lastButtonDownTime = 0;
static bool shiftTriggered = false;
static bool buttonLongHold = false;
//game controller data
static game_controller_data currentControllerData, previousControllerData;
static keyboard_data currentKeyboardData, previousKeyboardData;
static int currentActiveControl = CONTROLLER_NONE;

// command text to display on OLED screen
static String commandText[(int)PHY_BUTTON_END+1] = COMMAND_TEXT_STRING_ARRAY;
static String inputText[2] = CONTROL_INTERFACE_NAME_STRING;

// Setup mode
static unsigned int setupModeInput = NO_INPUT;
static unsigned int setupIdleStartTime = 0;
static bool setupMode_initialEnter = true;

/////////////// FUNCTION PROTOTYPES ////////////////
  ////////////////////////////////////////
  // jSmartSW Input Modes & Logic
  ////////////////////////////////////////

  static void routine_gswInputUpdate();
  static bool isAutoModeDisabled();   //check if auto mode is physically disabled
  static void gswMode_changeToAuto();
  static void gswMode_changeToManual();

  static void cycleGswInputDown();
  static void cycleGswInputUp();
  static void cycleGswInput(bool);

  ////////////////////////////////////////
  // Control interface functions
  ////////////////////////////////////////

  static void routine_keypad();
  static unsigned int keypadToInputNumber(keypad_reading);
  static bool isSingleKeyPress(keypad_reading);

  static void routine_controller();
  static void routine_keyboard();

  static void clearAllInputData();

  ////////////////////////////////////////
  // Setup Mode Logic
  ////////////////////////////////////////

  static void enterSetupMode();
  static void exitSetupMode();
  static void setupMode_keepalive();

  ////////////////////////////////////////
  // Energy-saving related
  ////////////////////////////////////////

  static void peripheralWakeup();
  static void routine_detectActivity();
  static void routine_energySaving();
  static bool isThereHardwareActivity();

  ////////////////////////////////////////
  // Various functions
  ////////////////////////////////////////

  void flipScreen();
  void cycleRgbLedColorScheme();
  void cycleRgbLedBrightness();

  ////////////////////////////////////////
  // Savedata Related
  ////////////////////////////////////////

  static void readSaveDataFromFlash();   // read on init and keep in RAM
  static bool saveDataVerificationFailed();
  static void routine_writeSaveDataToFlash();  //only write to flash when data is dirty and pico is in sleep power state

  ////////////////////////////////////////
  // Built-in LED functions
  ////////////////////////////////////////

  static void picoLedInit();
  static void keypadBootCheck();
//////////// END OF FUNCTION PROTOTYPES ////////////

////////////////////////////////////////
// Main Functions
////////////////////////////////////////

// Setup for different cores
void jsmartsw_core1Setup()
{
  ///////////////////////////////////////////
  // Hardware Init
  ///////////////////////////////////////////
  // Pico native hardware initialization
  ////Pico Built-in LED Init
  picoLedInit();
  // Pico built-in uart0 init
  rt4k_uartInit();

  //Read save data from flash
  readSaveDataFromFlash();

  //controller_init_deviceRegister();

  // Peripheral initialization
  //// RGB LED
  rgbLed_init();
  ////Screen
  screen_init();

  ///////////////////////////////////////////
  // End of Hardware Init
  ///////////////////////////////////////////

  //check for stuck button
  keypadBootCheck();

  //boot animations
  ////OLED screen boot splash screen
  screen_bootSplash();
  ////RGB LED animation
  rgbLed_bootAnimation();

  ////OLED screen animation part B
  screen_bootAnimationA();

  // Switch mode to auto during initialization
  gswMode_changeToAuto();

  ////////////////////////////////////////////
  // Wait for core 2 to finish initialization
  while(!core2SetupFinish) ;
  ////////////////////////////////////////////

  //enable hardware watchdog
  watchdog_enable(PICO_WATCH_DOG_TIMEOUT, true);

  pico_setMainProgramMode(JSMARTSW_POST_BOOT);

  core1SetupFinish = true;

  #ifdef DEBUG
    debugPort->println("[JSW] Complete!");
    debugPort->flush();
  #endif
}

void jsmartsw_core2Setup()
{
  #ifdef DEBUG
    while (NULL == debugPort) ; //Do NOT start without the debug serial port being ready
  #endif

  controller_init_deviceRegister();
  usbHost_init();

  core2SetupFinish = true;
}

// Main loop for different cores
void jsmartsw_core1Loop()
{
  //Blink Pico LED
  routine_blinkLed();

  ///////////////////////////////////////////////

  routine_keypad();
  routine_controller();
  routine_keyboard();

  if (JSMARTSW_NORMAL_MODE == mainProgramMode ||
      JSMARTSW_POST_BOOT   == mainProgramMode )
  {
    routine_gswInputUpdate();  // RT4K input change
    routine_rt4kCommunication();
  } else if (JSMARTSW_SETUP_MODE == mainProgramMode)
  {
    energySaverEnabled = false;

    // auto quit after a few seconds if no activity
    if (millis() - setupIdleStartTime > SETUPMODE_AUTO_QUIT_WAIT)
    {
      exitSetupMode();
    }
  }

  ///////////////////////////////////////////////

  routine_detectActivity();

  routine_rgbLedUpdate();

  // Peripheral routines
  routine_screenUpdate();

  routine_energySaving();

  //check and write data if necessary
  routine_writeSaveDataToFlash();

  //Feed the dog
  watchdog_update();
}

void jsmartsw_core2Loop()
{
  ////////////////////////////////////////////
  // Wait for core 1 to finish initialization
  while(!core1SetupFinish) ;
  ////////////////////////////////////////////

  if (true == stopCore2)
  {
    // Stop Core2
    #ifdef FLASH_DBG
      debugPort->println("[Core2] Going to bed.");
      debugPort->flush();
    #endif

    usb_stopUsbHost();
    stopCore2 = false;
  } else if (true == restartCore2)
  {
    // Resume Core2
    #ifdef FLASH_DBG
      debugPort->println("[Core2] Waking up!");
      debugPort->flush();
    #endif

    usb_restartUsbHost();
    restartCore2 = false;
  } else {
  // Main core 2 loop
  #ifdef SECOND_LOOP_TIMING_DBG
    timingMsg2 = "";
    loopTime2 = millis();
  #endif

  routine_usbHostFtdi();

  #ifdef SECOND_LOOP_TIMING_DBG
    timingMsg2 += "<< Loop2 >> " + String(millis() - loopTime2) + "\t";
    debugPort->println(timingMsg2);
  #endif
  }
}

////////////////////////////////////////
// jSmartSW Input Modes & Logic
////////////////////////////////////////

static void routine_gswInputUpdate()
{
  //Run auto mode routine if in auto mode
  if (GSW_MODE_AUTO == gswMode)
  {
    routine_gswAutoInputUpdate();

    if (true == isAutoModeDisabled())
    {
      newInput = NO_INPUT;  // if auto mode is disabled physically
                            // don't waste time on query an just spit out NO INPUT
    } else {
      newInput = gsw_queryInput();
    }

    // Cosmetic stuff
    picoLedDelay = PICO_LED_DELAY_AUTO;
  }

  // Input change detection prodecure
  if ((newInput != currentInput) || ((NO_INPUT == currentInput) && (NO_INPUT != newInput)))
  {
    #ifdef PICO_DBG
      debugPort->println("[PICO] Current Mode: " + String((GSW_MODE_AUTO == gswMode)? "Auto":"Manual"));
      debugPort->println("[PICO] Old input: " + String(currentInput));
      debugPort->println("[PICO] New input: " + String(newInput));
    #endif

    //Manual mode - write input selection to gsw EXT pins for ONCE
    if (GSW_MODE_MANUAL == gswMode)
    {
      #ifdef PICO_DBG
        debugPort->println("[PICO] Writing input: " + String(newInput));
      #endif
      gsw_writeInput(newInput);
    }

    rt4k_inputChange(newInput);
  }
  currentInput = newInput;
}

void pico_setMainProgramMode(int newMode)
{
  mainProgramMode = newMode;
}

int pico_getMainProgramMode()
{
  return mainProgramMode;
}

unsigned int pico_getGswMode()
{
  return gswMode;
}

unsigned int pico_getCurrentInput()
{
  return currentInput;
}

void pico_setNewInput(unsigned int inputNumber)
{
  if (inputNumber < INPUT_LOWEST)
  {
    newInput = INPUT_HIGHEST;
  } else if (inputNumber > INPUT_HIGHEST)
  {
    newInput = INPUT_LOWEST;
  } else {
    newInput = inputNumber;
  }
}

//check if auto mode is physically disabled
static bool isAutoModeDisabled()
{
  bool result = false;
  pinMode(PICO_PIN_DISABLE_AUTO_MODE, INPUT_PULLUP);

  if (LOW == digitalRead(PICO_PIN_DISABLE_AUTO_MODE))
  {
    result = true;
  }

  return result;
}


static void gswMode_changeToAuto()
{
  // command text to display on OLED screen
  String commandText[BUTTON_COUNT+1] = COMMAND_TEXT_STRING_ARRAY;

  if (false == pico_getGswMode())
  {
    gswMode = GSW_MODE_AUTO;
    gsw_setModeAuto();
  }
  screen_setCommandTextUpdate(commandText[8], 0);

  lastWakeTime = millis();
}

static void gswMode_changeToManual()
{
  gswMode = GSW_MODE_MANUAL;
  gsw_setModeManual();        //Change switch running mode by setting the EXT pin
  gsw_writeInput(newInput);   //Write new input in right away
  rt4k_inputChange(newInput); //Send input change request to RT4K anyway
}

static void cycleGswInputDown()
{
  cycleGswInput(false);
}

static void cycleGswInputUp()
{
  cycleGswInput(true);
}

static void cycleGswInput(bool goUp)
{
  if (true == pico_getGswMode())
  {
    //switching state to manual mode
    gswMode_changeToManual();
  }

  if (true == goUp)
  {
    pico_setNewInput(currentInput + 1);
  } else {
    pico_setNewInput(currentInput - 1);
  }

  pico_ledSetPeriod(PICO_LED_DELAY_MANUAL);
}


////////////////////////////////////////
// Control interface functions
////////////////////////////////////////

bool pico_isKeypadShiftOn()
{
  return previousKeypadData.shiftOn;
}

static void routine_keypad()
{
  //fetch data from control interfaces
  keypad_updateButtonReading();

  //////////////////////////////////////
  // Button Parsing
  //////////////////////////////////////

  currentKeypadData.reading = keypad_currentReading();

  //decide current button state
  if (previousKeypadData.reading != currentKeypadData.reading)
  {
    lastButtonChangeTime = millis();
  }

  if (KEYPAD_VALUE_READING_NONE == currentKeypadData.reading)
  { //no button is pressed
    currentKeypadData.buttonState = BUTTON_UP;
    buttonLongHold = false;
    //buttonShortHold = false;
    currentKeypadData.shiftOn = false;
    shiftTriggered = false;
  } else {  // something is pressed down
    currentKeypadData.buttonState = BUTTON_DOWN;

    //transitioned from button up state
    if (BUTTON_UP == previousKeypadData.buttonState)
    {
      lastButtonDownTime = millis();  // take note of the moment of button pressed down
    }

    if (millis() - lastButtonDownTime > BUTTON_ABNORMAL_HOLD_THRESHOLD)
    {
      pico_mcuReboot();
    }

    if (millis() - lastButtonDownTime > BUTTON_LONG_HOLD_THRESHOLD)
    {
      buttonLongHold = true;

      if (isSingleKeyPress(currentKeypadData.reading))
      {
        setupModeInput = keypadToInputNumber(currentKeypadData.reading);
        currentActiveControl = CONTROLLER_KEYPAD;
        enterSetupMode();
      }
    } else {
      buttonLongHold = false;
    }

    if (millis() - lastButtonDownTime > BUTTON_SHORT_HOLD_THRESHOLD)
    {
      // the shift buttons is the one that's held down
      if (KEYPAD_VALUE_1_SHIFT == (currentKeypadData.reading & KEYPAD_VALUE_1_SHIFT))
      {
        currentKeypadData.shiftOn = true;

        if (false == shiftTriggered)
        {
          screen_setCommandTextUpdate(commandText[1], SCREEN_COMMAND_SHIFT_DISPLAY_DURATION);
        }
        pico_ledSetPeriod(PICO_LED_DELAY_MANUAL); //LED flash faster
      } else {
        currentKeypadData.shiftOn = false;
        shiftTriggered = false;
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////
  //button logic
  if (previousKeypadData.reading != currentKeypadData.reading)
  {
    currentActiveControl = CONTROLLER_KEYPAD;

    if (JSMARTSW_NORMAL_MODE == mainProgramMode)
    {
      if (false == currentKeypadData.shiftOn)  // releasing shift button doesn't trigger input 1 when released without combination pressed
      {
        if ((false == shiftTriggered) &&
            ((BUTTON_DOWN == previousKeypadData.buttonState) && (BUTTON_UP == currentKeypadData.buttonState)))
        // after releasing button combination, no single input can be triggered by mistake
        {
          // non-shift mode button definitions
          switch (previousKeypadData.reading)
          {
            #ifdef GSCARTSW_VER_5_2
              ////////////////// General user button combination functions //////////////////
              case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_2_WAKE:     //1+2 = Toggle RT4K Power
                rt4k_cmdPowerToggle();
                screen_setCommandTextUpdate(commandText[PHY_BUTTON_2 + 1], 0);
                break;
              case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_3_4K:      //button 1+3 = Change to 4K resolution
                rt4k_cmdRes4k();
                screen_setCommandTextUpdate(commandText[PHY_BUTTON_3 + 1], 0);
                break;
              case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_4_1080P:   //button 1+4 = Change to 1080p resolution
                rt4k_cmdRes1080p();
                screen_setCommandTextUpdate(commandText[PHY_BUTTON_4 + 1], 0);
                break;
              case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_5_HD15:    //button 1+5 = Switch RT4K input to HD15 RGBS
                // Waiting for Mike to implement
                rt4k_cmdInputHD15();
                screen_setCommandTextUpdate(commandText[PHY_BUTTON_5 + 1], 0);
                break;
              case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_6_SCART:   //button 1+6 = Switch RT4K input to SCART RGBS
                // Waiting for Mike to implement
                rt4k_cmdInputSCART();
                screen_setCommandTextUpdate(commandText[PHY_BUTTON_6 + 1], 0);
                break;
              case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_7_DIMMER:  //button 1+7 = Cycle LED brightness
                cycleRgbLedBrightness();
                break;
              case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_8_AUTO:    //button 1+8 = back to AUTO mode
                gswMode_changeToAuto();
                break;
              ////////////////// More hidden functions //////////////////
              case KEYPAD_VALUE_4_1080P|KEYPAD_VALUE_5_HD15:    // Cycle color scheme
                cycleRgbLedColorScheme();
                break;
              case KEYPAD_VALUE_6_SCART|KEYPAD_VALUE_8_AUTO:    // Force saving
                pico_setSaveDataDirty();
                break;
              case KEYPAD_VALUE_3_4K|KEYPAD_VALUE_6_SCART:    // Flip screen
                flipScreen();
                break;
              case KEYPAD_VALUE_2_WAKE|KEYPAD_VALUE_7_DIMMER:   //button 2+7 = system reset
                pico_mcuReboot();
                break;
              // ------------ single button presses (manual input change)
            #endif

              default:
                if (false == previousKeypadData.shiftOn)
                {
                  unsigned int temporaryInput = keypadToInputNumber(previousKeypadData.reading);

                  // if real input change is requested, write input right now
                  if ((temporaryInput >= INPUT_LOWEST ) && (temporaryInput <= INPUT_HIGHEST))
                  {
                    pico_setNewInput(temporaryInput);
                    //if currently in AUTO mode, transition to manual mode
                    if (GSW_MODE_AUTO == pico_getGswMode())
                    {
                      //switching state to manual mode
                      gswMode_changeToManual();
                    }
                    //cosmetic stuff
                    pico_ledSetPeriod(PICO_LED_DELAY_MANUAL);   //LED flash faster
                  }
                }
          }
        }
      // end of non-shift state logic
      } else {
        // Button definitions
        switch (currentKeypadData.reading)
        {
          #ifdef GSCARTSW_VER_5_2
            // ------------ button combination cases
            case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_2_WAKE:     //1+2 = Toggle RT4K Power
              rt4k_cmdPowerToggle();
              screen_setCommandTextUpdate(commandText[2], 0);
              break;
            case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_3_4K:    //button 1+3 = Change to 4K resolution
              rt4k_cmdRes4k();
              screen_setCommandTextUpdate(commandText[3], 0);
              break;
            case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_4_1080P:    //button 1+4 = Change to 1080p resolution
              rt4k_cmdRes1080p();
              screen_setCommandTextUpdate(commandText[4], 0);
              break;
            case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_5_HD15:    //button 1+5 = Switch RT4K input to HD15 RGBS
              // Waiting for Mike to implement
              rt4k_cmdInputHD15();
              screen_setCommandTextUpdate(commandText[5], 0);
              break;
            case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_6_SCART:    //button 1+6 = Switch RT4K input to SCART RGBS
              // Waiting for Mike to implement
              rt4k_cmdInputSCART();
              screen_setCommandTextUpdate(commandText[6], 0);
              break;
            case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_7_DIMMER:    //button 1+7 = Cycle LED brightness
              cycleRgbLedBrightness();
              break;
            case KEYPAD_VALUE_1_SHIFT|KEYPAD_VALUE_8_AUTO:   //button 1+8 = back to AUTO mode
              gswMode_changeToAuto();
              break;
            case KEYPAD_VALUE_2_WAKE|KEYPAD_VALUE_7_DIMMER:    //button 2+7 = system reset
              pico_mcuReboot();
              break;
          #endif
        }
        //leave a mark so single button action can't be triggered after releasing SHIFT
        shiftTriggered = true;
        lastButtonDownTime = millis();
      //end of shift mode logic
      }
    } else if (JSMARTSW_SETUP_MODE == mainProgramMode)
    {
      if (BUTTON_DOWN == previousKeypadData.buttonState &&
          BUTTON_UP == currentKeypadData.buttonState    &&
          true == setupMode_initialEnter)
      {
        setupMode_initialEnter = false;
        previousKeypadData = keypad_data();
        currentKeypadData = keypad_data();
      }

      if (KEYPAD_VALUE_READING_NONE != previousKeypadData.reading)
      {
        setupMode_keepalive();
      }

      if (false == setupMode_initialEnter)
      {
        switch (previousKeypadData.reading)
        {
          #ifdef GSCARTSW_VER_5_2
            // ------------ button combination cases
            case KEYPAD_VALUE_1_SHIFT:
              screen_setupMode_iconIdCycleDown();
              break;
            case KEYPAD_VALUE_2_WAKE:
              screen_setupMode_iconIdCycleUp();
              break;
            case KEYPAD_VALUE_3_4K:
              screen_setupMode_iconSetCycleUp();
              break;
            case KEYPAD_VALUE_4_1080P:
              screen_setupMode_iconSetCycleDown();
              break;
            case KEYPAD_VALUE_5_HD15:
              break;
            case KEYPAD_VALUE_6_SCART:
              screen_setupMode_confirmChoice();
              exitSetupMode();
              break;
            case KEYPAD_VALUE_7_DIMMER:
              exitSetupMode();
              break;
            case KEYPAD_VALUE_8_AUTO:
              screen_setupMode_resetToDefault();
              exitSetupMode();
              break;
          #endif
        }
      }
    }
  }

  #ifdef BUTTON_LOGIC_DBG
    debugPort->print("[BTN_LOGIC] Reading: " +  String(previousKeypadData.reading)                                  + "\t:" + String(currentKeypadData.reading)                                   + "\t");
    debugPort->print("State: " +                String((previousKeypadData.buttonState == BUTTON_DOWN)?"Down":"Up") + "\t:" + String((currentKeypadData.buttonState == BUTTON_DOWN)?"Down":"Up")  + "\t");
    debugPort->print("Shift: " +                String((previousKeypadData.shiftOn == true)?"Yes":"No")             + "\t:" + String((currentKeypadData.shiftOn == true)?"Yes":"No")              + "\t");
    debugPort->print("Time Chg: " +             String(lastButtonChangeTime)                              + "\t");
    debugPort->print("Time Dwn: " +             String(lastButtonDownTime)                                + "\t");
    debugPort->println("Trigger: " +            String((shiftTriggered == true)?"Yes":"No") );
  #endif

  previousKeypadData = currentKeypadData;
}

#ifdef GSCARTSW_VER_5_2
static unsigned int keypadToInputNumber(keypad_reading keypadData)
{
  unsigned int temporaryInput = NO_INPUT;

  switch (keypadData) // Single buttons for input change
  {
    case KEYPAD_VALUE_1_SHIFT:
      temporaryInput = INPUT_1;
      break;
    case KEYPAD_VALUE_2_WAKE:
      temporaryInput = INPUT_2;
      break;
    case KEYPAD_VALUE_3_4K:
      temporaryInput = INPUT_3;
      break;
    case KEYPAD_VALUE_4_1080P:
      temporaryInput = INPUT_4;
      break;
    case KEYPAD_VALUE_5_HD15:
      temporaryInput = INPUT_5;
      break;
    case KEYPAD_VALUE_6_SCART:
      temporaryInput = INPUT_6;
      break;
    case KEYPAD_VALUE_7_DIMMER:
      temporaryInput = INPUT_7;
      break;
    case KEYPAD_VALUE_8_AUTO:
      temporaryInput = INPUT_8;
  }

  return temporaryInput;
}
#endif

static bool isSingleKeyPress(keypad_reading buttonReading)
{
  const keypad_reading buttonArray[PHY_BUTTON_END] = KEYPAD_VALUE_LIST;

  bool result = false;

  for (int i = PHY_BUTTON_1; i < PHY_BUTTON_END; i++)
  {
    if (buttonArray[i] == buttonReading)
    {
      result = true;
    }
  }

  return result;
}

///////////////////

static void routine_controller()
{
  ////////////////////////////////////////////////////////////////////////
  //controller logic
  bool controllerButtonPressed = false;

  static unsigned long controllerInitialRepeatWait = 0;
  static unsigned long controllerLastRepeatTime = 0;

  currentControllerData = controller_currentReading();

  if  ( currentControllerData.dpad_up       ||
        currentControllerData.dpad_down     ||
        currentControllerData.dpad_right    ||
        currentControllerData.dpad_left     ||

        currentControllerData.apad_north    ||
        currentControllerData.apad_west     ||
        currentControllerData.apad_south    ||
        currentControllerData.apad_east     ||

        currentControllerData.button_select ||
        currentControllerData.button_start  ||
        currentControllerData.button_home   ||

        currentControllerData.button_l1     ||
        currentControllerData.button_r1     ||
        currentControllerData.button_l2     ||
        currentControllerData.button_r2     ||
        currentControllerData.button_l3     ||
        currentControllerData.button_r3     ||
        
        currentControllerData.button_touchPadButton
      )
  {
    controllerButtonPressed = true;
    currentActiveControl = currentControllerData.controllerType;

    if (JSMARTSW_SETUP_MODE == mainProgramMode)
    {
      setupMode_keepalive();
      setupMode_initialEnter = false;
    }
  } else {
    controllerButtonPressed = false;
    controller_tickReset();
  }

  if (memcmp (&currentControllerData, &previousControllerData, sizeof(game_controller_data)))
  {
    if (JSMARTSW_NORMAL_MODE == mainProgramMode)
    {
      //process dpad. Do up & down first so accidental left/right input will be ignored
      if (currentControllerData.dpad_up   ) {rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_UP));}
      if (currentControllerData.dpad_down ) {rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_DOWN));}
      if (currentControllerData.dpad_right) {rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_RIGHT));}
      if (currentControllerData.dpad_left ) {rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_LEFT));}

      if (currentControllerData.apad_north) {rt4k_cycleQuickTabsIncrease();}
      if (currentControllerData.apad_west ) {rt4k_cycleQuickTabsDecrease();}
      if (currentControllerData.apad_south) {rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_OK));}
      if (currentControllerData.apad_east ) {rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_BACK));}

      if (currentControllerData.button_select ) {rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_DIAG));}
      if (currentControllerData.button_start  ) {rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_MENU));}

      if (currentControllerData.button_l1 ) {cycleGswInputDown();}
      if (currentControllerData.button_r1 ) {cycleGswInputUp();}

      if (currentControllerData.button_l2 ) {}
      if (currentControllerData.button_r2 ) {}

      if (currentControllerData.button_l3) {gswMode_changeToAuto();}
      #ifndef DEBUG
      
      if (currentControllerData.button_r3)
      {
        if (NO_INPUT != currentInput)
        {
          setupModeInput = currentInput;
          enterSetupMode();
        }
      }

      #else
        if (currentControllerData.button_r3) {pico_setSaveDataDirty();}
      #endif

      if (currentControllerData.button_touchPadButton)  {rt4k_remoteButtonSingle( String(RT4K_IR_REMOTE_PROF));}
    } else if (JSMARTSW_SETUP_MODE == mainProgramMode)
    {
      // Directions
      if (currentControllerData.dpad_up   ) {screen_setupMode_iconSetCycleUp();}
      if (currentControllerData.dpad_down ) {screen_setupMode_iconSetCycleDown();}
      if (currentControllerData.dpad_right) {screen_setupMode_iconIdCycleUp();}
      if (currentControllerData.dpad_left ) {screen_setupMode_iconIdCycleDown();}

      // Confirm button
      if (currentControllerData.apad_south)
      {
        screen_setupMode_confirmChoice();
        exitSetupMode();
      }

      // Cancel button
      if (currentControllerData.apad_east ) {
        exitSetupMode();
      }

      // Clear button
      if (currentControllerData.apad_north    ||    //PS
          currentControllerData.button_start  ||    //SwPro
          currentControllerData.button_l3       )   //Saturn
      {
        screen_setupMode_resetToDefault();
        exitSetupMode();
      }
    }

    if (true == controllerButtonPressed)
    {
      controllerInitialRepeatWait = millis();
    }
  } else {
    if ((millis() - controllerLastRepeatTime > CONTROLLER_REPEAT_INTERVAL) &&
        (millis() - controllerInitialRepeatWait > CONTROLLER_REPEAT_WAIT))
    {
      if (JSMARTSW_NORMAL_MODE == mainProgramMode)
      {
        //process dpad hold
        if (currentControllerData.dpad_down)
        {
          rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_DOWN));
        } else
        if (currentControllerData.dpad_up)
        {
          rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_UP));
        } else
        if (currentControllerData.dpad_right)
        {
          rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_RIGHT));
          controller_holdLeftRightRumbleTick(HOLDING_RIGHT, currentControllerData.dev_addr, currentControllerData.instance);
        } else
        if (currentControllerData.dpad_left)
        {
          rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_LEFT));
          controller_holdLeftRightRumbleTick(HOLDING_LEFT, currentControllerData.dev_addr, currentControllerData.instance);
        }

        //allow holding the cancel button
        if (currentControllerData.apad_east)
        {
          rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_BACK));
        }

        //home button to turn on RT4K is a hold action
        if (currentControllerData.button_home   ) { rt4k_remoteWakeFromSleep();
                                                    controllerInitialRepeatWait = millis();}
      } else if (JSMARTSW_SETUP_MODE == mainProgramMode)
      {
        if (currentControllerData.dpad_right) {screen_setupMode_iconIdCycleUp();}   else
        if (currentControllerData.dpad_left)  {screen_setupMode_iconIdCycleDown();} else
        if (currentControllerData.dpad_up   ) {screen_setupMode_iconSetCycleUp();}  else
        if (currentControllerData.dpad_down ) {screen_setupMode_iconSetCycleDown();}

        //setupMode_keepalive();
      }

      controllerLastRepeatTime = millis();
    }
  }

  previousControllerData = currentControllerData;
}

///////////////////

static void routine_keyboard()
{
  ////////////////////////////////////////////////////////////////////////
  //keyboard logic
  static unsigned long keyboardInitialRepeatWait = 0;
  static unsigned long keyboardLastRepeatTime = 0;
  currentKeyboardData = keyboard_currentReading();

  if  ( currentKeyboardData.firstKey != 0  /*||
        currentKeyboardData.modifier != 0    */)
  {
    currentActiveControl = CONTROLLER_KEYBOARD;

    if (JSMARTSW_SETUP_MODE == mainProgramMode)
    {
      setupMode_keepalive();
      setupMode_initialEnter = false;
    }

    if (currentKeyboardData.firstKey != previousKeyboardData.firstKey)
    {
      #ifdef KEYBOARD_DBG
        debugPort->println("[Keyboard] Single press: 0x" + String(currentKeyboardData.firstKey, HEX));
      #endif

      unsigned int temporaryInput = 0;

      if (JSMARTSW_NORMAL_MODE == mainProgramMode)
      {
        switch(currentKeyboardData.firstKey)
        {
          case 0x2c:  //Space
          case 0x62:  //Numpad 0
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_MENU));
            break;

          case 0x2a:  //Backspace
          case 0x4a:  //Home
          case 0x63:  //Numpad Del
            gswMode_changeToAuto();
            break;

          case 0x2d:  //-
          case 0x59:  //Numpad 1
          case 0x56:  //Numpad -
            cycleGswInputDown();
            break;

          case 0x2e:  //+
          case 0x5b:  //Numpad 3
          case 0x57:  //Numpad +
            cycleGswInputUp();
            break;

          case 0x2f:  // [
          case 0x4b:  //PgUp
          case 0x5f:  //Numpad 7
            rt4k_cycleQuickTabsDecrease();
            break;

          case 0x30:  // ]
          case 0x4e:  //PgDw
          case 0x61:  //Numpad 9
            rt4k_cycleQuickTabsIncrease();
            break;

          case 0x31:  //Slash '/'
          case 0x48:  //Break
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_DIAG));
            break;

          case 0x35:  //`
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_PROF));
            break;

          ///////////////////////////////////////////////////////
          // navigations
          case 0x29:  //ESC
          case 0x53:  //Numlock
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_BACK));
            break;

          case 0x28: //Enter
          case 0x58: //Numpad enter
          case 0x5d: //Numpad 5
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_OK));
            break;

          case 0x4f: //Arrow Right
          case 0x5e: //Numpad 6
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_RIGHT));
            break;

          case 0x50: //Arrow Left
          case 0x5c: //Numpad 4
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_LEFT));
            break;

          case 0x51: //Arrow Down
          case 0x5a: //Numpad 2
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_DOWN));
            break;

          case 0x52: //Arrow Up
          case 0x60: //Numpad 8
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_UP));
            break;

          ///////////////////////////////////////////////////////
          // Quick Profile button 1 to 12
          // Map to F1 to F12
          case 0x3a:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_1));
            break;
          case 0x3b:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_2));
            break;
          case 0x3c:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_3));
            break;
          case 0x3d:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_4));
            break;
          case 0x3e:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_5));
            break;
          case 0x3f:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_6));
            break;
          case 0x40:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_7));
            break;
          case 0x41:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_8));
            break;
          case 0x42:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_9));
            break;
          case 0x43:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_10));
            break;
          case 0x44:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_11));
            break;
          case 0x45:
            rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_12));
            break;

          ///////////////////////////////////////////////////////
          // number 1 - 8 change input
          case 0x1e:
            temporaryInput = INPUT_1;
            break;
          case 0x1f:
            temporaryInput = INPUT_2;
            break;
          case 0x20:
            temporaryInput = INPUT_3;
            break;
          case 0x21:
            temporaryInput = INPUT_4;
            break;
          case 0x22:
            temporaryInput = INPUT_5;
            break;
          case 0x23:
            temporaryInput = INPUT_6;
            break;
          case 0x24:
            temporaryInput = INPUT_7;
            break;
          case 0x25:
            temporaryInput = INPUT_8;
            break;

          ///////////////////////////////////////////////////////
          // Setup mode
          case 0x49: //Ins - Enter setup mode if there is an active input
            if (NO_INPUT != currentInput)
            {
              setupModeInput = currentInput;
              currentActiveControl = CONTROLLER_KEYBOARD;
              enterSetupMode();
            }
            break;
        }

        // if real input change is requested, write input right now
        if ((temporaryInput >= INPUT_LOWEST ) && (temporaryInput <= INPUT_HIGHEST))
        {
          pico_setNewInput(temporaryInput);
          //if currently in AUTO mode, transition to manual mode
          if (GSW_MODE_AUTO == pico_getGswMode())
          {
            //switching state to manual mode
            gswMode_changeToManual();
          }
          //cosmetic stuff
          pico_ledSetPeriod(PICO_LED_DELAY_MANUAL);   //LED flash faster
        }
      } else if (JSMARTSW_SETUP_MODE == mainProgramMode)
      {
        switch(currentKeyboardData.firstKey)
        {
          case 0x29: //ESC - exit setup mode
            exitSetupMode();
            break;

          case 0x28: //Enter
          case 0x58: //Numpad enter - save and exit setup mode
            screen_setupMode_confirmChoice();
            exitSetupMode();
            break;

          case 0x4c: //Del - clear selection and exit setup mode
            screen_setupMode_resetToDefault();
            exitSetupMode();
            break;

          //directions
          case 0x4f: //Arrow Right
          case 0x5e: //Numpad Right
            screen_setupMode_iconIdCycleUp();
            break;

          case 0x50: //Arrow Left
          case 0x5c: //Numpad Left
            screen_setupMode_iconIdCycleDown();
            break;

          case 0x51: //Arrow Down
          case 0x5a: //Numpad Down
            screen_setupMode_iconSetCycleDown();
            break;

          case 0x52: //Arrow Up
          case 0x60: //Numpad Up
            screen_setupMode_iconSetCycleUp();
            break;
        }
      }

      keyboardInitialRepeatWait = millis();
    } else {
      if ((millis() - keyboardLastRepeatTime > CONTROLLER_REPEAT_INTERVAL) &&
          (millis() - keyboardInitialRepeatWait > CONTROLLER_REPEAT_WAIT))
      {
        #ifdef KEYBOARD_DBG
          debugPort->println("[Keyboard] Holding key: 0x" + String(currentKeyboardData.firstKey, HEX));
        #endif

        if (JSMARTSW_NORMAL_MODE == mainProgramMode)
        {
          switch(currentKeyboardData.firstKey)
          {
            //only directions are repeateable
            case 0x4f: //Arrow Right
            case 0x5e: //Numpad Right
              rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_RIGHT));
              break;

            case 0x50: //Arrow Left
            case 0x5c: //Numpad Left
              rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_LEFT));
              break;

            case 0x51: //Arrow Down
            case 0x5a: //Numpad Down
              rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_DOWN));
              break;

            case 0x52: //Arrow Up
            case 0x60: //Numpad Up
              rt4k_remoteButtonSingle(String(RT4K_IR_REMOTE_UP));
              break;
          }
        } else if (JSMARTSW_SETUP_MODE == mainProgramMode)
        {
          switch(currentKeyboardData.firstKey)
          {
            case 0x4f: //Arrow Right
            case 0x5e: //Numpad Right
              screen_setupMode_iconIdCycleUp();
              break;

            case 0x50: //Arrow Left
            case 0x5c: //Numpad Left
              screen_setupMode_iconIdCycleDown();
              break;

            case 0x51: //Arrow Down
            case 0x5a: //Numpad Down
              screen_setupMode_iconSetCycleDown();
              break;

            case 0x52: //Arrow Up
            case 0x60: //Numpad Up
              screen_setupMode_iconSetCycleUp();
              break;
          }
        }

        keyboardLastRepeatTime = millis();
      }
    }
  }

  previousKeyboardData = currentKeyboardData;
}

///////////////////

bool pico_isThereInputActivity()
{
  return (keypad_isThereButtonActivity() || controller_isThereActivity());
}

int pico_currentControlDevice()
{
  return currentActiveControl;
}


static void clearAllInputData()
{
  previousKeypadData      = currentKeypadData     = keypad_data();
  previousControllerData  = currentControllerData = game_controller_data();
  previousKeyboardData    = currentKeyboardData   = keyboard_data();
}

////////////////////////////////////////
// Setup Mode Logic
////////////////////////////////////////

static void enterSetupMode()
{
  clearAllInputData();
  setupMode_initialEnter = true;
  mainProgramMode = JSMARTSW_ENTERING_SETUP_MODE;

  setupMode_keepalive();
}

static void exitSetupMode()
{
  clearAllInputData();
  energySaverEnabled = true;
  mainProgramMode = JSMARTSW_EXITING_SETUP_MODE;
}

static void setupMode_keepalive()
{
  setupIdleStartTime = millis();
}

unsigned int pico_getSetupModeInputNumber()
{
  return setupModeInput;
}

unsigned long pico_getSetupModeRemainingTime()
{
  return (SETUPMODE_AUTO_QUIT_WAIT + setupIdleStartTime - millis());
}

////////////////////////////////////////
// Energy-saving related
////////////////////////////////////////

unsigned int pico_getPowerState()
{
  return picoPowerState;
}

static void peripheralWakeup()
{
  screen_wakeUp();
  rgbLed_wakeup();

  picoPowerState = PICO_POWER_STATE_AWAKE;
  lastWakeTime = millis();
}

static bool isThereHardwareActivity()
{
  return (keypad_isThereButtonActivity() ||
          controller_isThereActivity()    ||
          screen_isThereActivity()        ||
          rt4k_attemptingPowerAction()    ||
          usb_isThereActivity()
          );
}

static void routine_detectActivity()
{
  if (true == isThereHardwareActivity())
  {
    peripheralWakeup();
  }
}

static void routine_energySaving()
{
  #ifdef PWR_DEBUG
    debugPort->println("[PICO] Cur Pwr: " + String(picoPowerState));
  #endif

  //Dim mode
  if (millis() - lastWakeTime > SCREEN_SAVING_PERIOD1)
  {
    screen_dimming(); //Screen dimming - all modes
    picoPowerState = PICO_POWER_STATE_DOZE;
  }

  //Darkout mode - auto mode only
  if ((millis() - lastWakeTime > SCREEN_SAVING_PERIOD2) &&
      (GSW_MODE_AUTO == gswMode) &&
      (true == energySaverEnabled))
  {
    screen_turnOff();
    rgbLed_goToSleep();

    picoPowerState = PICO_POWER_STATE_ASLEEP;
  }
}

////////////////////////////////////////
// Savedata Related
////////////////////////////////////////

#define savedataTo256Blocks(X)  ((1 + ((X - 1) >>  8)) <<  8)
#define savedataTo4096Blocks(X) ((1 + ((X - 1) >> 12)) << 12)

// read on init and keep in RAM
static void readSaveDataFromFlash()
{
  #ifdef FLASH_DBG
    debugPort->println("[FLS] Reading from flash");
    debugPort->println("[FLS] Addr:" + String(readFlashAddr, HEX));
  #endif
  memcpy(&mainSaveData, (const uint8_t *)readFlashAddr, sizeof(jsmartsw_savedata));

  #ifdef INJECT_FAKE_DATA
    jsmartsw_savedata fakeSaveData;
    extern input_number_spec fakeInputIconData;
    memcpy(mainSaveData.magicWord, magicWord.c_str(), magicWord.length());
    memcpy(mainSaveData.inputIconCustomizationData, &fakeInputIconData, sizeof(fakeInputIconData));
    mainSaveData.rgbled_colorScheme = RGBLED_COLOR_SCHEME_INTERGALATIC;
  #endif

  //flash content check failed
  if(true == saveDataVerificationFailed())
  {
    #ifdef FLASH_DBG
      debugPort->println("[FLS] Magic word mismatch!");
      debugPort->println("[FLS] Memory: " + String(mainSaveData.magicWord));
      debugPort->println("[FLS] Reset to default data...");
    #endif

    pico_saveDataReset();
    /*
    //copy over magic word
    memcpy(mainSaveData.magicWord,  magicWord.c_str(),       magicWord.length());
    memcpy(mainSaveData.version,    saveDataVersion.c_str(), saveDataVersion.length());
    //fill default values to input customization data array
    mainSaveData.rgbled_colorScheme = RGBLED_COLOR_SCHEME_DEFAULT;
    mainSaveData.rgbled_brightnessIndex = RGBLED_BRIGHTNESS_LV_DEFAULT;

    for (int i = 0; i< INPUT_END; i++)
    {
      mainSaveData.inputIconCustomizationData[i] = {INPUT_NUMBER_IS_BIG, 0, 0};
    }

    //write re-initialized data back into flash
    saveDataDirty = true;
    */
  } else {
    #ifdef FLASH_DBG
      debugPort->println("[FLS] Magic word match!");
      debugPort->println("[FLS] Flash: " + String(mainSaveData.magicWord));
    #endif
    saveDataDirty = false;
  }
}

static bool saveDataVerificationFailed()
{
  return (0 != memcmp( mainSaveData.magicWord,  magicWord.c_str(),        magicWord.length() )        ||
          0 != memcmp( mainSaveData.version,    saveDataVersion.c_str(),  saveDataVersion.length() )   );
}

unsigned int pico_getLedColorSchemeFromSavedata()
{
  return mainSaveData.rgbled_colorScheme;
}

unsigned int pico_getLedBrightnessFromSavedata()
{
  return mainSaveData.rgbled_brightnessIndex;
}

input_number_spec* pico_getIconCustomizationDataPointer()
{
  return &(mainSaveData.inputIconCustomizationData[0]);
}

unsigned int pico_getScreenOrientation()
{
  return mainSaveData.screen_rotation;
}

void pico_setSaveDataDirty()
{
  #ifdef FLASH_DBG
    debugPort->println("[FLS] Request saving");
    debugPort->flush();
  #endif
  saveDataDirty = true;
}

void pico_saveDataReset()
{
  //copy over magic word
  memcpy(mainSaveData.magicWord,  magicWord.c_str(),       magicWord.length());
  memcpy(mainSaveData.version,    saveDataVersion.c_str(), saveDataVersion.length());
  //fill default values to input customization data array
  mainSaveData.rgbled_colorScheme = RGBLED_COLOR_SCHEME_DEFAULT;
  mainSaveData.rgbled_brightnessIndex = RGBLED_BRIGHTNESS_LV_DEFAULT;
  mainSaveData.screen_rotation = SCREEN_ROTATION_DEFAULT;

  for (int i = 0; i< INPUT_END; i++)
  {
    mainSaveData.inputIconCustomizationData[i] = {INPUT_NUMBER_IS_BIG, 0, 0};
  }

  //write re-initialized data back into flash
  saveDataDirty = true;
}

//only write to flash when data is dirty and pico is in sleep power state
static void routine_writeSaveDataToFlash()
{
  #ifdef FORCE_FLASH
    static bool alreadyForcedFlashing = false;

    if (false == alreadyForcedFlashing)
    {
      debugPort->println("[FLS] Force saving!");
      saveDataDirty = true; //force flashing
      alreadyForcedFlashing = true;
    }
  #endif

  if ((true == saveDataDirty) && (PICO_POWER_STATE_AWAKE != picoPowerState))
  {
    #ifdef FLASH_DBG
      debugPort->println("[FLS] Saving needed!");
      debugPort->flush();
    #endif

    #ifdef FLASH_DBG
      debugPort->println("[FLS] Pausing Core 2");
      debugPort->flush();
    #endif
    stopCore2 = true;
    delay(100);
    rp2040.idleOtherCore();
    delay(100);

    #ifdef FLASH_DBG
      debugPort->println("[FLS] Erasing flash");
      debugPort->println("[FLS] Addr: " + String(writeFlashAddr, HEX));
      debugPort->println("[FLS] size: " + String(savedataTo4096Blocks(sizeof(jsmartsw_savedata))));
      debugPort->flush();
    #endif
    
    flash_range_erase   (writeFlashAddr, savedataTo4096Blocks(sizeof(jsmartsw_savedata)));
    delay(100);

    #ifdef FLASH_DBG
      debugPort->println("[FLS] Saving...");
      debugPort->flush();
    #endif    

    flash_range_program (writeFlashAddr, (uint8_t*)&mainSaveData, savedataTo256Blocks(sizeof(jsmartsw_savedata)));

    #if defined FLASH_DBG  || defined FORCE_FLASH
      debugPort->println("[FLS] Save complete");
      debugPort->flush();
    #endif
    delay(100);

    #ifdef PICO_DBG
      debugPort->println("[PICO] Data saved.");
      debugPort->flush();
      
    #endif
    delay(100);

    #ifdef FLASH_DBG
      debugPort->println("[FLS] Resuming 2nd core");
      debugPort->flush();
    #endif

    restartCore2 = true;
    rp2040.resumeOtherCore();

    saveDataDirty = false;
    screen_setCommandTextUpdate("SAVED", SETUPMODE_MESSAGE_DISPLAY_DURATION);
  }
}

////////////////////////////////////////
// Built-in LED functions
////////////////////////////////////////

void routine_blinkLed()
{
  if ((millis() - lastPicoLedToggleTime) > picoLedDelay)
  {
    digitalWrite(LED_BUILTIN, picoLedState);
    if (HIGH == picoLedState)
    {
      picoLedState = LOW;
    } else {
      picoLedState = HIGH;
    }
    lastPicoLedToggleTime = millis();
  }
}

void pico_ledSetPeriod(unsigned long newPeriod)
{
  picoLedDelay = newPeriod;
}

static void picoLedInit()
{
  mainProgramMode = JSMARTSW_BOOT_SEQUENCE;

  pinMode(LED_BUILTIN, OUTPUT);
}

////////////////////////////////////////
// Notification of RT4K Control Interface Changes
////////////////////////////////////////

void pico_notifyControlInterfaceUsbc()
{
  screen_setCommandTextUpdate(inputText[RT4K_UART_PORT_TYPE_USBC], CONTROL_INTERFACE_DISPLAY_DURATION);
}

void pico_notifyControlInterfaceHd15()
{
  screen_setCommandTextUpdate(inputText[RT4K_UART_PORT_TYPE_HD15], CONTROL_INTERFACE_DISPLAY_DURATION);
}

////////////////////////////////////////
// Misc functions
void pico_mcuReboot()
{
  #ifdef PICO_DBG
    debugPort->println("[PICO] Manual Rebooting...\n\n");
  #endif
  usb_stopUsbHost();

  screen_rebootScreen();

  //enable hardware watchdog
  watchdog_enable(10, true);
  //Wait for watchdog timeout and reboot
  while(1);
}

//Boot sequence check for stuck push buttons
static void keypadBootCheck()
{ 
  const pin_size_t buttonPin[(int)PHY_BUTTON_END] = PICO_PIN_BUTTON_ARRAY;

  int buttonState = BUTTON_UP;
  //array to store button reading - 0 is good, 1 is bad
  int buttonReading[(int)PHY_BUTTON_END];

  // Button pin mode setup
  for (int i = (int)PHY_BUTTON_1; i < (int)PHY_BUTTON_END; i++)
  {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

  //Initial button reading for stuck button detection
  for (int i = (int)PHY_BUTTON_1; i < (int)PHY_BUTTON_END; i++)
  {
    //initialize button reading to good first
    buttonReading[i] = BUTTON_UP;
    PinStatus currentButton = digitalRead(buttonPin[i]);

    if (LOW == currentButton)
    {
      buttonState = BUTTON_DOWN;
      buttonReading[i] = BUTTON_DOWN;

      #ifdef RAW_BUTTON_DBG
        debugPort->println("[BUTTON] Button "+ String(i+1) + " is stuck");
      #endif
    }
  }

  // Button error detected
  if (BUTTON_DOWN == buttonState)
  {
    #ifdef PICO_DBG
      delay(500);
      debugPort->println("[PICO] Stuck button detected!! Rebooting in 3 seconds...\n");
    #endif

    pico_ledSetPeriod(PICO_LED_DELAY_FAULTY);

    if (BUTTON_DOWN == buttonReading[1] && BUTTON_DOWN == buttonReading[6])
    {
      if (true == watchdog_enable_caused_reboot() &&
          true == watchdog_caused_reboot()          )
      {
        #ifdef PICO_DBG
          debugPort->println("[PICO] Should reset savedata!\n");
        #endif
        screen_clearSaveDataMessage();
        rgbLed_clearSaveDataAnimation();
        pico_saveDataReset();
        picoPowerState = PICO_POWER_STATE_DOZE;
        routine_writeSaveDataToFlash();
        delay(1000);
        pico_mcuReboot();
      } 
    } else {
      screen_buttonErrorRebootMessage();
      rgbLed_flashStuckButtons(buttonReading, PICO_BUTTON_ERROR_BLINK_CYCLE_COUNT);
    }
    pico_mcuReboot();
  }
}

void flipScreen()
{
  if (SCREEN_ROTATION_DEFAULT == mainSaveData.screen_rotation)
  {
    mainSaveData.screen_rotation = SCREEN_ROTATION_FLIP;
  } else {
    mainSaveData.screen_rotation = SCREEN_ROTATION_DEFAULT;
  }
  pico_setSaveDataDirty();

  screen_flip(mainSaveData.screen_rotation);
}

void cycleRgbLedBrightness()
{
    mainSaveData.rgbled_brightnessIndex = rgbLed_cycleBrightness();
    screen_setCommandTextUpdate(commandText[PHY_BUTTON_7 + 1], 0);
    pico_setSaveDataDirty();
}

void cycleRgbLedColorScheme()
{
  mainSaveData.rgbled_colorScheme = rgbled_cycleColorScheme();
  pico_setSaveDataDirty();
}