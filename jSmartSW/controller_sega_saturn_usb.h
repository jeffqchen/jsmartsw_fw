#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_SEGA_SATURN_USB_H
#define CONTROLLER_SEGA_SATURN_USB_H

///////////////////////////////
//Controller reports definition
///////////////////////////////
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{
  //byte 1
  uint8_t dpad_leftRight;

  //byte 2
  uint8_t dpad_upDown;

  //byte 3
  uint8_t button_a    :1;
  uint8_t button_b    :1;
  uint8_t button_c    :1;
  uint8_t button_x    :1;

  uint8_t button_y    :1;
  uint8_t button_z    :1;
  uint8_t button_l    :1;
  uint8_t button_r    :1;

  //byte 4
  uint8_t button_start;
} saturn_usb_report_t;

#define SATURN_USB_DPAD_NEUTRAL 0x80

#endif

//function prototype
//bool is_saturn_usb(uint8_t);
void process_saturn_usb(uint8_t const*, uint16_t, uint8_t, uint8_t);

void saturn_usb_mount(uint8_t, uint8_t);
void saturn_usb_umount(uint8_t, uint8_t);

void saturn_usb_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void saturn_usb_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void saturn_usb_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool saturn_usb_has_activity();

void saturn_usb_setupTick  (holding_left_right, uint8_t, uint8_t);
void saturn_usb_visualTick  (holding_left_right, uint8_t, uint8_t);
void saturn_usb_tactileTick(holding_left_right, uint8_t, uint8_t);
void saturn_usb_executeTick (holding_left_right, uint8_t, uint8_t);
