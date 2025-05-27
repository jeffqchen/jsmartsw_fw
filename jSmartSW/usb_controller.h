#include <sys/_intsup.h>
#include <sys/types.h>
#include <sys/_stdint.h>
#include "tusb.h"

#include "mypico.h"

#ifndef USB_CONTROLLER_H
#define USB_CONTROLLER_H

//Tick action related
typedef enum
{
  HOLDING_RIGHT,
  HOLDING_LEFT,
  HOLDING_NEITHER
} holding_left_right;

#define RUMBLE_TICK_DURATION 1000
#define TICK_EVERY_TIMES 10
#define RUMBLE_TICK_ADJUST_VALUE 1

enum {
  CONTROLLER_NONE = 0,
  CONTROLLER_KEYPAD,
  CONTROLLER_KEYBOARD,
  CONTROLLER_MOUSE,
  CONTROLLER_DUALSHOCK3,
  CONTROLLER_DUALSHOCK4,
  CONTROLLER_DUALSENSE,
  CONTROLLER_SWITCHJOYCON,
  CONTROLLER_SWITCHPRO,
  CONTROLLER_8BITDO_M30,
  CONTROLLER_SATURN_USB,
  CONTROLLER_PSX2USB,
  CONTROLLER_MD2X
};

//mapped controller reading
typedef struct _game_controller_data {
  //dpad
  bool dpad_up;
  bool dpad_down;
  bool dpad_left;
  bool dpad_right;

  //face buttons
  bool apad_north;
  bool apad_south;
  bool apad_east;
  bool apad_west;

  //select & start OR share & option
  bool button_start;
  bool button_select;
  bool button_home;

  // shoulder buttons
  bool button_l1;
  bool button_r1;

  bool button_l2;
  bool button_r2;

  //analog stick buttons
  bool button_l3;
  bool button_r3;

  //modern buttons on PlayStation controlelrs  
  bool button_touchPadButton;

  bool hasActivity;
  
  uint8_t dev_addr;
  uint8_t instance;

  int controllerType;

  _game_controller_data() 
  {
    dpad_up = dpad_down = dpad_left = dpad_right = apad_north = apad_south = apad_east = apad_west = button_start = button_select = button_home = button_l1 = button_r1 = button_l2 = button_r2 = button_l3 = button_r3 = button_touchPadButton = hasActivity = false;
    controllerType = CONTROLLER_NONE;
  }
} game_controller_data;

#define game_controller_data()   _game_controller_data() 

typedef struct _keyboard_data {
  uint8_t modifier;
  uint8_t firstKey;
  bool hasActivity;

  _keyboard_data()
  {
    modifier = 0x0;
    firstKey = 0x0;
    hasActivity = false;
  }
} keyboard_data;

#define keyboard_data()   _keyboard_data()

typedef struct {
  void (*mount) (uint8_t, uint8_t);
  void (*umount)(uint8_t, uint8_t);

  void (*hid_set_report_complete) (uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
  void (*hid_get_report_complete) (uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
  void (*hid_report_received)     (uint8_t, uint8_t, uint8_t const*, uint16_t);

  bool (*hasActivity)();

  void (*setupTick)   (holding_left_right, uint8_t, uint8_t);
  void (*visualTick)  (holding_left_right, uint8_t, uint8_t);
  void (*tactilelTick)(holding_left_right, uint8_t, uint8_t);
  void (*executeTick) (holding_left_right, uint8_t, uint8_t);
} controller_api_interface;

#endif

//external
void routine_updateControllerReading();

game_controller_data controller_currentReading();
bool controller_isThereActivity();
void controller_tickReset();
void controller_holdLeftRightRumbleTick(holding_left_right, uint8_t, uint8_t);
//void controller_holdLeftRightRumbleTick(holding_left_right, unsigned int);
bool controller_supportsTick(const game_controller_data);
//void controller_resetTickVariables();

//internal

//void clearControllerData(game_controller_data*);