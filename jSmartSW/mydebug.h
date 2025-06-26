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
  //#define RT4K_PWR_DBG
  //#define CDC_DBG

  //#define SIGNAL_IN_DBG
  //#define SIGNAL_OUT_DBG
  //#define EXT_DBG

  //#define RAW_BUTTON_DBG
  //#define BUTTON_LOGIC_DBG

  //#define OLED_DBG
  //#define RGBLED_DBG
  //#define SCREEN_DBG

  #define CONTROLLER_DBG
  #ifdef CONTROLLER_DBG
    //#define HID_DBG
    
    //#define DS3_DBG
    //#define DS4_DBG
    //#define DS5_DBG

    //#define SWITCHLIB_DBG
    //#define SWCON_DBG
    //#define SWITCHPRO_DBG

    //#define SW2_DBG
    //#define SW2_VERBOSE_DBG

    //#define SATURNUSB_DBG
    //#define KEYBOARD_DBG
    //#define MOUSE_DBG
  #endif

  //#define CONTROLLER_REPORT_DBG
  //Report filtering
  #ifdef CONTROLLER_REPORT_DBG   
    #define REPORT_FILTERING
    #ifdef REPORT_FILTERING

      #if defined(SWITCHLIB_DBG) || defined(SWCON_DBG)
        #define NO_SWITCH_INPUT_REPORT    && (0x21 != report[0])
      #endif
      //////defaults//////
      #ifndef NO_SWITCH_INPUT_REPORT
        #define NO_SWITCH_INPUT_REPORT  && true
      #endif
      //////defaults//////

      #if defined(DS3_DBG) || defined(DS4_DBG) || defined(DS5_DBG)
        #define NO_DS_REPORT             && (0x01 != report[0])
      #endif
      //////defaults//////
      #ifndef NO_DS_REPORT
        #define NO_DS_REPORT           && true
      #endif
      //////defaults//////

      #ifdef SW2_DBG
        #define NO_SW2_REPORT
      #endif
      #ifdef NO_SW2_REPORT
        //#define NO_JOYCON2_L_REPORT       && (0x7 != report[0])
        #define NO_JOYCON2_R_REPORT       && (0x8 != report[0])
        //#define NO_PROCON2_REPORT         && (0x9 != report[0])
      #endif

      //////defaults//////
      #ifndef NO_JOYCON2_L_REPORT
        #define NO_JOYCON2_L_REPORT     && true
      #endif
      
      #ifndef NO_JOYCON2_R_REPORT
        #define NO_JOYCON2_R_REPORT     && true
      #endif
      
      #ifndef NO_PROCON2_REPORT
        #define NO_PROCON2_REPORT       && true
      #endif
      //////defaults//////

    #else
      #define NO_REPORT_FILTERING
    #endif
  #endif
  //end of report filtering

  //#define SETUPMODE_DBG

  //Performance check
  //#define MAIN_LOOP_TIMING_DBG
  //#define SECOND_LOOP_TIMING_DBG
  //#define SCREEN_TIMING_DBG
#endif

#endif