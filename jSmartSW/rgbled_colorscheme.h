/////////////////////////////////////////////////////////////////////////////////////////////////////////
//Color Scheme
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Add scheme name, data and then add data entry into the array area to take effect.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Color Scheme Reference
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
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SPRING_PINK (HSVHue)(HUE_PINK + 20)
/////////////////////////////////////////////////////////////////////////////////////////////////////////
enum {
  RGBLED_COLOR_SCHEME_DEFAULT = 0,  //Boeing / Summer
  RGBLED_COLOR_SCHEME_INTERGALATIC, //Autumn
  RGBLED_COLOR_SCHEME_SPRING,       //Spring
  RGBLED_COLOR_SCHEME_WINTER,       //Winter
  RGBLED_COLOR_SCHEME_END
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RGBLED_COLOR_SCHEME_DEFAULT_DATA        { /* Default Color Scheme */                          \
                                                  HUE_YELLOW, HUE_YELLOW, 255,                        \
                                                  HUE_GREEN,  255,  HUE_BLUE, 255,                    \
                                                  HUE_ORANGE, 255,                                    \
                                                  HUE_GREEN,  255,  HUE_RED,  255,  HUE_YELLOW, 255,  \
                                                  HUE_RED,    255                                     }
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RGBLED_COLOR_SCHEME_INTERGALATIC_DATA   { /* Intergalatic Color Scheme */                     \
                                                  HUE_ORANGE, HUE_ORANGE, 255,                        \
                                                  HUE_GREEN,  128,  HUE_YELLOW, 255,                  \
                                                  HUE_ORANGE, 255,                                    \
                                                  HUE_GREEN,  255,  HUE_RED,  255,  HUE_YELLOW, 255,  \
                                                  HUE_RED,    0                                       }
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RGBLED_COLOR_SCHEME_SPRING_DATA         { /* Spring Color Scheme */                           \
                                                  HUE_GREEN,  HUE_GREEN,  128,                        \
                                                  HUE_BLUE,   0, SPRING_PINK,  115,                   \
                                                  HUE_YELLOW,   200,                                  \
                                                  SPRING_PINK,  190,  HUE_RED, 128, HUE_GREEN,  128,  \
                                                  SPRING_PINK,  115                                   }                                         
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RGBLED_COLOR_SCHEME_WINTER_DATA         { /* Winter Color Scheme */                           \
                                                  HUE_BLUE,   HUE_BLUE,  20,                           \
                                                  HUE_RED,    128, HUE_BLUE,  128,                    \
                                                  HUE_GREEN,    128,                                  \
                                                  HUE_BLUE,     128,  HUE_RED, 128, HUE_BLUE,  0,     \
                                                  SPRING_PINK,  0                                     }                                         
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RGBLED_COLOR_SCHEME_ARRAY { RGBLED_COLOR_SCHEME_DEFAULT_DATA,       \
                                    RGBLED_COLOR_SCHEME_INTERGALATIC_DATA,  \
                                    RGBLED_COLOR_SCHEME_WINTER_DATA,        \
                                    RGBLED_COLOR_SCHEME_SPRING_DATA         }
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RGBLED_COLR_SCHEME_COUNT    RGBLED_COLOR_SCHEME_END
#define RGBLED_COLOR_SCHEME_ARRAY_BOUNDARY    (RGBLED_COLR_SCHEME_COUNT - 1)