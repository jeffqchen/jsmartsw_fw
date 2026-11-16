#include "controller_sony_ds3.h"
#include "controller_sony_ds4.h"
#include "controller_sony_dualsense.h"
#include "controller_sega_saturn_usb.h"
#include "controller_nintendo_switch.h"
//#include "controller_nintendo_switch_pro.h"
#include "controller_nintendo_switch2.h"
#include "controller_psx2usb.h"
#include "controller_md2x.h"
#include "controller_keyboard.h"
#include "controller_mouse.h"
#include "controller_8bitdo_retro_sfc.h"

#ifndef SUPPORTED_CONTROLLERS_H
#define SUPPORTED_CONTROLLERS_H

//controlelr api traverse index
enum {
  API_ID_EMPTY = 0,
  API_ID_KEYBOARD,
  API_ID_MOUSE,
  API_ID_DUALSHOCK3,
  API_ID_DUALSHOCK4,
  API_ID_DUALSENSE,
  API_ID_SWJOYCON,
  API_ID_SWITCHPRO,
  API_ID_SW2JOYCON,
  API_ID_SATURN_USB,
  API_ID_PSX2USB,
  API_ID_MD2X,
  API_ID_8BITDO_SFC,
  API_ID_END
};

#define NUM_OF_API  API_ID_END

#define API_NAME_STRING_ARRAY "Dummy",          \
                              "Keyboard",       \
                              "Mouse",          \
                              "DualShock 3",    \
                              "DualShock 4",    \
                              "DualSense",      \
                              "Switch JoyCon",  \
                              "Switch ProCon",  \
                              "Switch2 JoyCon", \
                              "Saturn USB",     \
                              "PSX2USB",        \
                              "MD2X",           \
                              "8bitdo Retro SFC"

#define FUNC_NOT_IMPLEMENTED  0

#endif

//function prototypes
void controller_init_deviceRegister();

//empty api
bool empty_mount(uint8_t, uint8_t);
void empty_umount(uint8_t, uint8_t);

void empty_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void empty_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void empty_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool empty_has_activity();

void empty_setupTick  (holding_left_right, uint8_t, uint8_t);
void empty_visualTick (holding_left_right, uint8_t, uint8_t);
void empty_tactileTick(holding_left_right, uint8_t, uint8_t);
void empty_executeTick(holding_left_right, uint8_t, uint8_t);

void empty_housekeepingRoutine();