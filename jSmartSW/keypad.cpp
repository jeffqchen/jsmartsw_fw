#include "mydebug.h"

#include "gsw_hardware.h"
#include "mypico.h"
#include "keypad.h"

#include "rt4k.h"

//only for boot error procedures
#include "screen.h"
#include "rgbled.h"


#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

 keypad_reading retainedButtonReading = KEYPAD_VALUE_READING_NONE;
bool buttonActivity = false;

//Pi Pico data pin definition
const pin_size_t buttonPin[(int)PHY_BUTTON_END] = PICO_PIN_BUTTON_ARRAY;

/////////////// FUNCTION PROTOTYPES ////////////////
  static keypad_reading readButtons();
//////////// END OF FUNCTION PROTOTYPES ////////////

//external functions
 keypad_reading keypad_currentReading()
{
  return retainedButtonReading;
}

bool keypad_isThereButtonActivity()
{
  return buttonActivity;
}

//routine
void keypad_updateButtonReading()
{
  static  keypad_reading immediateButtonReading = KEYPAD_VALUE_READING_NONE;
  static  keypad_reading previousImmediateButtonReading = KEYPAD_VALUE_READING_NONE;

  static  unsigned long lastButtonPollTime = 0;
  static  unsigned long lastDebounceFailureTime = 0;

  //button polling period check
  if ((millis() - lastButtonPollTime) > BUTTON_POLL_DELAY)
  {
    immediateButtonReading = readButtons();
    lastButtonPollTime = millis();
  }

  // Button press debouncing
  if (immediateButtonReading != previousImmediateButtonReading)
  {
    previousImmediateButtonReading = immediateButtonReading;
    lastDebounceFailureTime = millis();  //reset debounce timer due to changing button state
  }

  // Button press debouncing time passed
  if (millis() - lastDebounceFailureTime > BUTTON_DEBOUNCE_TIME)
  {
    //record debounced input for current cycle
    retainedButtonReading = immediateButtonReading;
  }

  #ifdef RAW_BUTTON_DBG
    debugPort->print("[BUTTON] immBR: " + String(immediateButtonReading)  + "\t");
    debugPort->print("pimBR: " + String(previousImmediateButtonReading)   + "\t");
    debugPort->println("rtnBR: " + String(retainedButtonReading)            + "\t");
  #endif

  //// record undebounced button reading
  previousImmediateButtonReading = immediateButtonReading;
}

//internal function
static keypad_reading readButtons()
{
  keypad_reading immediateButtonReading = 0;

  for (int i = (int)PHY_BUTTON_1; i < (int)PHY_BUTTON_END; i++)
  {
    immediateButtonReading |= (!(digitalRead(buttonPin[i])) << i);
  }
  
  if (immediateButtonReading != 0)
  { //immediate button down actions
    buttonActivity = true;
  } else { //immediate button up actions
    buttonActivity = false;
  }

  return immediateButtonReading;
}

/*
//Boot sequence check for stuck push buttons
void keypad_bootCheck()
{
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

    if (BUTTON_DOWN == buttonReading[1] && BUTTON_DOWN == buttonReading[6])
    {
      if (true == watchdog_enable_caused_reboot() &&
          true == watchdog_caused_reboot()          )
      {
        #ifdef PICO_DBG
          debugPort->println("[PICO] Should reset savedata!\n");
        #endif
      }
    }

    pico_ledSetPeriod(PICO_LED_DELAY_FAULTY);

    screen_buttonErrorRebootMessage();
    rgbLed_flashStuckButtons(buttonReading, PICO_BUTTON_ERROR_BLINK_CYCLE_COUNT);

    pico_mcuReboot();
  }
}
*/