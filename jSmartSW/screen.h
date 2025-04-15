#include "mydebug.h"
#include "bitmaps.h"
#include <Adafruit_SSD1306.h>

#ifndef SCREEN_H
#define SCREEN_H

// SSD1306 OLED Screen Setup
#define OLED_RESET      -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_REFRESH_PERIOD   (unsigned long)17 // 17ms ~60fps

#define BOOT_DISPLAY_DURATION   (unsigned long)600
#define SCREEN_SAVING_PERIOD1   (unsigned long)5000   //seconds to dim
#define SCREEN_SAVING_PERIOD2   SCREEN_SAVING_PERIOD1 + (unsigned long)5000  //seconds to turn off

#define SCREEN_CHAR_COUNT_HORIZONTAL          6
#define SCREEN_CHAR_SIZE_1_WIDTH              5
#define SCREEN_CHAR_SIZE_1_PITCH              (SCREEN_CHAR_SIZE_1_WIDTH + 1)
#define SCREEN_CHAR_SIZE_1_HEIGHT             7
#define SCREEN_TEXT_SCROLL_STEP_F             0.3f

#define SCREEN_ELEMENT_VERTICAL_SEPARATION    2
#define SCREEN_CHAR_VERTICAL_SEPARATION       2

#define SCREEN_COMMAND_SHIFT_DISPLAY_DURATION (unsigned long)100
#define SCREEN_COMMAND_TEXT_DISPLAY_DURATION  (unsigned long)500

#define CONTROL_INTERFACE_DISPLAY_DURATION    (unsigned long)5000
#define SETUPMODE_MESSAGE_DISPLAY_DURATION    (unsigned long)2000

#define SCREEN_INPUT_ROTATION_STEP      3
#define SETUPMODE_ICON_ROTATION_STEP_X  SCREEN_INPUT_ROTATION_STEP
#define SETUPMODE_ICON_ROTATION_STEP_Y  SCREEN_INPUT_ROTATION_STEP

enum
{
  INPUT_NUMBER_IS_BIG,
  INPUT_NUMBER_IS_SMALL
};  //input number display size

enum
{
  ROTATE_UP     = -2,
  ROTATE_LEFT   = -1,
  ROTATE_NONE   =  0,
  ROTATE_RIGHT  =  1,
  ROTATE_DOWN   =  2,
}; //input number rotation direction

enum
{
  INPUT_NUMBER_TYPE_BIG,
  INPUT_NUMBER_TYPE_MINI,
  INPUT_NUMBER_TYPE_ICON
}; //element types

typedef struct bitmap_element_entity{
  int posX;
  int posY;
  
  int bitmapWidth;
  int bitmapHeight;
  const uint8_t* bitmapData;

  uint16_t color;

  bitmap_element_entity()
  {
    bitmapData = NULL;
    posX = posY = bitmapWidth = bitmapHeight = 0;
    color = SSD1306_WHITE;
  }

  bitmap_element_entity(int newPosX, int newPosY, int newWidth, int newHeight, const uint8_t* newDataPointer, uint16_t newColor)
  {
    posX = newPosX;
    posY = newPosY;
    
    bitmapWidth = newWidth;
    bitmapHeight = newHeight;

    bitmapData = newDataPointer;

    color = newColor;
  }
} bitmap_element_entity;

typedef struct input_icon_set {
  const uint8_t * bitmapSet;
  const unsigned int elementCount;
  const String setName;
} input_icon_set;

////////////////////////////////////////////////////
// Screen Composition
////////////////////////////////////////////////////

////////////////////////////////////////////////////
// Normal Mode
////////////////////////////////////////////////////
////Scrolling Banner Section
#define MODE_BANNER_START_POS_X  0
#define MODE_BANNER_START_POS_Y  0
#define MODE_BANNER_START_OFFSET_X  0
#define MODE_BANNER_START_OFFSET_Y  3

#define BANNER_SCROLL_SPEED_FACTOR_AUTO   2
#define BANNER_SCROLL_SPEED_FACTOR_MANUAL 3

////Text Command Section
#define SCREEN_COMMAND_TEXT_POS_Y  MODE_BANNER_BG_HEIGHT + 3

////Big Input Number
#define INPUT_NUMBER_BIG_POS_X  (SCREEN_WIDTH - INPUT_NUMBER_BIG_BITMAP_WIDTH)/2
#define INPUT_NUMBER_BIG_POS_Y  35

////Small Input Number & Custom Icon
#define INPUT_NUMBER_MINI_POS_X (SCREEN_WIDTH - INPUT_NUMBER_MINI_BITMAP_WIDTH)/2
#define INPUT_NUMBER_MINI_POS_Y (SCREEN_COMMAND_TEXT_POS_Y + 10)
#define INPUT_ICON_POS_X        (SCREEN_WIDTH - INPUT_ICON_WIDTH)/2
#define INPUT_ICON_POS_Y        (INPUT_NUMBER_MINI_POS_Y + INPUT_NUMBER_MINI_BITMAP_HEIGHT + SCREEN_ELEMENT_VERTICAL_SEPARATION)
#define INPUT_ICON_OFFSET_X     (-1)
#define INPUT_ICON_OFFSET_Y     (1)

////Mascot Section
#define HEART_POS_X   12
#define HEART_POS_Y   106

#define MASCOT_SEPARATION_POS_Y     17
#define MASCOT_SEPARATION_PIX_COUNT 1
#define MASCOT_BANNER_POS_Y         111

#define TINKY_EXCLAMATION_X   7
#define TINKY_EXCLAMATION_Y   101

////////////////////////////////////////////////////
// Setup Mode
////////////////////////////////////////////////////

#define SETUPMODE_STRIP_TOP_POS_X           0
#define SETUPMODE_STRIP_TOP_POS_Y           0

#define SETUPMODE_INPUT_NUMBER_MINI_POS_X   INPUT_NUMBER_MINI_POS_X
#define SETUPMODE_INPUT_NUMBER_MINI_POS_Y   (SETUPMODE_STRIP_TOP_POS_Y + SETUPMODE_STRIPE_HEIGHT + SCREEN_ELEMENT_VERTICAL_SEPARATION)

#define SETUPMODE_CATEGORY_NAME_TEXT_POS_Y  (SETUPMODE_INPUT_NUMBER_MINI_POS_Y + INPUT_NUMBER_MINI_BITMAP_HEIGHT + SCREEN_ELEMENT_VERTICAL_SEPARATION)

#define SETUPMODE_INPUT_ICON_POS_X          INPUT_ICON_POS_X
#define SETUPMODE_INPUT_ICON_POS_Y          (SETUPMODE_CATEGORY_NAME_TEXT_POS_Y + 12)

#define SETUPMODE_ARROW_VERTICAL_POS_X      ((SCREEN_WIDTH - SETUPMODE_ARROW_WIDTH) / 2)
#define SETUPMODE_ARROW_UP_POS_Y            (SETUPMODE_INPUT_ICON_POS_Y)
#define SETUPMODE_ARROW_DOWN_POS_Y          (SETUPMODE_INPUT_ICON_POS_Y + INPUT_ICON_HEIGHT - SETUPMODE_ARROW_HEIGHT)

#define SETUPMODE_ARROW_HORIZONTAL_Y        (SETUPMODE_INPUT_ICON_POS_Y + (INPUT_ICON_HEIGHT - SETUPMODE_ARROW_HEIGHT) / 2)
#define SETUPMODE_ARROW_LEFT_POS_X          0
#define SETUPMODE_ARROW_RIGHT_POS_X         (SCREEN_WIDTH - SETUPMODE_ARROW_WIDTH)

#define SETUPMODE_TOOLTIP_POS_X             0
#define SETUPMODE_TOOLTIP_CLEAR_POS_Y       (SCREEN_HEIGHT                  - SCREEN_CHAR_SIZE_1_HEIGHT - SCREEN_CHAR_VERTICAL_SEPARATION)
#define SETUPMODE_TOOLTIP_NO_POS_Y          (SETUPMODE_TOOLTIP_CLEAR_POS_Y  - SCREEN_CHAR_SIZE_1_HEIGHT - SCREEN_CHAR_VERTICAL_SEPARATION)
#define SETUPMODE_TOOLTIP_YES_POS_Y         (SETUPMODE_TOOLTIP_NO_POS_Y     - SCREEN_CHAR_SIZE_1_HEIGHT - SCREEN_CHAR_VERTICAL_SEPARATION)

#define SETUPMODE_STRIP_BOTTOM_POS_X        0
#define SETUPMODE_STRIP_BOTTOM_POS_Y        (SCREEN_HEIGHT - SETUPMODE_STRIPE_HEIGHT)

////////////////////////////////////////////////////
// Other
////////////////////////////////////////////////////

#define REBOOT_SPLASH_POS_X  0
#define REBOOT_SPLASH_POS_Y  45

////Boot animation
#define BOOT_PANEL_TOP_POS_X  0
#define BOOT_PANEL_TOP_POS_Y  0
#define BOOT_PANEL_BOTTOM_POS_X  0
#define BOOT_PANEL_BOTTOM_POS_Y  BOOT_PANEL_TOP_POS_Y

#define NAMEPLATE_POS_X   0
#define NAMEPLATE_POS_Y   0

#endif

//initi
void screen_init();

/////////////////////////////
//Main routine
void routine_screenUpdate();      //Loop routine for updating screen
/////////////////////////////

//external functions
bool screen_isThereActivity();

////various parts of gfx
void screen_setCommandTextUpdate(String, unsigned long);

//// energy saver
void screen_dimming();
void screen_wakeUp();
void screen_turnOff();

//// special screens
void screen_rebootScreen();
void screen_buttonErrorRebootMessage();
void screen_clearSaveDataMessage();

//// Setup Mode
void screen_setupMode_iconSetCycleUp();
void screen_setupMode_iconSetCycleDown();
void screen_setupMode_iconIdCycleUp();
void screen_setupMode_iconIdCycleDown();
void screen_setupMode_confirmChoice();
void screen_setupMode_resetToDefault();

void screen_modeTransitionAnimation();

// animations
////Boot animations
void screen_bootSplash();  
void screen_bootAnimationA();  
void screen_bootAnimationB();