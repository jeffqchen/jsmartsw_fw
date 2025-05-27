#ifndef GSW_HARDWARE_H
#define GSW_HARDWARE_H

// gscartsw version select
#define GSCARTSW_VER_5_2

// gscartsw v5.2 hardware definition and parameters
#ifdef GSCARTSW_VER_5_2

  #define GSCARTSW_VER  "5.2"  
  
  ////////////////////////////////////////////////////////
  // Input Number Definition
  ////////////////////////////////////////////////////////
  enum {
    NO_INPUT  = 0,
    INPUT_1,
    INPUT_2,
    INPUT_3,
    INPUT_4,
    INPUT_5,
    INPUT_6,
    INPUT_7,
    INPUT_8,
    INPUT_END
  }; // gsw_input_number;
  #define GSCARTSW_NUM_INPUTS (INPUT_END - 1)
  #define INPUT_LOWEST  INPUT_1
  #define INPUT_HIGHEST (INPUT_END - 1)

  ////////////////////////////////////////////////////////
  // gScartSW EXT port pin definiotion
  ////////////////////////////////////////////////////////
  enum {
    GSW_EXT_PIN_1,  //GND
    GSW_EXT_PIN_2,  //Override @ 3V3
    GSW_EXT_PIN_3,  //NC
    GSW_EXT_PIN_4,  //+5V
    GSW_EXT_PIN_5,  //BIT_0
    GSW_EXT_PIN_6,  //BIT_1
    GSW_EXT_PIN_7,  //BIT_2
    GSW_EXT_PIN_8,  //NC
    GSW_EXT_PIN_END
  }; // gsw_ext_pin_header;
  ////Overrid pin
  #define EXT_PIN_OVERRIDE  (pin_size_t)2  
  #define GSCART_OVERRIDE_PIN_AUTO_MODE HIGH
  #define GSCART_OVERRIDE_PIN_MANUAL_MODE LOW
  ////Data pins
  enum {
    EXT_PIN_1 = 0,
    EXT_PIN_2,
    EXT_PIN_3,
    EXT_PIN_END,
  }; // gsw_input_data_pin;
  #define EXT_DATA_PIN_COUNT  EXT_PIN_END

  #define PICO_PIN_EXT_BIT_0  (pin_size_t)22
  #define PICO_PIN_EXT_BIT_1  (pin_size_t)26
  #define PICO_PIN_EXT_BIT_2  (pin_size_t)27
  #define PICO_EXT_PIN_ARRAY  { PICO_PIN_EXT_BIT_0, \
                                PICO_PIN_EXT_BIT_1, \
                                PICO_PIN_EXT_BIT_2  }
  //sampling parameters
  #define GSW_EXT_LOGIC_HIGH_COUNT  6
  #define GSW_EXT_LOGIC_MID_COUNT   2
  #define INPUT_BITS_SAMPLING_PERIOD (unsigned long)11 //ms
  #define REQUIRED_SAMPLE_CYCLES 8 // Actual count is plus one

  #define AUTO_MODE_CHANGE_DEBOUNCE (unsigned long)60 //ms
                                                      //Smaller numbers cause input bouncing when switching to auto mode

  ////////////////////////////////////////////////////////
  // RT4K UART
  ////////////////////////////////////////////////////////
  #define PICO_PIN_DISABLE_AUTO_MODE  (pin_size_t)6

  // Screen
  #define SCREEN_WIDTH_PHYSICAL   128 // OLED display width, in pixels
  #define SCREEN_HEIGHT_PHYSICAL  32  // OLED display height, in pixels

  #define SCREEN_WIDTH            SCREEN_HEIGHT_PHYSICAL  //Swap height and width due to vertical ussage  
  #define SCREEN_HEIGHT           SCREEN_WIDTH_PHYSICAL
  #define SCREEN_ROTATION_DEFAULT   1   ////clockwise, 1 = 90º, 3 = 270º
  #define SCREEN_ROTATION_FLIP      3   //Flipped

  ////////////////////////////////////////////////////////
  // RGB LEDs
  ////////////////////////////////////////////////////////
  #define RGBLED_DATA_PIN  (pin_size_t)9

  #define RGBLED_POWER_LIMIT
  #ifdef RGBLED_POWER_LIMIT
    #define RGBLED_POWER_VOLT   5
    #define RGBLED_POWER_AMP    300
  #endif

  enum {
    RGBLED_INPUT_1  = 0,
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
    RGBLED_END
  }; //rgbLed_number;
  #define NUM_LEDS              RGBLED_END
  #define NUM_LEDS_FIRST_ROW    (RGBLED_INPUT_8 + 1)

  ////////////////////////////////////////////////////////
  // Keypad Buttons
  ////////////////////////////////////////////////////////
  enum {
    PHY_BUTTON_1  = INPUT_1 - 1,
    PHY_BUTTON_2,
    PHY_BUTTON_3,
    PHY_BUTTON_4,
    PHY_BUTTON_5,
    PHY_BUTTON_6,
    PHY_BUTTON_7,
    PHY_BUTTON_8,
    PHY_BUTTON_END
  }; // physical_button_number;
  #define BUTTON_COUNT PHY_BUTTON_END

  // Button hardware setup
  #define BUTTON_DOWN 1
  #define BUTTON_UP   0
  
  // button pin definitions
  #define PICO_PIN_BUTTON_1 (pin_size_t)10
  #define PICO_PIN_BUTTON_2 (pin_size_t)11
  #define PICO_PIN_BUTTON_3 (pin_size_t)12
  #define PICO_PIN_BUTTON_4 (pin_size_t)13
  #define PICO_PIN_BUTTON_5 (pin_size_t)18
  #define PICO_PIN_BUTTON_6 (pin_size_t)19
  #define PICO_PIN_BUTTON_7 (pin_size_t)20
  #define PICO_PIN_BUTTON_8 (pin_size_t)21

  #define PICO_PIN_BUTTON_ARRAY { PICO_PIN_BUTTON_1,  \
                                  PICO_PIN_BUTTON_2,  \
                                  PICO_PIN_BUTTON_3,  \
                                  PICO_PIN_BUTTON_4,  \
                                  PICO_PIN_BUTTON_5,  \
                                  PICO_PIN_BUTTON_6,  \
                                  PICO_PIN_BUTTON_7,  \
                                  PICO_PIN_BUTTON_8   }

  //button values
  #define KEYPAD_VALUE_READING_NONE 0x0
  #define KEYPAD_VALUE_1_SHIFT  0x1
  #define KEYPAD_VALUE_2_WAKE   0x2
  #define KEYPAD_VALUE_3_4K     0x4
  #define KEYPAD_VALUE_4_1080P  0x8
  #define KEYPAD_VALUE_5_HD15   0x10
  #define KEYPAD_VALUE_6_SCART  0x20
  #define KEYPAD_VALUE_7_DIMMER 0x40
  #define KEYPAD_VALUE_8_AUTO   0x80

  #define KEYPAD_VALUE_LIST { KEYPAD_VALUE_1_SHIFT,   \
                              KEYPAD_VALUE_2_WAKE,    \
                              KEYPAD_VALUE_3_4K,      \
                              KEYPAD_VALUE_4_1080P,   \
                              KEYPAD_VALUE_5_HD15,    \
                              KEYPAD_VALUE_6_SCART,   \
                              KEYPAD_VALUE_7_DIMMER,  \
                              KEYPAD_VALUE_8_AUTO,    }

  //command messages to display under mode banner
  #define COMMAND_TEXT_STRING_ARRAY { " ",      \
                                      "SHIFT",  \
                                      "WAKE",   \
                                      "4K",     \
                                      "1080P",  \
                                      "HD15",   \
                                      "SCART",  \
                                      "DIM",    \
                                      "AUTO"    }
  
  #define CONTROL_INTERFACE_NAME_STRING { "HD15", \
                                          "USB-C" }

#endif

#endif