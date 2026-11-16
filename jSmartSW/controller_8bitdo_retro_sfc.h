#include <stdint.h>
#include "mydebug.h"

//#include "tusb.h"
#include "usb_hid.h"

#ifndef CONTROLLER_8BITDO_RETRO_SFC_H
#define CONTROLLER_8BITDO_RETRO_SFC_H

///////////////////////////////
//Controller reports definition
///////////////////////////////
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{ //9 bytes [00] 00 00 0f 7f 7f 7f 7f 00 00
  //byte 1
  uint8_t button_a    :1;
  uint8_t button_b    :1;
  uint8_t             :1;
  uint8_t button_x    :1;

  uint8_t button_y    :1;
  uint8_t             :1;
  uint8_t button_l    :1;
  uint8_t button_r    :1;

  //byte 2
  uint8_t               :1;
  uint8_t               :1;
  uint8_t button_select :1;
  uint8_t button_start  :1;

  //byte 3
  uint8_t unkonwn_1;

  //byte 4
  uint8_t x_axis;

  //byte 5
  uint8_t y_axis;

  //byte 6
  uint8_t z_axis;

  //byte 7
  uint8_t rz_axis;

  //byte 8
  uint8_t unkonwn_2;

  //byte 9
  uint8_t unkonwn_3;
} retro_sfc_8bitdo_report_t;

TU_ATTR_PACKED_END
TU_ATTR_BIT_FIELD_ORDER_END

#define RETRO_SFC_8BITDO_AXIS_NEUTRAL 0x7F
#define RETRO_SFC_8BITDO_AXIS_MIN 0x0
#define RETRO_SFC_8BITDO_AXIS_MAX 0xff

#endif

//function prototype
//bool is_saturn_usb(uint8_t);
void process_retro_sfc_8bitdo_(uint8_t const*, uint16_t, uint8_t, uint8_t);

bool retro_sfc_8bitdo_mount(uint8_t, uint8_t);
void retro_sfc_8bitdo_umount(uint8_t, uint8_t);

void retro_sfc_8bitdo_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void retro_sfc_8bitdo_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void retro_sfc_8bitdo_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool retro_sfc_8bitdo_has_activity();

void retro_sfc_8bitdo_setupTick  (holding_left_right, uint8_t, uint8_t);
void retro_sfc_8bitdo_visualTick  (holding_left_right, uint8_t, uint8_t);
void retro_sfc_8bitdo_tactileTick(holding_left_right, uint8_t, uint8_t);
void retro_sfc_8bitdo_executeTick (holding_left_right, uint8_t, uint8_t);

void retro_sfc_8bitdo_housekeepingRoutine ();
