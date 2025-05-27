#include <sys/_stdint.h>
//#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_KEYBOARD_H
#define CONTROLLER_KEYBOARD_H

///////////////////////////////
//Controller reports definition
///////////////////////////////
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{
  //byte 1
  uint8_t modifier;

  //byte 2
  uint8_t reserved;

  //byte 3
  uint8_t keycode[6];
} keyboard_report_t;

TU_ATTR_PACKED_END
TU_ATTR_BIT_FIELD_ORDER_END

#endif

//function prototype
//bool is_keyboard(uint8_t);
void process_keyboard(uint8_t const*, uint16_t, uint8_t, uint8_t);

void keyboard_mount(uint8_t, uint8_t);
void keyboard_umount(uint8_t, uint8_t);

void keyboard_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void keyboard_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);

void keyboard_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool keyboard_has_activity();

void keyboard_setupTick  (holding_left_right, uint8_t, uint8_t);
void keyboard_visualTick  (holding_left_right, uint8_t, uint8_t);
void keyboard_tactileTick(holding_left_right, uint8_t, uint8_t);
void keyboard_executeTick (holding_left_right, uint8_t, uint8_t);

keyboard_data keyboard_currentReading();
