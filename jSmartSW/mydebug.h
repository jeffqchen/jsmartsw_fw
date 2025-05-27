#ifndef MYDEBUG_H
#define MYDEBUG_H

///////////////////
//Toggle Debug Mode
///////////////////

//#define DEBUG

///////////////////

// Toggle various debug messages to USB serial
#ifdef DEBUG

#include <cstring>

  //#define PICO_DBG
  //#define PWR_DEBUG

  //#define FLASH_DBG
  //#define FORCE_FLASH
  //#define INJECT_FAKE_DATA

  //#define RT4K_DBG
  //#define CDC_DBG

  //#define SIGNAL_IN_DBG
  //#define SIGNAL_OUT_DBG
  //#define EXT_DBG

  //#define RAW_BUTTON_DBG
  //#define BUTTON_LOGIC_DBG

  //#define OLED_DBG
  //#define RGBLED_DBG
  //#define SCREEN_DBG

  //#define CONTROLLER_REPORT_DBG
  #ifdef CONTROLLER_REPORT_DBG
    #define REPORT_FILTERING
    #ifdef REPORT_FILTERING

    #define NO_SWITCH_INPUT_REPORT  (0x21 != report[0])
    #ifndef NO_SWITCH_INPUT_REPORT
      #define NO_SWITCH_INPUT_REPORT  true
    #endif

    #define NO_DS3_REPORT           && (0x01 != report[0])
    #ifndef NO_DS3_REPORT
      #define NO_DS3_REPORT           && true
    #endif
    
    #else
      #define NO_REPORT_FILTERING
    #endif
  #endif

  #define CONTROLLER_DBG
  #ifdef CONTROLLER_DBG
    //#define DS3_DBG
    //#define DS4_DBG
    //#define DS5_DBG

    //#define SWITCHLIB_DBG
    #define SWITCHJOYCON_DBG
    //#define SWITCHPRO_DBG    

    //#define SATURNUSB_DBG
    //#define KEYBOARD_DBG
    //#define MOUSE_DBG
  #endif

  //#define SETUPMODE_DBG

  //Performance check
  //#define MAIN_LOOP_TIMING_DBG
  //#define SECOND_LOOP_TIMING_DBG
  //#define SCREEN_TIMING_DBG
#endif

#endif