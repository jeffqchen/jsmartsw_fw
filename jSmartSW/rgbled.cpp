#define FASTLED_INTERNAL  //this line is to supress the pragma warning message of software SPI during compiling

#include "mydebug.h"
#include "gsw_hardware.h"

#include <FastLED.h>
#include "colorutils.h"
#include "rgbled.h"

#include "rt4k.h"
#include "mypico.h"

#ifdef DEBUG
  #include "SerialUART.h"
  extern HardwareSerial* debugPort;
#endif

static CRGB rgbLeds[NUM_LEDS];

static unsigned int rgbledColorSchemeIndex;
static const rgbled_normalMode_color_scheme \
rgbLed_normalMode_colorScheme[RGBLED_COLR_SCHEME_COUNT] = RGBLED_COLOR_SCHEME_ARRAY;

static const rgbled_setupMode_color_scheme  \
rgbled_setupMode_colorScheme = {HUE_YELLOW, 0,
                                HUE_YELLOW, 255,
                                HUE_GREEN,  255,
                                HUE_RED,    255,
                                HUE_BLUE,   128 };

static const rgbLed_brightness_data rgbLedBrightnessData[RGBLED_BRIGHTNESS_LEVEL_COUNT] = RGBLED_BRIGHTNESS_DATA_ARRAY;

static unsigned int rgbLedBrightnessIndex;
static unsigned int rgbLedBrightnessIndexStore;
static unsigned int rgbLedTargetBrightness;
static unsigned int rgbLedCurrentBrightness;

static unsigned long rgbLedLastRefresh = 0;

static int rgbLed_operationMode = JSMARTSW_NORMAL_MODE;

static int scanAnimationPosition = 0;
static float scanAnimationBlend = 0.0;
static int scanStepping = SCAN_STEPPING_RIGHT;
static unsigned long lastScanCycle = 0;
static CRGB scanAnimationLedArray[NUM_LEDS];

static pulse_animation_parameters fastPulse = {ANIMATION_NO_BLEND, RGBLED_BLEND_STEP_FAST};
static pulse_animation_parameters slowPulse = {ANIMATION_NO_BLEND, RGBLED_BLEND_STEP_SLOW};

// currently impossible to know these states
/*
static bool rt4k_res_4k = false;
static bool rt4k_res_1080p = false;
static bool rt4k_hd15 = false;
static bool rt4k_scart = false;
*/

/////////////// FUNCTION PROTOTYPES ////////////////
  ////////////////////////////////////////
  // Various functions
  ////////////////////////////////////////

  static void normalMode_drawLowerRowLed(HSVHue currentBgColor);
  static pulse_animation_parameters pulseAnimation(pulse_animation_parameters);
  static void normalMode_scanAnimation();
  static void normalMode_scanAnimationIncreaseBlend();
  static void normalMode_scanAnimationReduceBlend();
  static void mirrorLedTopRowToBottomRow();
  static void setupMode_directionButtonsAnimation();
//////////// END OF FUNCTION PROTOTYPES ////////////

////////////////////////////////////////
// Initialization
////////////////////////////////////////

void rgbLed_init()
{
  //retrieve saved color scheme selection and brightness settings from saved data
  rgbledColorSchemeIndex = pico_getLedColorSchemeFromSavedata();
  rgbLedBrightnessIndex = pico_getLedBrightnessFromSavedata();
  #ifdef RGBLED_DBG
    debugPort->println("[RGB LED] Retrieved saved brightness: " + String(rgbLedBrightnessData[rgbLedBrightnessIndex].brightnessValue));
  #endif

  rgbLedBrightnessIndexStore = rgbLedBrightnessIndex;
  rgbLedTargetBrightness = rgbLedBrightnessData[rgbLedBrightnessIndex].brightnessValue;
  rgbLedCurrentBrightness = rgbLedTargetBrightness;

  //Initialize NeoPixel stripe hardware
  FastLED.addLeds<RGBLED_TYPE,RGBLED_DATA_PIN,COLOR_ORDER>(rgbLeds, NUM_LEDS).setCorrection(UncorrectedColor);
  #ifdef RGBLED_POWER_LIMIT
    FastLED.setMaxPowerInVoltsAndMilliamps(RGBLED_POWER_VOLT, RGBLED_POWER_AMP);
  #endif

  //Set initial brightness
  FastLED.setBrightness(rgbLedTargetBrightness); // Must initialize with target brightness
  FastLED.setCorrection(rgbLedBrightnessData[rgbLedBrightnessIndex].correctionValue); // Shining through PCB FR4 changes the color by a lot and this requires color correction.
}

////////////////////////////////////////
// Routine
////////////////////////////////////////

void routine_rgbLedUpdate()
{
  if ((millis() - rgbLedLastRefresh) > RGBLED_FRAME_TIME)
  {
    //First of all, sync operation mode to main program
    rgbLed_operationMode = pico_getMainProgramMode();

    //////////////////////////////////////////////////
    // Brightness transition section
    //Update target brightness from the latest setting
    rgbLedTargetBrightness = rgbLedBrightnessData[rgbLedBrightnessIndex].brightnessValue;
    FastLED.setCorrection(rgbLedBrightnessData[rgbLedBrightnessIndex].correctionValue);

    // If current LED brightness is different from target, step towards the target
    if (rgbLedCurrentBrightness != rgbLedTargetBrightness)
    {
      int brightnessStepping;

      if (0 == rgbLedBrightnessIndex)
      {
        brightnessStepping = 2 * RGBLED_BRIGHTNESS_STEPPING;
      } else {
        brightnessStepping = RGBLED_BRIGHTNESS_STEPPING;
      }
      //If brightness close enough, just set it equal
      if (abs((int)(rgbLedCurrentBrightness - rgbLedTargetBrightness)) < brightnessStepping)
      {
        rgbLedCurrentBrightness = rgbLedTargetBrightness;
      }

      if (rgbLedCurrentBrightness < rgbLedTargetBrightness)
      {
        rgbLedCurrentBrightness += brightnessStepping;
      } else {
        rgbLedCurrentBrightness -= brightnessStepping;
      }
    }
    // End of brightness transition section
    //////////////////////////////////////////////////

    if (JSMARTSW_NORMAL_MODE == rgbLed_operationMode)
    {
      //keep running the scanning animation in the background
      normalMode_scanAnimation();

      //////////////////////////////////////////////////
      // Main color show!!!
      // Fill BG color with corresponding BG color, then change input button color if there is input
      HSVHue newBgColor, newHighlightColor;
      unsigned int newHighlightSaturation;
      newBgColor = rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonBgManual;
      unsigned int currentLedInput = pico_getCurrentInput();

      if (false == pico_getGswMode())
      { // Manual mode
        normalMode_scanAnimationReduceBlend(); //Keep reducing blend percentage for scan animation
        newHighlightColor = rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonActiveManual;   
        newHighlightSaturation = rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonActiveManual;
      } else {// Auto Mode
        newBgColor = rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonBgAuto;
        newHighlightColor = rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonActiveAuto;
        newHighlightSaturation =  rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonActiveAuto;

        // If there is input detected
        if (currentLedInput != NO_INPUT)
        {
          normalMode_scanAnimationReduceBlend(); //Keep reducing blend percentage
        } else {//Do auto mode & no input stuff here!
          normalMode_scanAnimationIncreaseBlend(); //Increase blend percentage to show the scan animation
        }
      }

      //Actually drawing the top row BG and Hightlight
      fill_solid( rgbLeds,
                  NUM_LEDS, 
                  CHSV( newBgColor,
                        rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonBg,
                        rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow(scanAnimationBlend, 2.0f))
                      ) 
                );
      if (currentLedInput > NO_INPUT)
      {
        rgbLeds[(int)currentLedInput - 1] = CHSV( newHighlightColor, newHighlightSaturation, rgbLedCurrentBrightness);
      }

      #ifdef RGBLED_DBG
        debugPort->println("[RGB LED] current blend: " + String(scanAnimationBlend));
      #endif

      //Mix in the scan animation
      for (int i = 0; i < NUM_LEDS; i++)
      {
        rgbLeds[i] += scanAnimationLedArray[i];
      }
      // End of Main color show
      //////////////////////////////////////////////////

      // mirror top to bottom row for better illumiation
      mirrorLedTopRowToBottomRow();

      // bottom row special colors go here
      if (PICO_POWER_STATE_AWAKE == pico_getPowerState())//false == rgbLedSleeping)
      {
        normalMode_drawLowerRowLed(newBgColor);
      }
      
    } else if (JSMARTSW_SETUP_MODE == rgbLed_operationMode)
    {
      rgbLeds[RGBLED_INPUT_5] = rgbLeds[RGBLED_INPUT_HD15] = CHSV(HUE_YELLOW, 255, 0);
      
      rgbLeds[RGBLED_INPUT_6] = rgbLeds[RGBLED_INPUT_SCART] = CHSV(rgbled_setupMode_colorScheme.hue_buttonYes, rgbled_setupMode_colorScheme.saturation_buttonYes, rgbLedCurrentBrightness);
      rgbLeds[RGBLED_INPUT_7] = rgbLeds[RGBLED_DIMMER] = CHSV(rgbled_setupMode_colorScheme.hue_buttonNo, rgbled_setupMode_colorScheme.saturation_buttonNo, rgbLedCurrentBrightness);
      rgbLeds[RGBLED_INPUT_8] = rgbLeds[RGBLED_AUTO] = CHSV(rgbled_setupMode_colorScheme.hue_buttonClr, rgbled_setupMode_colorScheme.saturation_buttonClr, rgbLedCurrentBrightness);

      setupMode_directionButtonsAnimation();
    }

    FastLED.show(); //finally show the latest led state      
    rgbLedLastRefresh = millis();
  }
}

////////////////////////////////////////
// Various functions
////////////////////////////////////////

static void normalMode_drawLowerRowLed(HSVHue currentBgColor)
{
  /*
    RGBLED_INPUT_1,
    RGBLED_INPUT_2,
    RGBLED_INPUT_3,
    RGBLED_INPUT_4,
    RGBLED_INPUT_5,
    RGBLED_INPUT_6,
    RGBLED_INPUT_7,
    RGBLED_INPUT_8,
    RGBLED_SHIFT,
    RGBLED_RT4K_PWR,
    RGBLED_RES_4K,
    RGBLED_RES_1080P,
    RGBLED_INPUT_HD15,
    RGBLED_INPUT_SCART,
    RGBLED_DIMMER,
    RGBLED_AUTO,
  */

  fastPulse = pulseAnimation(fastPulse);
  slowPulse = pulseAnimation(slowPulse);

  //shift button special coloring
  if (true == pico_isKeypadShiftOn())
  {
    rgbLeds[RGBLED_SHIFT] = CHSV(rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_shiftOn, rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_shiftOn, rgbLedCurrentBrightness);
  }

  //RT4K Power Action Animation (Fast)
  if (true == rt4k_attemptingPowerAction())
  { //power transition animation
    float targetBrightness = 0.0f;

    unsigned int tempRt4kConnection = rt4k_getConnection();
    if (RT4K_UART_PORT_TYPE_USBC == tempRt4kConnection)
    { // usb-c flash fast
      targetBrightness = rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow(fastPulse.percentage, 2.0f));
    } else if (RT4K_UART_PORT_TYPE_HD15 == tempRt4kConnection)
    { // hd15 flash black fast
      targetBrightness = 0.0f;
    }

    rgbLeds[RGBLED_RT4K_PWR] =  CHSV( currentBgColor  ,
                                      rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonBg,
                                      rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow( (ANIMATION_FULL_BLEND - fastPulse.percentage), 2.0f) ) 
                                    ) +
                                CHSV( rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_rt4kOn     ,
                                      rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_rt4kOn,
                                      targetBrightness
                                    );
  } else {
    //RT4K power state LED lighting (slow animation or solid)
    switch(rt4k_getPowerState())
    {
      case RT4K_ON:   //slow blink of ON color
        rgbLeds[RGBLED_RT4K_PWR] =  CHSV( currentBgColor  ,
                                          rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonBg,
                                          rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow( (ANIMATION_FULL_BLEND - slowPulse.percentage), 2.0f ) ) 
                                        ) +
                                    CHSV( rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_rt4kOn     ,
                                          rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_rt4kOn,
                                          rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow(slowPulse.percentage, 2.0f) ) 
                                        );
        break;
      case RT4K_OFF:  //slow blink of OFF color
        rgbLeds[RGBLED_RT4K_PWR] =  CHSV( currentBgColor  ,
                                          rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonBg,
                                          rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow( (ANIMATION_FULL_BLEND - slowPulse.percentage), 2.0f ) ) 
                                        ) +
                                    CHSV( rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_rt4kOff    ,
                                          rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_rt4kOff,
                                          rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow(slowPulse.percentage, 2.0f) ) 
                                        );
        break;
      case RT4K_UNKNOWN:  //solid bg color so you can't see
        break;
    }
  }

  //Auto mode indicator
  if (GSW_MODE_AUTO == pico_getGswMode())//(true == rt4k_auto)
  {
    rgbLeds[RGBLED_AUTO] =  CHSV( currentBgColor        ,
                                  rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonBg,
                                  rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow( (ANIMATION_FULL_BLEND - slowPulse.percentage), 2.0f ) ) 
                                ) +
                            CHSV( rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonActiveAuto,
                                  rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonActiveAuto,
                                  rgbLedCurrentBrightness * (ANIMATION_FULL_BLEND - pow(slowPulse.percentage, 2.0f)) 
                                );
  }
}

static pulse_animation_parameters pulseAnimation(pulse_animation_parameters pulse)
{
  pulse.percentage += pulse.stepping;

  if (pulse.percentage < ANIMATION_NO_BLEND)
  {
    pulse.percentage = ANIMATION_NO_BLEND;
    pulse.stepping = 0 - pulse.stepping;
  }

  if (pulse.percentage > ANIMATION_FULL_BLEND)
  {
    pulse.percentage = ANIMATION_FULL_BLEND;
    pulse.stepping = 0 - pulse.stepping;
  } 

  return pulse;
}

static void normalMode_scanAnimation()
{
  int animationBrightness = rgbLedCurrentBrightness;
  uint8_t scanColor;

  if (PICO_POWER_STATE_ASLEEP != pico_getPowerState())
  {
    fadeToBlackBy(scanAnimationLedArray, GSCARTSW_NUM_INPUTS, SCAN_FADE_STEPPING);
    scanColor = rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonBgAuto;
  } else {
    fadeToBlackBy(scanAnimationLedArray, GSCARTSW_NUM_INPUTS, SCAN_FADE_STEPPING_SLEEP);
    scanColor = rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_scanAnimationAsleep;
  }

  // change direction of scanning
  if ((scanAnimationPosition + scanStepping) > GSCARTSW_NUM_INPUTS - 1)
  {
    scanStepping = SCAN_STEPPING_LEFT;
  } else if ((scanAnimationPosition + scanStepping) < 0)
  {
    scanStepping = SCAN_STEPPING_RIGHT;
  }

  if (millis() - lastScanCycle > SCAN_ANIMATION_FRAME_TIME)
  {
    scanAnimationPosition += scanStepping;
    scanAnimationLedArray[scanAnimationPosition] = CHSV ( scanColor,
                                                          rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_scanAnimationAsleep,
                                                          animationBrightness * (ANIMATION_FULL_BLEND - pow((ANIMATION_FULL_BLEND - scanAnimationBlend), 2.0f))
                                                        );
    lastScanCycle = millis();
  }
}

static void normalMode_scanAnimationIncreaseBlend()
{
  if (scanAnimationBlend < ANIMATION_FULL_BLEND - RGBLED_BLEND_STEP_FAST)
  {
    scanAnimationBlend += RGBLED_BLEND_STEP_FAST;
  } else {
    scanAnimationBlend = ANIMATION_FULL_BLEND;
  }
}

static void normalMode_scanAnimationReduceBlend()
{
  if (scanAnimationBlend > RGBLED_BLEND_STEP_FAST)
  {
    scanAnimationBlend -= RGBLED_BLEND_STEP_FAST;
  } else {
    scanAnimationBlend = ANIMATION_NO_BLEND;
  }
}

static void mirrorLedTopRowToBottomRow()
{
  for (int i = 0; i < NUM_LEDS/2; i++)
  {
    rgbLeds[i + NUM_LEDS/2] = rgbLeds[i];
  } 
}

static void setupMode_directionButtonsAnimation()
{
  #define SETUPMODE_ANIMATION_BRIGHTNESS_LEFTRIGHT_STEP 0.02f
  #define SETUPMODE_ANIMATION_BRIGHTNESS_UPDOWN_STEP 0.05f

  // Left & Right
  static float leftRightBrightnessPercentage = 1.0f;
  static int sign = 1;

  rgbLeds[RGBLED_INPUT_1] = rgbLeds[RGBLED_SHIFT]     = CHSV(rgbled_setupMode_colorScheme.hue_buttonLeftRight, rgbled_setupMode_colorScheme.saturation_buttonLeftRight, leftRightBrightnessPercentage * rgbLedCurrentBrightness);
  rgbLeds[RGBLED_INPUT_2] = rgbLeds[RGBLED_RT4K_PWR]  = CHSV(rgbled_setupMode_colorScheme.hue_buttonLeftRight, rgbled_setupMode_colorScheme.saturation_buttonLeftRight, (1.0f - leftRightBrightnessPercentage) * rgbLedCurrentBrightness);
  
  leftRightBrightnessPercentage += sign * SETUPMODE_ANIMATION_BRIGHTNESS_LEFTRIGHT_STEP;

  if (leftRightBrightnessPercentage + sign * SETUPMODE_ANIMATION_BRIGHTNESS_LEFTRIGHT_STEP < SETUPMODE_ANIMATION_BRIGHTNESS_LEFTRIGHT_STEP)
  {
    leftRightBrightnessPercentage = 0.0f;
    sign = 1;
  } else if (leftRightBrightnessPercentage + sign * SETUPMODE_ANIMATION_BRIGHTNESS_LEFTRIGHT_STEP > (1 - SETUPMODE_ANIMATION_BRIGHTNESS_LEFTRIGHT_STEP))
  {
    leftRightBrightnessPercentage = 1.0f;
    sign = -1;
  }

  // Up & Down
  static int upDownPhase = 0;
  static float upDownBrightnessPercentage = 0.0f;

  if (0 == upDownPhase)
  {
    rgbLeds[RGBLED_RES_4K]  = rgbLeds[RGBLED_INPUT_4]   = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, upDownBrightnessPercentage * rgbLedCurrentBrightness);
    rgbLeds[RGBLED_INPUT_3] = rgbLeds[RGBLED_RES_1080P] = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, 0);
    upDownBrightnessPercentage += SETUPMODE_ANIMATION_BRIGHTNESS_UPDOWN_STEP;
  }

  if (1 == upDownPhase)
  {
    rgbLeds[RGBLED_RES_4K]  = rgbLeds[RGBLED_INPUT_4]   = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, rgbLedCurrentBrightness);
    rgbLeds[RGBLED_INPUT_3] = rgbLeds[RGBLED_RES_1080P] = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, upDownBrightnessPercentage * rgbLedCurrentBrightness);
    upDownBrightnessPercentage += SETUPMODE_ANIMATION_BRIGHTNESS_UPDOWN_STEP;
  }

  if (2 == upDownPhase)
  {
    rgbLeds[RGBLED_RES_4K]  = rgbLeds[RGBLED_INPUT_4]   = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, (1 - upDownBrightnessPercentage) * rgbLedCurrentBrightness);
    rgbLeds[RGBLED_INPUT_3] = rgbLeds[RGBLED_RES_1080P] = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, rgbLedCurrentBrightness);
    upDownBrightnessPercentage += SETUPMODE_ANIMATION_BRIGHTNESS_UPDOWN_STEP;
  }

  if (3 == upDownPhase)
  {
    rgbLeds[RGBLED_RES_4K]  = rgbLeds[RGBLED_INPUT_4]   = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, 0);
    rgbLeds[RGBLED_INPUT_3] = rgbLeds[RGBLED_RES_1080P] = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, (1 - upDownBrightnessPercentage) * rgbLedCurrentBrightness);
    upDownBrightnessPercentage += SETUPMODE_ANIMATION_BRIGHTNESS_UPDOWN_STEP;
  }

  if (4 == upDownPhase)
  {
    rgbLeds[RGBLED_RES_4K]  = rgbLeds[RGBLED_INPUT_4]   = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, 0);
    rgbLeds[RGBLED_INPUT_3] = rgbLeds[RGBLED_RES_1080P] = CHSV(rgbled_setupMode_colorScheme.hue_buttonUpDown, rgbled_setupMode_colorScheme.saturation_hue_buttonUpDown, 0);
    upDownBrightnessPercentage += SETUPMODE_ANIMATION_BRIGHTNESS_UPDOWN_STEP;
  }
  
  if (upDownBrightnessPercentage + SETUPMODE_ANIMATION_BRIGHTNESS_UPDOWN_STEP > (1 - SETUPMODE_ANIMATION_BRIGHTNESS_UPDOWN_STEP))
  {
    upDownBrightnessPercentage = 0.0f;

    if (++upDownPhase > 4)
    {
      upDownPhase = 0;
    }
  }
}

unsigned int rgbLed_cycleBrightness()
{
  if (++rgbLedBrightnessIndex > RGBLED_BRIGHTNESS_DATA_ARRAY_BOUNDARY)
  {
    rgbLedBrightnessIndex = RGBLED_BRIGHTNESS_LV1;
  }

  rgbLedBrightnessIndexStore = rgbLedBrightnessIndex;

  return rgbLedBrightnessIndexStore;
}

unsigned int rgbled_cycleColorScheme()
{
  if (++rgbledColorSchemeIndex > RGBLED_COLOR_SCHEME_ARRAY_BOUNDARY)
  {
    rgbledColorSchemeIndex = RGBLED_COLOR_SCHEME_DEFAULT;
  }

  return rgbledColorSchemeIndex;
}

////////////////////////////////////////
// Power saving
////////////////////////////////////////

void rgbLed_wakeup()
{
  //rgbLedSleeping = false;  
  rgbLedBrightnessIndex = rgbLedBrightnessIndexStore; // restore brightness recorded before sleeping
}

void rgbLed_goToSleep()
{
  // if currently awake
  if (PICO_POWER_STATE_AWAKE == pico_getPowerState())
  {
    rgbLedBrightnessIndexStore = rgbLedBrightnessIndex; // Save brightness before sleeping
  }

  //rgbLedSleeping = true;
  rgbLedBrightnessIndex = RGBLED_BRIGHTNESS_LV_SLEEP;
}

////////////////////////////////////////
// Boot & Misc
////////////////////////////////////////

void rgbLed_bootAnimation()
{ 
  //generate brightness offset values
  float brightnessOffset[NUM_LEDS_FIRST_ROW] = {-1, -0.66, -0.33, 0, 0, -0.33, -0.66, -1};

  for (int i = (int)RGBLED_MINIMUM_BRIGHTNESS; i < (int)rgbLedCurrentBrightness * 2; i += BOOT_ANIMATION_BRIGHTNESS_STEP)
  {
    for (int j = 0; j < NUM_LEDS_FIRST_ROW; j++)
    {
      int tempBrightness = (int)rgbLedCurrentBrightness * brightnessOffset[j] + i;

      if (tempBrightness < RGBLED_MINIMUM_BRIGHTNESS)
      {
        // Before reaching minimum brightness, stick to minimum brightness
        rgbLeds[j] = CHSV(rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonBgAuto, 
                          rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonBg,
                          RGBLED_MINIMUM_BRIGHTNESS);

      } else if (tempBrightness > (int)rgbLedCurrentBrightness)
      {
        // After hitting max brightness, stick to max brightness
        rgbLeds[j] = CHSV(rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonBgAuto,
                          rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonBg,
                          rgbLedCurrentBrightness);
      } else {
        // During transition
        rgbLeds[j] = CHSV(rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].hue_buttonBgAuto,
                          rgbLed_normalMode_colorScheme[rgbledColorSchemeIndex].saturation_buttonBg,
                          tempBrightness);
      }
    }

    mirrorLedTopRowToBottomRow();
    FastLED.show();
  }
}

void rgbLed_flashStuckButtons(int * buttonReading, int timesToBlink)
{
  //define bad button color
  CHSV badButtonColor;
  badButtonColor.hue = HUE_RED;
  badButtonColor.saturation = 255;
  badButtonColor.value = 250;

  for (int i = 0; i < timesToBlink; i++)
  {
    routine_blinkLed();

    for (int j = 0; j < BUTTON_COUNT; j++)
    {
      if (BUTTON_DOWN == buttonReading[j])
      {        
        badButtonColor.value = rgbLedBrightnessData[RGBLED_BRIGHTNESS_LEVEL_COUNT - 1].brightnessValue * (i & 0x1);
        rgbLeds[j] = rgbLeds[j + BUTTON_COUNT] = badButtonColor;
      } else {
        rgbLeds[j] = rgbLeds[j + BUTTON_COUNT] = CRGB::Black;
      }
    }
    FastLED.show();

    //Feed the dog while showing error
    //watchdog_update();
    delay(200);
  }
}

void rgbLed_clearSaveDataAnimation()
{
  fill_rainbow_circular(rgbLeds, NUM_LEDS_FIRST_ROW, HUE_RED);
  mirrorLedTopRowToBottomRow();
  FastLED.show();
}