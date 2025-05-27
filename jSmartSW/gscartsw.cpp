#include "mydebug.h"

#include <Arduino.h>

#include "gsw_hardware.h"
#include "gscartsw.h"
#include "mypico.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

static pin_size_t sensorPins[EXT_DATA_PIN_COUNT] = PICO_EXT_PIN_ARRAY;
static unsigned int extPinPulseCounter[3] = {0, 0, 0};
static unsigned long lastEXTpoll = 0;  //ms
static unsigned int extSampleCycleCounter = 0; //ms


static unsigned int storedAutoInput = NO_INPUT;

/////////////// FUNCTION PROTOTYPES ////////////////
  static void pulseSampling (pin_size_t *, int, unsigned int *); //gscartsw EXT port
  static void setMode(bool);
  static void extPinsHiz ();  // change EXT data pins to Hi-Z mode for protection
//////////// END OF FUNCTION PROTOTYPES ////////////

void routine_gswAutoInputUpdate()
{
  if (millis() - lastEXTpoll > INPUT_BITS_SAMPLING_PERIOD)
  {
    pulseSampling(sensorPins, EXT_DATA_PIN_COUNT, extPinPulseCounter);
    lastEXTpoll = millis();
  }

  if (extSampleCycleCounter >= REQUIRED_SAMPLE_CYCLES)
  {
    unsigned int translatedInput = 0;
    bool noInputDetected = false;

    #ifdef EXT_DBG
      String usbMsg = "[GSW EXT] extPinPulseCounter: ";
    #endif

    for (int i = 0; i < EXT_DATA_PIN_COUNT; i++)
    {
      #ifdef EXT_DBG       
        usbMsg += String(extPinPulseCounter[i]) + "\t";
      #endif

      if (extPinPulseCounter[i] > GSW_EXT_LOGIC_HIGH_COUNT)
      {
        translatedInput |= 1<<i; //convert to input ID
      } else if (extPinPulseCounter[i] > GSW_EXT_LOGIC_MID_COUNT)
      {
        noInputDetected = true;
      }

      extPinPulseCounter[i] = 0;
    }

    if (true == noInputDetected)
    {
      translatedInput = NO_INPUT;
    } else {
      translatedInput = GSCARTSW_NUM_INPUTS - translatedInput;
    }

    #ifdef EXT_DBG
      debugPort->println(usbMsg);
      debugPort->println("[GSW EXT] translatedInput = " + String(translatedInput));
    #endif

    storedAutoInput = translatedInput;
    extSampleCycleCounter = 0;
  }
}

void gsw_setModeAuto()
{
  setMode(GSW_MODE_AUTO);
}

void gsw_setModeManual()
{
  setMode(GSW_MODE_MANUAL);
}

void gsw_writeInput(unsigned int inputNumber)
{
  #ifdef SIGNAL_OUT_DBG
    String usbMsg = "[Signal bits written]\t";
  #endif
  int rawInputNumber = GSCARTSW_NUM_INPUTS - inputNumber;  //input number is inverted and shifted

  // Setting input pins state
  for (int i = 0; i < EXT_DATA_PIN_COUNT; i++)
  {
    pinMode(sensorPins[i], OUTPUT);
    digitalWrite(sensorPins[i], HIGH & (rawInputNumber)>>i);

    #ifdef SIGNAL_OUT_DBG
      usbMsg += ("[" + String(i) + "]:" + String(0x1 & (rawInputNumber)>>i) + "\t");
    #endif
  }

  #ifdef SIGNAL_OUT_DBG
    debugPort->println(usbMsg);
  #endif
}

unsigned int gsw_queryInput()
{    
  return storedAutoInput;
}

//Internal Functions

static void pulseSampling(pin_size_t * pinsToSample, int numberOfPins, unsigned int * pollResult)
{
  //Count PWM pulses
  #ifdef SIGNAL_IN_DBG
    String usbMsg = "[Signal bits read]\t";
  #endif

  for (int i = 0; i < numberOfPins; i++)
  {
    pinMode(pinsToSample[i], INPUT);
    PinStatus tempReading = digitalRead(pinsToSample[i]);

    #ifdef SIGNAL_IN_DBG
      usbMsg += ("[" + String(i) + "]:" + String(tempReading) + "\t");
    #endif

    if (HIGH == tempReading)
    {
      pollResult[i] += 1;
    }      
  }
  #ifdef SIGNAL_IN_DBG
    debugPort->println(usbMsg);
  #endif

  extSampleCycleCounter += 1;
}

static void setMode(bool targetMode)
{
  pinMode(EXT_PIN_OVERRIDE, INPUT_PULLUP);

  if (GSW_MODE_MANUAL == targetMode)
  {
    digitalWrite(EXT_PIN_OVERRIDE, GSCART_OVERRIDE_PIN_MANUAL_MODE);
  } else {
    extPinsHiz(); //set pico pins to Hi-Z first in order to protect switch EXT pins
    digitalWrite(EXT_PIN_OVERRIDE, GSCART_OVERRIDE_PIN_AUTO_MODE);
    delay(AUTO_MODE_CHANGE_DEBOUNCE);   //wait for EXT pins to calm down before trying to read
  }
}

static void extPinsHiz()  //set pico pins to Hi-Z
{
  for (int i = 0; i < EXT_DATA_PIN_COUNT; i++)
  {
    pinMode(sensorPins[i], INPUT);
  }
}