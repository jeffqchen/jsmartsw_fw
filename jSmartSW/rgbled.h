#include "mydebug.h"

#include "crgb.h"
#include "chsv.h"

#include "rgbled_colorscheme.h"
#include "gscartsw.h"
#include "rt4k.h"

#ifndef RGBLED_H
#define RGBLED_H

//Neopixel
#define RGBLED_TYPE   WS2811
#define COLOR_ORDER   GRB
#define RGBLED_FRAMES_PER_SECOND   120
#define RGBLED_FRAME_TIME          1000/RGBLED_FRAMES_PER_SECOND
#define RGBLED_MINIMUM_BRIGHTNESS  0
#define RGBLED_MAXIMUM_BRIGHTNESS  255

//Color Scheme
typedef struct
{
  HSVHue hue_buttonBgManual;
  HSVHue hue_buttonBgAuto;
  unsigned int saturation_buttonBg;

  HSVHue hue_buttonActiveManual;
  unsigned int saturation_buttonActiveManual;
  HSVHue hue_buttonActiveAuto;
  unsigned int saturation_buttonActiveAuto;

  // function colors
  HSVHue hue_shiftOn;
  unsigned int saturation_shiftOn;

  HSVHue hue_rt4kOn;
  unsigned int saturation_rt4kOn;
  HSVHue hue_rt4kOff;
  unsigned int saturation_rt4kOff;
  HSVHue hue_rt4kUnknown;
  unsigned int saturation_rt4kUnknown;

  HSVHue hue_scanAnimationAsleep;
  unsigned int saturation_scanAnimationAsleep;
} rgbled_normalMode_color_scheme;

typedef struct
{
  HSVHue hue_buttonLeftRight;
  unsigned int saturation_buttonLeftRight;
  
  HSVHue hue_buttonUpDown;
  unsigned int saturation_hue_buttonUpDown;  

  // function colors
  HSVHue hue_buttonYes;
  unsigned int saturation_buttonYes;

  HSVHue hue_buttonNo;
  unsigned int saturation_buttonNo;

  HSVHue hue_buttonClr;
  unsigned int saturation_buttonClr;
} rgbled_setupMode_color_scheme;

//end of color scheme

enum {
  RGBLED_BRIGHTNESS_LV1_VALUE = 100,
  RGBLED_BRIGHTNESS_LV2_VALUE = 180,
  RGBLED_BRIGHTNESS_LV3_VALUE = 250
};

enum {
  JLCPCB_FR4_COLOR_CORRECTION_1 = 0x9FFFFF,
  JLCPCB_FR4_COLOR_CORRECTION_2 = 0x7FDFFF,
  JLCPCB_FR4_COLOR_CORRECTION_3 = 0x70DFFF,
};

enum {
  RGBLED_BRIGHTNESS_LV1 = 0,
  RGBLED_BRIGHTNESS_LV2,
  RGBLED_BRIGHTNESS_LV3,
  RGBLED_BRIGHTNESS_LV_END
};
#define RGBLED_BRIGHTNESS_LEVEL_COUNT   RGBLED_BRIGHTNESS_LV_END

#define RGBLED_BRIGHTNESS_LV_DEFAULT  RGBLED_BRIGHTNESS_LV2;
#define RGBLED_BRIGHTNESS_LV_SLEEP    RGBLED_BRIGHTNESS_LV1;


#define RGBLED_COLOR_CORRECTION

enum {
  #ifdef RGBLED_COLOR_CORRECTION
    RGBLED_BRIGHTNESS_LV1_CORRECTION = JLCPCB_FR4_COLOR_CORRECTION_1,
    RGBLED_BRIGHTNESS_LV2_CORRECTION = JLCPCB_FR4_COLOR_CORRECTION_2,
    RGBLED_BRIGHTNESS_LV3_CORRECTION = JLCPCB_FR4_COLOR_CORRECTION_3
  #else
    RGBLED_BRIGHTNESS_LV1_CORRECTION = 0xFFFFFF,
    RGBLED_BRIGHTNESS_LV2_CORRECTION = 0xFFFFFF,
    RGBLED_BRIGHTNESS_LV3_CORRECTION = 0xFFFFFF
  #endif
};

typedef struct
{
  unsigned int brightnessValue;
  CRGB correctionValue;
} rgbLed_brightness_data;

#define RGBLED_BRIGHTNESS_DATA_ARRAY    { {RGBLED_BRIGHTNESS_LV1_VALUE, RGBLED_BRIGHTNESS_LV1_CORRECTION},  \
                                          {RGBLED_BRIGHTNESS_LV2_VALUE, RGBLED_BRIGHTNESS_LV2_CORRECTION},  \
                                          {RGBLED_BRIGHTNESS_LV3_VALUE, RGBLED_BRIGHTNESS_LV3_CORRECTION}   }

#define RGBLED_BRIGHTNESS_DATA_ARRAY_BOUNDARY   (RGBLED_BRIGHTNESS_LEVEL_COUNT - 1)
#define RGBLED_BRIGHTNESS_SLEEPING      RGBLED_BRIGHTNESS_LV1
#define RGBLED_BRIGHTNESS_STEPPING 2

//animation
////Boot animation
#define BOOT_ANIMATION_BRIGHTNESS_STEP  2

#define ANIMATION_NO_BLEND      0.00f
#define ANIMATION_FULL_BLEND    1.00f
#define RGBLED_BLEND_STEP_FAST  0.1f
#define RGBLED_BLEND_STEP_SLOW  0.01f

//Scan animation related
#define SCAN_ANIMATION_FRAME_TIME   1500/(GSCARTSW_NUM_INPUTS)
#define SCAN_STEPPING_RIGHT         1
#define SCAN_STEPPING_LEFT          -1
#define SCAN_FADE_STEPPING          3
#define SCAN_FADE_STEPPING_SLEEP    1

typedef struct {
  float percentage;
  float stepping;
} pulse_animation_parameters;

#endif

////////////////////////////////////////
// Initialization
////////////////////////////////////////

void rgbLed_init();

////////////////////////////////////////
// Routine
////////////////////////////////////////

void routine_rgbLedUpdate();

unsigned int rgbLed_cycleBrightness();
unsigned int rgbled_cycleColorScheme();

////////////////////////////////////////
// Power saving
////////////////////////////////////////

void rgbLed_wakeup();
void rgbLed_goToSleep();

////////////////////////////////////////
// Boot & Misc
////////////////////////////////////////

void rgbLed_bootAnimation();
void rgbLed_flashStuckButtons(int * , int);
void rgbLed_clearSaveDataAnimation();