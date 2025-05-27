#include "mydebug.h"
#include "gsw_hardware.h"

#include <Adafruit_SSD1306.h>

#include "mypico.h"
#include "gscartsw.h"
#include "rt4k.h"

#include "screen.h"
#include "usb_controller.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
  #ifdef SCREEN_TIMING_DBG
    unsigned long funcTime = 0;
    unsigned long loopTime = 0;
    String screenTimingMsg = "";
  #endif
#endif

//input icon related data
#ifdef INJECT_FAKE_DATA
  input_number_spec fakeInputIconData[INPUT_END] = \
  {
    { INPUT_NUMBER_IS_BIG, 0, 0},    //? is always big
    ///////////////////////////
    { INPUT_NUMBER_IS_BIG,    0,                  0                       },  //1
    { INPUT_NUMBER_IS_BIG,    0,                  0                       },  //2
    { INPUT_NUMBER_IS_SMALL,  ICON_SET_NINTENDO,  NINTENDO_SUPER_FAMICOM  },  //3
    { INPUT_NUMBER_IS_BIG,    0,                  0                       },  //4
    { INPUT_NUMBER_IS_BIG,    0,                  0                       },  //5
    { INPUT_NUMBER_IS_SMALL,  ICON_SET_NINTENDO,  NINTENDO_FAMICOM        },  //6
    { INPUT_NUMBER_IS_BIG,    0,                  0                       },  //7
    { INPUT_NUMBER_IS_BIG,    0,                  0                       },  //8
  };
#endif

static unsigned int  screenOrientation;
static unsigned long bootTime = 0;

//static unsigned long lastScreenRefresh = 0;
static bool isScreenDim = false;
static bool isScreenOff = false;
static int frameCounter = 0;

static Adafruit_SSD1306 bufferA(SCREEN_WIDTH_PHYSICAL, SCREEN_HEIGHT_PHYSICAL, &Wire, OLED_RESET);
/*
Adafruit_SSD1306 bufferB(SCREEN_WIDTH_PHYSICAL, SCREEN_HEIGHT_PHYSICAL, &Wire, OLED_RESET);
Adafruit_SSD1306 * buffers[2];
*/
static Adafruit_SSD1306 * display;
//static unsigned int bufferSelection = 0;  // double buffer

static bool screenImportantActivity = false;

//Various screen component states
static unsigned int commandTextIndex = 0; //default state = no command indicated

static unsigned int screenCurrentDisplayedInput = NO_INPUT;
static unsigned long screenCommandTextDisplayTime = 0;
static unsigned long screenCommandTextDisplayDuration = SCREEN_COMMAND_TEXT_DISPLAY_DURATION;
static String screenCommandText = " ";

static unsigned int tinkySelect = TINKY_EAR_DOWN;

static input_number_spec* inputNumberCustomizationData;
extern jsmartsw_savedata mainSaveData;

input_icon_set inputIconBitmaps[ICON_SET_END] = ICON_SET_DATA_COLLECTION;

// Setup Mode
static int screen_operationMode = JSMARTSW_NORMAL_MODE;
static unsigned int setupMode_inputToCustomize = NO_INPUT;
static int setupMode_currentCategoryId = 0;
static int setupMode_currentIconId = 0;
static bitmap_element_entity setupMode_currentInputIcon;
static int setupMode_iconScrollDirection = ROTATE_NONE;
static int targetCategoryId , targetIconId;
static int setupMode_transitionPanel_posX = SETUPMODE_TRANSITION_PANEL_POSITION_END;

/////////////// FUNCTION PROTOTYPES ////////////////
  //// Normal Mode screen components to display
  static void normalMode_screenComponent_modeBanner();
  static void normalMode_screenComponent_controlInterface();
  static void normalMode_screenComponent_buttonCommand();
  static void normalMode_screenComponent_mascots();
  static void normalMode_screen_inputNumberOrRotationAnimation(); //Input number rotation animation
  static void displayTextStaticCenteredOrScrolling(String, int*, int);

  //// Setup Mode screen components to display
  static void setupMode_init();

  static void setupMode_screenComponent_inputNumber();
  static void setupMode_screenComponent_iconCategoryText();
  static void setupMode_screenComponent_inputIcon();
  static void setupMode_screenComponent_stripes();
  static void setupMode_screenComponent_toolTip();

  static void drawScreenElement(bitmap_element_entity);
  static bool isInputNumberBig(unsigned int);
  static const uint8_t* inputNumberToIconDataPointer(unsigned int);
  static bool modeChangeTriggered();
  static bool transitionHasFinished();
  static void transitionReset();
//////////// END OF FUNCTION PROTOTYPES ////////////

void screen_init()
{
  inputNumberCustomizationData = pico_getIconCustomizationDataPointer();
  screenOrientation = pico_getScreenOrientation();
  
  //Initialize display
  bufferA.setRotation(screenOrientation);
  bufferA.ssd1306_command(SSD1306_SETCONTRAST);
  bufferA.ssd1306_command(0xff);
  bufferA.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  /* Double buffer
    bufferB.setRotation(screenOrientation);
    bufferB.ssd1306_command(SSD1306_SETCONTRAST);
    bufferB.ssd1306_command(0xff);
    bufferB.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

    buffers[0] = &bufferA;
    buffers[1] = &bufferB;
  */
  
  display = &bufferA;
}

void routine_screenUpdate()
{
  #ifdef SCREEN_TIMING_DBG
    String screenTimingMsg = "";
    loopTime = millis();
  #endif

  /* Double buffer
    {
      #ifdef SCREEN_TIMING_DBG
        String screenAddressTemp = String((int)(display->getBuffer()), HEX);
        screenAddressTemp.toUpperCase();
        screenTimingMsg += "<< Displaying >> 0x" + screenAddressTemp + "\tDraw finished: " + (drawFinished? "Yes":"No") + "\t";
      #endif
    }

    display->display();

    #ifdef SCREEN_TIMING_DBG
      screenTimingMsg += "[DISPLAY] " + String(millis() - funcTime) + "\t";
      funcTime = millis();
    #endif
    

    bufferSelection++;
    if (bufferSelection > 1)
    {
      bufferSelection = 0;
    }

    display = buffers[bufferSelection];
    

    {
      #ifdef SCREEN_TIMING_DBG
        String screenAddressTemp = String((int)(display->getBuffer()), HEX);
        screenAddressTemp.toUpperCase();
        screenTimingMsg += "<< Drawing to >> 0x" + screenAddressTemp + "\t";
      #endif
    }

    drawFinished = false;
  */

  //Update main program mode before current frame
  screen_operationMode = pico_getMainProgramMode();

  //////////////////////////////
  // Routine updates
  //////////////////////////////

  display->clearDisplay();

  //assume no activity by default, components will change this value if something is happening
  screenImportantActivity = false;
  
  #ifdef SCREEN_TIMING_DBG
    funcTime = millis();
  #endif

  //write current mode banner bitmap data into buffer
  normalMode_screenComponent_modeBanner();    

  #ifdef SCREEN_TIMING_DBG
    screenTimingMsg += "[Banner] " + String(millis() - funcTime) + "\t";
    funcTime = millis();
  #endif

  //write button command when any button or button combination is trigged
  normalMode_screenComponent_buttonCommand();

  #ifdef SCREEN_TIMING_DBG
    screenTimingMsg += "[Command] " + String(millis() - funcTime) + "\t";
    funcTime = millis();
  #endif
  
  //write current input number bitmap data into buffer
  normalMode_screen_inputNumberOrRotationAnimation();

  #ifdef SCREEN_TIMING_DBG
    screenTimingMsg += "[Input] " + String(millis() - funcTime) + "\t";
    funcTime = millis();
  #endif

  //rt4k mascots
  normalMode_screenComponent_mascots();

  //USB-C connection indicator
  normalMode_screenComponent_controlInterface();

  #ifdef SCREEN_TIMING_DBG
    screenTimingMsg += "[Mascot] " + String(millis() - funcTime) + "\t";
    funcTime = millis();
  #endif

  if (JSMARTSW_SETUP_MODE         == screen_operationMode ||
      JSMARTSW_EXITING_SETUP_MODE == screen_operationMode)
  {
    display->fillRect ( 0, 0,
                        SCREEN_WIDTH, SCREEN_HEIGHT,
                        SSD1306_BLACK);
    setupMode_screenComponent_inputIcon();    
    setupMode_screenComponent_inputNumber();
    setupMode_screenComponent_iconCategoryText();
    setupMode_screenComponent_stripes();
    setupMode_screenComponent_toolTip();
  }

  //////////////////////////////
  // Occassional high priority updates
  // Like mode change animations
  //////////////////////////////

  screen_modeTransitionAnimation();

  if (JSMARTSW_ENTERING_SETUP_MODE == screen_operationMode)
  { 
    //reset panel position, triggering transition animation
    if (transitionHasFinished())
    {
      transitionReset();
    }

    if (modeChangeTriggered())
    {
      setupMode_init();
      pico_setMainProgramMode(JSMARTSW_SETUP_MODE);
    }
  }

  if (JSMARTSW_EXITING_SETUP_MODE == screen_operationMode)
  {
    //reset panel position, triggering transition animation
    if (transitionHasFinished())
    {
      transitionReset();
    }

    if (modeChangeTriggered())
    {
      pico_setMainProgramMode(JSMARTSW_NORMAL_MODE);
    }
    //
  }
  
  if (JSMARTSW_POST_BOOT == screen_operationMode)
  {
    //Boot animation sequence continued
    screen_bootAnimationB();
  }
  
  display->display();

  frameCounter++;

  #ifdef SCREEN_TIMING_DBG
    screenTimingMsg += "<<Displaying>> " + String(millis() - funcTime) + "\t";
  #endif
  //////////////////////////////
  // Screen update ends
  //////////////////////////////
  
  //Double buffer
  //drawFinished = true;
  
  //lastScreenRefresh = millis();

  #ifdef SCREEN_TIMING_DBG
    screenTimingMsg += "<< Loop >> " + String(millis() - loopTime);
    debugPort->println(screenTimingMsg);
  #endif
}

void screen_dimming()
{
  if (false == isScreenDim)
  {
    display->ssd1306_command(SSD1306_SETCONTRAST);
    display->ssd1306_command(0x10);
    display->dim(true);
    isScreenDim = true;
  }

}

void screen_turnOff()
{
  if (false == isScreenOff)
  {
    display->ssd1306_command(SSD1306_DISPLAYOFF);
    isScreenOff = true;
  }
}

void screen_wakeUp()
{
  if (true == isScreenDim)
  {
    display->ssd1306_command(SSD1306_SETCONTRAST);
    display->ssd1306_command(0xff);
    display->dim(false);
    isScreenDim = false;
  }

  if (true == isScreenOff)
  {
    display->ssd1306_command(SSD1306_DISPLAYON);
    isScreenOff = false;
  }
}

//////////////////////////////////
// boot visuals
//////////////////////////////////
void screen_bootSplash()
{
  display->clearDisplay();
  //show nameplate
  display->drawBitmap( NAMEPLATE_POS_X, NAMEPLATE_POS_Y,
                      bitmap_nameplate,
                      NAMEPLATE_WIDTH, NAMEPLATE_HEIGHT,
                      SSD1306_WHITE);
  display->display();
}

void screen_bootAnimationA()
{
  static int bootPanelYOffset = 0;
  //bootPanelYOffset = 0;

  while(bootPanelYOffset < 70)
  {
    display->clearDisplay();

    //nameplate
    display->drawBitmap(NAMEPLATE_POS_X, NAMEPLATE_POS_Y,
                        bitmap_nameplate,
                        NAMEPLATE_WIDTH, NAMEPLATE_HEIGHT,
                        SSD1306_WHITE);

    //panels
    //draw underlying black backing first
    display->drawBitmap( BOOT_PANEL_TOP_POS_X, -3 - BOOT_PANEL_TOP_HEIGHT + bootPanelYOffset,
                        bitmap_bootPanelTop[1],
                        BOOT_PANEL_TOP_WIDTH, BOOT_PANEL_TOP_HEIGHT,
                        SSD1306_BLACK);
    display->drawBitmap( BOOT_PANEL_BOTTOM_POS_X, (3 * BOOT_PANEL_TOP_HEIGHT - BOOT_PANEL_BOTTOM_HEIGHT) - bootPanelYOffset - 1,
                        bitmap_bootPanelBottom[1],
                        BOOT_PANEL_BOTTOM_WIDTH, BOOT_PANEL_BOTTOM_HEIGHT,
                        SSD1306_BLACK);
    //draw the actual panels
    display->drawBitmap( BOOT_PANEL_TOP_POS_X, -3 - BOOT_PANEL_TOP_HEIGHT + bootPanelYOffset,
                        bitmap_bootPanelTop[0],
                        BOOT_PANEL_TOP_WIDTH, BOOT_PANEL_TOP_HEIGHT,
                        SSD1306_WHITE);
    display->drawBitmap( BOOT_PANEL_BOTTOM_POS_X, (3 * BOOT_PANEL_TOP_HEIGHT - BOOT_PANEL_BOTTOM_HEIGHT) - bootPanelYOffset - 1,
                        bitmap_bootPanelBottom[0],
                        BOOT_PANEL_BOTTOM_WIDTH, BOOT_PANEL_BOTTOM_HEIGHT,
                        SSD1306_WHITE);

    
    if (bootPanelYOffset < 25)
    {bootPanelYOffset += 16;} 
    else if (bootPanelYOffset < 40)
    {bootPanelYOffset += 12;} 
    else if (bootPanelYOffset < 50)
    {bootPanelYOffset += 8;}
    else if (bootPanelYOffset < 62)
    {bootPanelYOffset += 4;}
    else
    {bootPanelYOffset += 1;}
    display->display();
  }

  bootPanelYOffset = 0;

  delay(BOOT_DISPLAY_DURATION);
  bootTime = millis(); //record boot time for animation part 2

  //pico_setMainProgramMode(JSMARTSW_POST_BOOT);
}

void screen_bootAnimationB()
{
  static int bootPanelYOffset = 0;

  if (BOOT_PANEL_BOTTOM_HEIGHT - bootPanelYOffset > -1)
  {
    //draw underlying black backing first
    display->drawBitmap( BOOT_PANEL_TOP_POS_X, BOOT_PANEL_TOP_POS_Y - bootPanelYOffset,
                        bitmap_bootPanelTop[1],
                        BOOT_PANEL_TOP_WIDTH, BOOT_PANEL_TOP_HEIGHT,
                        SSD1306_BLACK);
    display->drawBitmap( BOOT_PANEL_BOTTOM_POS_X, BOOT_PANEL_TOP_HEIGHT + bootPanelYOffset + 1,
                        bitmap_bootPanelBottom[1],
                        BOOT_PANEL_BOTTOM_WIDTH, BOOT_PANEL_BOTTOM_HEIGHT,
                        SSD1306_BLACK);

    //draw the actual panels
    display->drawBitmap( BOOT_PANEL_TOP_POS_X, BOOT_PANEL_TOP_POS_Y - bootPanelYOffset,
                        bitmap_bootPanelTop[0],
                        BOOT_PANEL_TOP_WIDTH, BOOT_PANEL_TOP_HEIGHT,
                        SSD1306_WHITE);
    display->drawBitmap( BOOT_PANEL_BOTTOM_POS_X, BOOT_PANEL_TOP_HEIGHT + bootPanelYOffset + 1,
                        bitmap_bootPanelBottom[0],
                        BOOT_PANEL_BOTTOM_WIDTH, BOOT_PANEL_BOTTOM_HEIGHT,
                        SSD1306_WHITE);

    
    if (bootPanelYOffset < 25)
    {bootPanelYOffset += 7;} 
    else if (bootPanelYOffset < 35)
    {bootPanelYOffset += 5;} 
    else if (bootPanelYOffset < 45)
    {bootPanelYOffset += 3;}
    else if (bootPanelYOffset < 50)
    {bootPanelYOffset += 2;}
    else
    {bootPanelYOffset += 2;}
    
    #ifdef OLED_DBG
      debugPort->println("bootPanelYOffset = " + String(bootPanelYOffset));
    #endif
  } else {
    #ifdef OLED_DBG
      debugPort->println("Boot animation not playing");
    #endif
      
    pico_setMainProgramMode(JSMARTSW_NORMAL_MODE);
  }
}

//////////////////////////////////

static const uint8_t* inputNumberToIconDataPointer(unsigned int inputNumber)
{
  const uint8_t* tempIconSet;

  //locate the input specs of the specific input
  input_number_spec tempInputSpec = inputNumberCustomizationData[inputNumber];
  //locate the beginning address of the bitmap icon set
  tempIconSet = &(inputIconBitmaps[tempInputSpec.inputIconCategory].bitmapSet[0]);
  //calculate the offset to the requested icon bitmap data
  int tempInSetIconOffset = tempInputSpec.inputIconId * INPUT_ICON_BITMAP_SIZE;

  return tempIconSet + tempInSetIconOffset;
}

static void drawScreenElement(bitmap_element_entity inputElement)
{
  if (inputElement.bitmapData   != NULL ||
      inputElement.bitmapWidth  != 0    ||
      inputElement.bitmapHeight != 0    )
  {
    display->drawBitmap(  inputElement.posX, inputElement.posY,
                        inputElement.bitmapData,
                        inputElement.bitmapWidth, inputElement.bitmapHeight,
                        inputElement.color);
  }
  display->writePixel(0, 0, 1);
}

static void normalMode_inputElementInit(bitmap_element_entity *inputElement, int elementType)
{
  // initialize repeating fixed data according to image element type

  if (INPUT_NUMBER_TYPE_BIG == elementType)
  {
    inputElement->posX = INPUT_NUMBER_BIG_POS_X;
    inputElement->posY = INPUT_NUMBER_BIG_POS_Y;
    inputElement->bitmapWidth = INPUT_NUMBER_BIG_BITMAP_WIDTH;
    inputElement->bitmapHeight = INPUT_NUMBER_BIG_BITMAP_HEIGHT;
  }

  if (INPUT_NUMBER_TYPE_MINI == elementType)
  {
    inputElement->posX = INPUT_NUMBER_MINI_POS_X;
    inputElement->posY = INPUT_NUMBER_MINI_POS_Y;
    inputElement->bitmapWidth = INPUT_NUMBER_MINI_BITMAP_WIDTH;
    inputElement->bitmapHeight = INPUT_NUMBER_MINI_BITMAP_HEIGHT;
  }

  if (INPUT_NUMBER_TYPE_ICON == elementType)
  {
    inputElement->posX = INPUT_ICON_POS_X + INPUT_ICON_OFFSET_X;
    inputElement->posY = INPUT_ICON_POS_Y + INPUT_ICON_OFFSET_Y;
    inputElement->bitmapWidth = INPUT_ICON_WIDTH;
    inputElement->bitmapHeight = INPUT_ICON_HEIGHT;
  }
}

//////////////////////////////////////////////////////////////////////////
// Setup Mode screen components
//////////////////////////////////////////////////////////////////////////

static void normalMode_screen_inputNumberOrRotationAnimation()
{
  unsigned int screenTargetInput = pico_getCurrentInput();
  static int inputNumberPaneloffset = 0;

  bitmap_element_entity currentInputNumber, currentInputIcon;
  bitmap_element_entity targetInputNumber, targetInputIcon;
  
  int widthToScroll;
  int rotationDirection;

  //decide rotation direction
  if (screenCurrentDisplayedInput < screenTargetInput)
  { //new input is larget, rotate to the left
    rotationDirection = ROTATE_LEFT;
  } else if (screenCurrentDisplayedInput > screenTargetInput)
  { //new input is smaller, rotate to the right
    rotationDirection = ROTATE_RIGHT;
  } else {
    rotationDirection = ROTATE_NONE;
  }

  //////////////////////////////
  // Current elements calculation
  //////////////////////////////
  //decide artwork big or small - current
  //current input parameters are always calculated and used
  bitmap_element_entity* inputElementPointer;  // tool pointer for feeding into init function
  inputElementPointer = &currentInputNumber;

  if (true == isInputNumberBig(screenCurrentDisplayedInput)) //if current is big
  {
    // Process current input number (big)
    normalMode_inputElementInit(inputElementPointer, INPUT_NUMBER_TYPE_BIG);
    
    currentInputNumber.posX += rotationDirection * inputNumberPaneloffset;
    currentInputNumber.bitmapData = &(bitmap_inputNumberBig[0][0]) + screenCurrentDisplayedInput * INPUT_NUMBER_BIG_BITMAP_SIZE;
  } else { // current is small
    // Process current input number (small)
    normalMode_inputElementInit(inputElementPointer, INPUT_NUMBER_TYPE_MINI);

    currentInputNumber.posX += rotationDirection * inputNumberPaneloffset;
    currentInputNumber.bitmapData = &(bitmap_inputNumberMini[0][0]) + screenCurrentDisplayedInput * INPUT_NUMBER_MINI_BITMAP_SIZE;
    
    // Process current input icon
    inputElementPointer = &currentInputIcon;
    normalMode_inputElementInit(inputElementPointer, INPUT_NUMBER_TYPE_ICON);

    currentInputIcon.posX += currentInputNumber.posX;
    currentInputIcon.bitmapData = inputNumberToIconDataPointer(screenCurrentDisplayedInput);
  }

  //rotation is needed
  if (screenCurrentDisplayedInput != screenTargetInput)
  { //////////////////////////////
    // Target elements calculation
    //////////////////////////////
    //target input parameters are only calculated when transition is necessary    
    if (true == isInputNumberBig(screenTargetInput)) //target is big
    {
      // Process target input number (big)
      inputElementPointer = &targetInputNumber;
      normalMode_inputElementInit(inputElementPointer, INPUT_NUMBER_TYPE_BIG);

      targetInputNumber.posX += rotationDirection * (inputNumberPaneloffset - INPUT_NUMBER_BIG_BITMAP_WIDTH);
      targetInputNumber.bitmapData = &(bitmap_inputNumberBig[0][0]) + screenTargetInput * INPUT_NUMBER_BIG_BITMAP_SIZE;

      widthToScroll = INPUT_NUMBER_BIG_BITMAP_WIDTH;
    } else { // target is small
      // Process target input number (small)
      inputElementPointer = &targetInputNumber;
      normalMode_inputElementInit(inputElementPointer, INPUT_NUMBER_TYPE_MINI);

      targetInputNumber.posX += rotationDirection * (inputNumberPaneloffset - INPUT_NUMBER_MINI_BITMAP_WIDTH);
      targetInputNumber.bitmapData = &(bitmap_inputNumberMini[0][0]) + screenTargetInput * INPUT_NUMBER_MINI_BITMAP_SIZE;

      widthToScroll = INPUT_NUMBER_MINI_BITMAP_WIDTH;

      // Process target input icon
      inputElementPointer = &targetInputIcon;
      normalMode_inputElementInit(inputElementPointer, INPUT_NUMBER_TYPE_ICON);
      
      targetInputIcon.posX = targetInputNumber.posX;
      targetInputIcon.bitmapData = inputNumberToIconDataPointer(screenTargetInput);
    }

    // Input number rotation animation on screen
    screenImportantActivity = true;  //wake up screen first
    
    //wipe the whole big area first
    display->fillRect ( 0, INPUT_NUMBER_BIG_POS_Y,
                        SCREEN_WIDTH, INPUT_NUMBER_BIG_BITMAP_HEIGHT,
                        SSD1306_BLACK);
    
    //if rotation is incomplete
    if ((widthToScroll - inputNumberPaneloffset) > SCREEN_INPUT_ROTATION_STEP)
    { ///////////////////////////////////////
      // Animation state
      ///////////////////////////////////////
      
      // Input number display
      ////current input number
      drawScreenElement(currentInputNumber);
      ////target input number
      drawScreenElement(targetInputNumber);

      // Input Icon display
      ////current input number icon
      if (false == isInputNumberBig(screenCurrentDisplayedInput))
      { 
        drawScreenElement(currentInputIcon);
      }
      
      ////target input number icon
      if (false == isInputNumberBig(screenTargetInput))
      { 
        drawScreenElement(targetInputIcon);
      }

      ////draw little ! mark on top of tinky during transition
      display->setTextSize(1);
      display->setTextColor(SSD1306_WHITE);
      display->setCursor(TINKY_EXCLAMATION_X, TINKY_EXCLAMATION_Y);
      display->print('!');

      inputNumberPaneloffset += SCREEN_INPUT_ROTATION_STEP;
    } else {  //rotation has just completed

      //display final target input number
      drawScreenElement(targetInputNumber);    

      //display final target input icon
      if (false == isInputNumberBig(screenTargetInput))
      { 
        drawScreenElement(targetInputIcon);                        
      }

      screenCurrentDisplayedInput = screenTargetInput;
      inputNumberPaneloffset = 0;
      rotationDirection = ROTATE_NONE;
    }     
  } else {
    ///////////////////////////////////////
    // Normal static state
    ///////////////////////////////////////
    //rotation is not needed
    inputNumberPaneloffset = 0; //must reset the offset or input number panels might get stuck in a wrong position

    //display current input number
    drawScreenElement(currentInputNumber);

    //display current input icon
    if (false == isInputNumberBig(screenCurrentDisplayedInput))
    {
      drawScreenElement(currentInputIcon);                      
    }
  }    
}

static void normalMode_screenComponent_mascots()
{
  if (pico_isThereInputActivity())//true == keypad_isThereButtonActivity() || true == controller_isThereActivity())
  {
    tinkySelect = TINKY_EAR_UP;
  } else {
    tinkySelect = TINKY_EAR_DOWN;
  }
  display->drawBitmap( MASCOT_SEPARATION_POS_Y - MELLY_WIDTH - MASCOT_SEPARATION_PIX_COUNT, MASCOT_BANNER_POS_Y,
                      bitmap_rt4kMelly,
                      MELLY_WIDTH, MELLY_HEIGHT,
                      SSD1306_WHITE);

  display->drawBitmap( MASCOT_SEPARATION_POS_Y, MASCOT_BANNER_POS_Y,
                      bitmap_rt4kTinky[tinkySelect],
                      TINKY_WIDTH, TINKY_HEIGHT,
                      SSD1306_WHITE);
}

static void normalMode_screenComponent_modeBanner()
{
  static int bannerPosX = 0;

  if (true == pico_getGswMode())
  { // Auto Banner
    display->drawBitmap( MODE_BANNER_START_POS_X, MODE_BANNER_START_POS_Y,
                        &bitmap_bannerBg[0][0],
                        MODE_BANNER_BG_WIDTH, MODE_BANNER_BG_HEIGHT,
                        SSD1306_WHITE);

    if (bannerPosX/BANNER_SCROLL_SPEED_FACTOR_AUTO + MODE_BANNER_AUTO_WIDTH < SCREEN_WIDTH)
    {
      bannerPosX = 0;
    }
    display->drawBitmap( MODE_BANNER_START_POS_X + MODE_BANNER_START_OFFSET_X + bannerPosX/BANNER_SCROLL_SPEED_FACTOR_AUTO, MODE_BANNER_START_POS_Y + MODE_BANNER_START_OFFSET_Y,
                        bitmap_modeBanner_auto,
                        MODE_BANNER_AUTO_WIDTH, MODE_BANNER_AUTO_HEIGHT,
                        SSD1306_INVERSE);
  } else {  //Manual Banner
    display->drawBitmap( MODE_BANNER_START_POS_X, MODE_BANNER_START_POS_Y,
                        &bitmap_bannerBg[1][0],
                        MODE_BANNER_BG_WIDTH, MODE_BANNER_BG_HEIGHT,
                        SSD1306_WHITE);
    if (bannerPosX/BANNER_SCROLL_SPEED_FACTOR_MANUAL + MODE_BANNER_MANUAL_WIDTH < SCREEN_WIDTH)
    {
      bannerPosX = 0;
    }
    display->drawBitmap( MODE_BANNER_START_POS_X + MODE_BANNER_START_OFFSET_X + bannerPosX/BANNER_SCROLL_SPEED_FACTOR_MANUAL, MODE_BANNER_START_POS_Y + MODE_BANNER_START_OFFSET_Y,
                        bitmap_modeBanner_manual,
                        MODE_BANNER_MANUAL_WIDTH, MODE_BANNER_MANUAL_HEIGHT,
                        SSD1306_WHITE);
  }
  bannerPosX--;
}

static void normalMode_screenComponent_buttonCommand()
{
  if (millis() - screenCommandTextDisplayTime < screenCommandTextDisplayDuration)
  {
    display->setTextSize(1); // Draw 2X-scale text
    display->setTextColor(SSD1306_WHITE);
    display->setCursor( ((SCREEN_WIDTH / 2) + 1 - (SCREEN_CHAR_SIZE_1_PITCH / 2 * screenCommandText.length())),
                        SCREEN_COMMAND_TEXT_POS_Y);
    display->println(screenCommandText);
  }
}

static void normalMode_screenComponent_controlInterface()
{
  if (RT4K_UART_PORT_TYPE_USBC == rt4k_getConnection())
  {
    display->drawBitmap(  HEART_POS_X, HEART_POS_Y,
                          bitmap_heart,
                          HEART_WIDTH, HEART_HEIGHT,
                          SSD1306_WHITE);
  }
}

//////////////////////////////////////////////////////////////////////////
// Various functions
//////////////////////////////////////////////////////////////////////////

void screen_setCommandTextUpdate(String stringToDisplay, unsigned long durationToDisplay)
{
  screenCommandText = stringToDisplay;
  if (0 == durationToDisplay)
  {
    screenCommandTextDisplayDuration = SCREEN_COMMAND_TEXT_DISPLAY_DURATION;
  } else {
    screenCommandTextDisplayDuration = durationToDisplay;
  }
  screenCommandTextDisplayTime = millis();
}

void screen_rebootScreen()
{
  display->fillRect   ( REBOOT_SPLASH_POS_X, REBOOT_SPLASH_POS_Y-1,
                        REBOOT_SPLASH_WIDTH, REBOOT_SPLASH_HEIGHT+2,
                        SSD1306_BLACK);
  display->drawBitmap ( REBOOT_SPLASH_POS_X, REBOOT_SPLASH_POS_Y,
                        bitmap_reboot_splash,
                        REBOOT_SPLASH_WIDTH, REBOOT_SPLASH_HEIGHT,
                        SSD1306_WHITE);
  display->display();
  delay(1000);
}

void screen_buttonErrorRebootMessage()
{
  //OLED error message
  display->clearDisplay();
  display->setTextSize(1); // Draw 2X-scale text
  display->setTextColor(SSD1306_WHITE);

  displayTextStaticCenteredOrScrolling("Key",   0, 10);
  displayTextStaticCenteredOrScrolling("Stuck",  0, 19);

  display->display();
}

void screen_clearSaveDataMessage()
{
  //OLED error message
  display->clearDisplay();
  display->setTextSize(1); // Draw 2X-scale text
  display->setTextColor(SSD1306_WHITE);

  displayTextStaticCenteredOrScrolling("All",   0, 10);
  displayTextStaticCenteredOrScrolling("Data",  0, 19);
  displayTextStaticCenteredOrScrolling("Reset", 0, 28);

  display->display();
}

void screen_setCommandTextIndex(int index)
{
  commandTextIndex = index;
}

static bool isInputNumberBig(unsigned int inputNumber)
{
  if (INPUT_NUMBER_IS_BIG == inputNumberCustomizationData[inputNumber].inputSize)
  {
    return true;
  }

  return false;
}

static void displayTextStaticCenteredOrScrolling(String text, int* posX, int posY)
{
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setTextWrap(false);

  if (text.length() < SCREEN_CHAR_COUNT_HORIZONTAL)
  { 
    display->setCursor( ((SCREEN_WIDTH / 2) + 1 - ( text.length() * SCREEN_CHAR_SIZE_1_PITCH / 2)),
                        posY);
    display->print(text);
  } else {  //scroll text if wider than screen
    if (frameCounter % 3 == 1)  {*posX -= 1;}
    if (*posX < (0 - SCREEN_CHAR_SIZE_1_PITCH * ((int)text.length() + 1))) {*posX = 0;}
    
    display->setCursor(*posX, posY);
    display->print(text + " " + text);
  }
}

//////////////////////////////////////////////////////////////////////////
// Setup Mode screen components
//////////////////////////////////////////////////////////////////////////

static void setupMode_screenComponent_inputNumber()
{
  static bitmap_element_entity setupMode_currentInputNumber;

  //prepare and present the small input number  
  setupMode_currentInputNumber.bitmapWidth  = INPUT_NUMBER_MINI_BITMAP_WIDTH;
  setupMode_currentInputNumber.bitmapHeight = INPUT_NUMBER_MINI_BITMAP_HEIGHT;
  setupMode_currentInputNumber.posX = SETUPMODE_INPUT_NUMBER_MINI_POS_X;
  setupMode_currentInputNumber.posY = SETUPMODE_INPUT_NUMBER_MINI_POS_Y;
  setupMode_currentInputNumber.bitmapData = &(bitmap_inputNumberMini[0][0]) + setupMode_inputToCustomize * INPUT_NUMBER_MINI_BITMAP_SIZE;
  
  drawScreenElement(setupMode_currentInputNumber);
}

static void setupMode_screenComponent_iconCategoryText()
{
  static int posX;
  displayTextStaticCenteredOrScrolling(inputIconBitmaps[setupMode_currentCategoryId].setName, &posX, SETUPMODE_CATEGORY_NAME_TEXT_POS_Y);
}

static void setupMode_inputIconScrollEnding()
{
  setupMode_iconScrollDirection = ROTATE_NONE;
  setupMode_currentCategoryId = targetCategoryId;
  setupMode_currentIconId = targetIconId;

  setupMode_currentInputIcon.bitmapData = &(inputIconBitmaps[setupMode_currentCategoryId].bitmapSet[0]) + setupMode_currentIconId * INPUT_ICON_BITMAP_SIZE;  
  drawScreenElement(setupMode_currentInputIcon);
}

static void setupMode_screenComponent_inputIcon()
{
  static int iconOffsetX = 0;
  static int iconOffsetY = 0;
  static bitmap_element_entity targetInputIcon;
  //static int iconWidthToScroll, iconHeightToScroll;
  static int iconStepDirectionX, iconStepDirectionY;

  //decide target icon parameters
  switch(setupMode_iconScrollDirection)
  {
    case ROTATE_UP:
      targetIconId = 0;
      targetCategoryId = setupMode_currentCategoryId - 1;
      if (targetCategoryId < 0)
      {
        targetCategoryId = ICON_SET_BOUNDARY;
      }

      targetInputIcon.posX = SETUPMODE_INPUT_ICON_POS_X;
      iconStepDirectionX = 0;
      iconStepDirectionY = -1;
      //iconWidthToScroll  = 0;
      //iconHeightToScroll = INPUT_ICON_HEIGHT;

      break;
    case ROTATE_DOWN:
      targetIconId = 0;
      targetCategoryId = setupMode_currentCategoryId + 1;
      if (targetCategoryId > ICON_SET_BOUNDARY)
      {
        targetCategoryId = 0;
      }

      targetInputIcon.posX = SETUPMODE_INPUT_ICON_POS_X;
      iconStepDirectionX = 0;
      iconStepDirectionY = 1;
      //iconWidthToScroll  = 0;
      //iconHeightToScroll = INPUT_ICON_HEIGHT;

      break;
    case ROTATE_LEFT:
      targetCategoryId = setupMode_currentCategoryId;
      targetIconId = setupMode_currentIconId - 1;
      if (targetIconId < 0)
      {
        targetIconId = inputIconBitmaps[setupMode_currentCategoryId].elementCount - 1;
      }

      targetInputIcon.posY = SETUPMODE_INPUT_ICON_POS_Y;
      iconStepDirectionX = -1;
      iconStepDirectionY = 0;
      //iconWidthToScroll  = SCREEN_WIDTH;
      //iconHeightToScroll = 0;

      break;
    case ROTATE_RIGHT:
      targetCategoryId = setupMode_currentCategoryId;
      targetIconId = setupMode_currentIconId + 1;
      if (targetIconId > (int)inputIconBitmaps[setupMode_currentCategoryId].elementCount - 1)
      {
        targetIconId = 0;
      }

      targetInputIcon.posY = SETUPMODE_INPUT_ICON_POS_Y;
      iconStepDirectionX = 1;
      iconStepDirectionY = 0;
      //iconWidthToScroll  = SCREEN_WIDTH;
      //iconHeightToScroll = 0;

      break;
  }

  if (ROTATE_NONE != setupMode_iconScrollDirection)
  {
    targetInputIcon.bitmapWidth = INPUT_ICON_WIDTH;
    targetInputIcon.bitmapHeight = INPUT_ICON_HEIGHT;
    targetInputIcon.bitmapData = &(inputIconBitmaps[targetCategoryId].bitmapSet[0]) + targetIconId * INPUT_ICON_BITMAP_SIZE;

    if (ROTATE_UP == setupMode_iconScrollDirection || ROTATE_DOWN == setupMode_iconScrollDirection)
    {
      iconOffsetY += SETUPMODE_ICON_ROTATION_STEP_Y;

      if (iconOffsetY + SETUPMODE_ICON_ROTATION_STEP_Y > INPUT_ICON_HEIGHT)
      {
        iconOffsetX = 0;
        iconOffsetY = 0;
        setupMode_inputIconScrollEnding();
      }
    }

    if (ROTATE_LEFT == setupMode_iconScrollDirection || ROTATE_RIGHT == setupMode_iconScrollDirection)
    {
      iconOffsetX += SETUPMODE_ICON_ROTATION_STEP_X;

      if (iconOffsetX + SETUPMODE_ICON_ROTATION_STEP_X > SCREEN_WIDTH)
      {
        iconOffsetX = 0;
        iconOffsetY = 0;
        setupMode_inputIconScrollEnding();
      }
    }

    setupMode_currentInputIcon.posX = SETUPMODE_INPUT_ICON_POS_X - iconOffsetX * iconStepDirectionX;
    setupMode_currentInputIcon.posY = SETUPMODE_INPUT_ICON_POS_Y - iconOffsetY * iconStepDirectionY;

    targetInputIcon.posX = SETUPMODE_INPUT_ICON_POS_X - iconOffsetX * iconStepDirectionX + iconStepDirectionX * SCREEN_WIDTH;
    targetInputIcon.posY = SETUPMODE_INPUT_ICON_POS_Y - iconOffsetY * iconStepDirectionY + iconStepDirectionY * INPUT_ICON_HEIGHT;

    drawScreenElement(targetInputIcon);
    drawScreenElement(setupMode_currentInputIcon);

  } else {
    setupMode_currentInputIcon.bitmapData = &(inputIconBitmaps[setupMode_currentCategoryId].bitmapSet[0]) + setupMode_currentIconId * INPUT_ICON_BITMAP_SIZE;
    drawScreenElement(setupMode_currentInputIcon);
  }

  display->fillRect ( 0, 0,
                    SCREEN_WIDTH, SETUPMODE_INPUT_ICON_POS_Y,
                    SSD1306_BLACK);
  display->fillRect ( 0, SETUPMODE_INPUT_ICON_POS_Y + INPUT_ICON_HEIGHT,
                    SCREEN_WIDTH, SCREEN_HEIGHT - SETUPMODE_INPUT_ICON_POS_Y - INPUT_ICON_HEIGHT,
                    SSD1306_BLACK);
}

static void setupMode_screenComponent_stripes()
{
  bitmap_element_entity stripe;

  stripe.bitmapWidth = SETUPMODE_STRIPE_WIDTH;
  stripe.bitmapHeight = SETUPMODE_STRIPE_HEIGHT;
  stripe.bitmapData = bitmap_setupMode_stripe;

  stripe.posX = SETUPMODE_STRIP_TOP_POS_X;
  stripe.posY = SETUPMODE_STRIP_TOP_POS_Y;
  
  drawScreenElement(stripe);

  /*  Removed due to too little vertical space
  stripe.posX = SETUPMODE_STRIP_BOTTOM_POS_X;
  stripe.posY = SETUPMODE_STRIP_BOTTOM_POS_Y;
  
  drawScreenElement(stripe);
  */

  // Countdown indicator - 
  // cover up the diagonal patterns inside the strip
  // from the right end as a 5 second countdown
  static unsigned long remainingTime = 0;

  if (JSMARTSW_SETUP_MODE == screen_operationMode)
  {
    remainingTime = pico_getSetupModeRemainingTime();
  }

  int countdownPosX = (int)((float)SCREEN_WIDTH * ((float)remainingTime/(float)5000ul));

  #ifdef SCREEN_DBG
    debugPort->println("[Screen] Setup Mode remaining time: " + String(remainingTime));
    debugPort->println("[Screen] Countdown blackout PosX: " + String(countdownPosX));
  #endif

  if (remainingTime < 5000ul)
  {
    display->fillRect(countdownPosX, 2, SCREEN_WIDTH, 3, SSD1306_BLACK);
  }
}

static void displayTextAlternative(String str1, String str2, int posY)
{
  static bool displayStr1 = false;

  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setTextWrap(false);

  if (frameCounter % 30 == 0)
  {
    displayStr1 = !displayStr1;
  }

  if (str1.length() + str2.length() > SCREEN_CHAR_COUNT_HORIZONTAL - 1)
  {
    if (displayStr1)
    {
      display->setCursor( 0, posY);
      display->print(str1);
    } else {
      display->setCursor( 0,//(SCREEN_CHAR_COUNT_HORIZONTAL - str2.length()) * SCREEN_CHAR_SIZE_1_PITCH - 3, 
                          posY);
      display->print("\xB0" + str2);
    }
  } else {
    display->setCursor( 0, posY);
    display->print(str1 + str2);
  }
}

static void setupMode_screenComponent_toolTip()
{
  static bool arrowFlash = true;
  static int currentControlDevice;

  currentControlDevice = pico_currentControlDevice();

  #ifdef SETUPMODE_DBG
    debugPort->println("[SETUP] Current Control Device: " + String(currentControlDevice));
  #endif

  ////////////////////////////
  //Directional tool tips
  unsigned int arrowSet = 0;
  bitmap_element_entity arrow;

  if (CONTROLLER_KEYPAD == currentControlDevice)
  {
    arrowSet = 1;
  }

  arrow.bitmapWidth  = SETUPMODE_ARROW_WIDTH;
  arrow.bitmapHeight = SETUPMODE_ARROW_HEIGHT;
  
  if (true == arrowFlash)
  {
    if (ICON_SET_EMPTY != setupMode_currentCategoryId)
    {
      //Left & Right
      arrow.posY = SETUPMODE_ARROW_HORIZONTAL_Y;
      ////Left
      display->fillRect(SETUPMODE_ARROW_LEFT_POS_X - 1, SETUPMODE_ARROW_HORIZONTAL_Y - 1,
                        SETUPMODE_ARROW_WIDTH + 2, SETUPMODE_ARROW_HEIGHT + 2,
                        SSD1306_BLACK);
      arrow.posX = SETUPMODE_ARROW_LEFT_POS_X;
      arrow.bitmapData = &bitmap_setupMode_arrowSet[arrowSet][ARROW_LEFT][0];
      drawScreenElement(arrow);
      ////Right
      display->fillRect(SETUPMODE_ARROW_RIGHT_POS_X - 1, SETUPMODE_ARROW_HORIZONTAL_Y - 1,
                        SETUPMODE_ARROW_WIDTH + 2, SETUPMODE_ARROW_HEIGHT + 2,
                        SSD1306_BLACK);
      arrow.posX = SETUPMODE_ARROW_RIGHT_POS_X;
      arrow.bitmapData = &bitmap_setupMode_arrowSet[arrowSet][ARROW_RIGHT][0];
      drawScreenElement(arrow);
    }

    //Up & Down
    arrow.posX = SETUPMODE_ARROW_VERTICAL_POS_X;
    ////Up
    display->fillRect(SETUPMODE_ARROW_VERTICAL_POS_X - 1, SETUPMODE_ARROW_UP_POS_Y - 1,
                      SETUPMODE_ARROW_WIDTH + 2, SETUPMODE_ARROW_HEIGHT + 2,
                      SSD1306_BLACK);
    arrow.posY = SETUPMODE_ARROW_UP_POS_Y;
    arrow.bitmapData = &bitmap_setupMode_arrowSet[arrowSet][ARROW_UP][0];
    drawScreenElement(arrow);

    ////Down
    display->fillRect(SETUPMODE_ARROW_VERTICAL_POS_X - 1, SETUPMODE_ARROW_DOWN_POS_Y - 1,
                      SETUPMODE_ARROW_WIDTH + 2, SETUPMODE_ARROW_HEIGHT + 2,
                      SSD1306_BLACK);
    arrow.posY = SETUPMODE_ARROW_DOWN_POS_Y;
    arrow.bitmapData = &bitmap_setupMode_arrowSet[arrowSet][ARROW_DOWN][0];
    drawScreenElement(arrow);    
  }

  if (frameCounter % 30 == 0)
  {
    arrowFlash = !arrowFlash;
  }

  ////////////////////////////
  //Bottom tool tips
  String buttonYes, buttonNo, buttonClr;

  switch(currentControlDevice)
  {
    // PlayStation Family
    case CONTROLLER_DUALSHOCK3:
    case CONTROLLER_DUALSHOCK4:
    case CONTROLLER_DUALSENSE:
    case CONTROLLER_PSX2USB:
      buttonYes = "X";
      buttonNo  = "O";
      buttonClr = "\x7F";
      break;

    //Nintendo Family
    case CONTROLLER_SWITCHPRO:
    case CONTROLLER_SWITCHJOYCON:
      buttonYes = "A";
      buttonNo  = "B";
      buttonClr = "X";
      break;

    //Sega Family
    case CONTROLLER_SATURN_USB:
    case CONTROLLER_MD2X:
    case CONTROLLER_8BITDO_M30:
      buttonYes = "A";
      buttonNo  = "B";
      buttonClr = "C";
      break;

    case CONTROLLER_KEYBOARD:
      buttonYes = "\xD9";
      buttonNo  = "Esc";
      buttonClr = "Del";
      break;

    case CONTROLLER_MOUSE:
      buttonYes = "L";
      buttonNo  = "R";
      buttonClr = "L+R";
      break;

    case CONTROLLER_KEYPAD:
      buttonYes = "6";
      buttonNo  = "7";
      buttonClr = "8";
      break;
  }

  String helpTipYes = ":Yes";
  String helpTipNo  = ":No";
  String helpTipClr = ":Clr";

  display->cp437(true);

  displayTextAlternative(buttonYes, helpTipYes, SETUPMODE_TOOLTIP_YES_POS_Y);
  displayTextAlternative(buttonNo,  helpTipNo,  SETUPMODE_TOOLTIP_NO_POS_Y);
  displayTextAlternative(buttonClr, helpTipClr, SETUPMODE_TOOLTIP_CLEAR_POS_Y);
}

void screen_setupMode_iconSetCycleUp()
{
  setupMode_iconScrollDirection = ROTATE_UP;
}

void screen_setupMode_iconSetCycleDown()
{
  setupMode_iconScrollDirection = ROTATE_DOWN;
}

void screen_setupMode_iconIdCycleUp()
{
  if (ICON_SET_EMPTY != setupMode_currentCategoryId)
  {
    setupMode_iconScrollDirection = ROTATE_RIGHT;
  }
}

void screen_setupMode_iconIdCycleDown()
{
  if (ICON_SET_EMPTY != setupMode_currentCategoryId)
  {
    setupMode_iconScrollDirection = ROTATE_LEFT;
  }
}

void screen_setupMode_confirmChoice()
{
  // no change, no need to save
  if ((int)inputNumberCustomizationData[setupMode_inputToCustomize].inputIconCategory  == setupMode_currentCategoryId &&
      (int)inputNumberCustomizationData[setupMode_inputToCustomize].inputIconId        == setupMode_currentIconId      )
  {
    return;
  }

  // Chose the No Icon category, equals to reset
  if (ICON_SET_EMPTY == setupMode_currentCategoryId)
  {
    screen_setupMode_resetToDefault();
    return;
  }

  inputNumberCustomizationData[setupMode_inputToCustomize].inputIconCategory = setupMode_currentCategoryId;
  inputNumberCustomizationData[setupMode_inputToCustomize].inputIconId = setupMode_currentIconId;
  inputNumberCustomizationData[setupMode_inputToCustomize].inputSize = INPUT_NUMBER_IS_SMALL;
  pico_setSaveDataDirty();
  
  screen_setCommandTextUpdate("SAVED", SETUPMODE_MESSAGE_DISPLAY_DURATION);
}

void screen_setupMode_resetToDefault()
{
  inputNumberCustomizationData[setupMode_inputToCustomize].inputIconCategory = ICON_SET_EMPTY;
  inputNumberCustomizationData[setupMode_inputToCustomize].inputIconId = 0;
  inputNumberCustomizationData[setupMode_inputToCustomize].inputSize = INPUT_NUMBER_IS_BIG;
  pico_setSaveDataDirty();

  screen_setCommandTextUpdate("CLEAR", SETUPMODE_MESSAGE_DISPLAY_DURATION);
}

static void setupMode_init()
{
  //read the held button number translated to input number from main program
  setupMode_inputToCustomize = pico_getSetupModeInputNumber();

  //prepare the current selected input icon
  setupMode_currentInputIcon.bitmapWidth  = INPUT_ICON_WIDTH;
  setupMode_currentInputIcon.bitmapHeight = INPUT_ICON_HEIGHT;
  setupMode_currentInputIcon.posX = SETUPMODE_INPUT_ICON_POS_X;
  setupMode_currentInputIcon.posY = SETUPMODE_INPUT_ICON_POS_Y;
  setupMode_currentInputIcon.bitmapData = inputNumberToIconDataPointer(setupMode_inputToCustomize);

  input_number_spec tempInputSpec = inputNumberCustomizationData[setupMode_inputToCustomize];
  setupMode_currentCategoryId = tempInputSpec.inputIconCategory;
  setupMode_currentIconId = tempInputSpec.inputIconId;
}

void screen_modeTransitionAnimation()
{
  if ( (JSMARTSW_ENTERING_SETUP_MODE  == screen_operationMode ||
        JSMARTSW_EXITING_SETUP_MODE   == screen_operationMode ||
        JSMARTSW_NORMAL_MODE          == screen_operationMode ||
        JSMARTSW_SETUP_MODE           == screen_operationMode  ) &&
      setupMode_transitionPanel_posX > SETUPMODE_TRANSITION_PANEL_POSITION_END  )
  {
    
    static bitmap_element_entity panel;
    panel.posY = 0;
    panel.bitmapWidth  = SETUPMODE_TRANSITION_PANEL_WIDTH;
    panel.bitmapHeight = SETUPMODE_TRANSITION_PANEL_HEIGHT;
    panel.bitmapData = bitmap_setupMode_transitionPanel;
    panel.color = SSD1306_BLACK;

    panel.posX = setupMode_transitionPanel_posX;
    
    drawScreenElement(panel);
    
    /*
    static bitmap_element_entity panelBg, panel;
    panel.posY = 0;
    panel.bitmapWidth  = SETUPMODE_TRANSITION_LOVE_PANEL_WIDTH;
    panel.bitmapHeight = SETUPMODE_TRANSITION_LOVE_PANEL_HEIGHT;
    panel.bitmapData = bitmap_setupMode_transition_lovePanel;
    
    panelBg.posY = 0;
    panelBg.bitmapWidth  = SETUPMODE_TRANSITION_LOVE_PANEL_BG_WIDTH;
    panelBg.bitmapHeight = SETUPMODE_TRANSITION_LOVE_PANEL_BG_HEIGHT;
    panelBg.bitmapData = bitmap_setupMode_transition_lovePanel_bg;    

    panelBg.color = SSD1306_BLACK;

    panel.posX = panelBg.posX = setupMode_transitionPanel_posX;
    
    drawScreenElement(panelBg);
    */
    drawScreenElement(panel);

    setupMode_transitionPanel_posX -= SETUPMODE_TRANSITION_PANEL_MOVE_STEP;
  }
}

static bool modeChangeTriggered()
{
  if (setupMode_transitionPanel_posX + SETUPMODE_TRANSITION_PANEL_TRANSITION_LEFT < 0)
  {
    screenImportantActivity = true;
    return true;
  }

  return false;
}

static bool transitionHasFinished()
{
  if (setupMode_transitionPanel_posX <= SETUPMODE_TRANSITION_PANEL_POSITION_END)
  {
    return true;
  }

  return false;
}

static void transitionReset()
{
  setupMode_transitionPanel_posX = SETUPMODE_TRANSITION_PANEL_POSITION_START;
}

//////////////////////////////////////////////////////////////////////////

bool screen_isThereActivity()
{
  return screenImportantActivity;
}

void screen_flip(unsigned int newOrientation)
{
  screenOrientation = newOrientation;

  bufferA.setRotation(screenOrientation);
  bufferA.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
}