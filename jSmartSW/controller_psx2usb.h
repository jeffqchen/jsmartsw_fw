/*
PSX2USB
A USB HID adapter for PlayStation controllers
https://github.com/jeffqchen/PSX2USB_hardware
*/

//#include <sys/_stdint.h>
#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_PSX2USB_H
#define CONTROLLER_PSX2USB_H

///////////////////////////////
//Controller reports definition
///////////////////////////////
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{
  //byte 1
  uint8_t square     :1;
  uint8_t cross      :1;
  uint8_t circle     :1;
  uint8_t triangle   :1;

  uint8_t l1         :1;
  uint8_t r1         :1;
  uint8_t l2         :1;
  uint8_t r2         :1;

  //byte 2
  uint8_t select     :1;
  uint8_t start      :1;
  uint8_t            :1;
  uint8_t            :1;

  uint8_t            :1;
  uint8_t            :1;
  uint8_t            :1;
  uint8_t            :1;

  //byte 3
  uint8_t dpad       :4;
  uint8_t            :4;

  //byte 4
  uint16_t axis_x;  // 0x8001 to 7fff
  uint16_t axis_y;
} psx2usb_report_03_t;

TU_ATTR_PACKED_END
TU_ATTR_BIT_FIELD_ORDER_END

#endif

//bool is_psx2usb(uint8_t);

void psx2usb_mount(uint8_t, uint8_t);
void psx2usb_umount(uint8_t, uint8_t);

void psx2usb_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void psx2usb_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void psx2usb_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool psx2usb_has_activity();

void psx2usb_setupTick  (holding_left_right, uint8_t, uint8_t);
void psx2usb_visualTick  (holding_left_right, uint8_t, uint8_t);
void psx2usb_tactileTick(holding_left_right, uint8_t, uint8_t);
void psx2usb_executeTick (holding_left_right, uint8_t, uint8_t);
