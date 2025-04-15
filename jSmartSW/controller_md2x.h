/*
Genesis 2X Controller to USB Adapter
A USB HID adapter that supports two Genesis/MegaDrive controllers at the same time.
https://github.com/jeffqchen/Genesis-2X-Controller-to-USB-Adapter
*/

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_SEGA_md2x_H
#define CONTROLLER_SEGA_md2x_H

///////////////////////////////
//Controller reports definition
///////////////////////////////
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{
  //byte 1
  uint8_t button_a      :1;
  uint8_t button_b      :1;
  uint8_t button_c      :1;
  uint8_t button_x      :1;

  uint8_t button_y      :1;
  uint8_t button_z      :1;
  uint8_t button_start  :1;
  uint8_t button_mode   :1;

  //byte 2
  uint8_t something;

  //byte 3
  uint8_t dpad_leftRight;

  //byte 4
  uint8_t dpad_upDown;
} md2x_report_t;

typedef struct
{
  uint8_t instance;
  bool  mounted;
} md2x_instance;

#endif

//function prototype
//bool is_md2x(uint8_t);
void process_md2x(uint8_t const*, uint16_t, uint8_t, uint8_t);

void md2x_mount(uint8_t, uint8_t);
void md2x_umount(uint8_t, uint8_t);

void md2x_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void md2x_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);

void md2x_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool md2x_has_activity();

void md2x_setupTick  (holding_left_right, uint8_t, uint8_t);
void md2x_visualTick  (holding_left_right, uint8_t, uint8_t);
void md2x_tactileTick(holding_left_right, uint8_t, uint8_t);
void md2x_executeTick (holding_left_right, uint8_t, uint8_t);
