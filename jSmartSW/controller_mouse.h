#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_MOUSE_H
#define CONTROLLER_MOUSE_H

///////////////////////////////
//Controller reports definition
///////////////////////////////
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{
  //byte 1
  uint8_t button;

  //byte 2
  uint8_t axis_x;

  //byte 3
  uint8_t axis_y;

  //byte 4
  uint8_t wheel;
} mouse_input_report_t;

TU_ATTR_PACKED_END
TU_ATTR_BIT_FIELD_ORDER_END

#define MOUSE_BUTTON_NONE 0x00
#define MOUSE_BUTTON_L    0x01
#define MOUSE_BUTTON_R    0x02
#define MOUSE_BUTTON_MID  0x04
#define MOUSE_BUTTON_4    0x08
#define MOUSE_BUTTON_5    0x10

#define MOUSE_WHEEL_NEUTRAL 0x00
#define MOUSE_WHEEL_UP      0x01
#define MOUSE_WHEEL_DOWN    0xff

#define MOUSE_AXIS_NEUTRAL              0x00
#define MOUSE_AXIS_MIDDLE               0x80
#define MOUSE_AXIS_DEADZONE_UP          0xee
#define MOUSE_AXIS_DEADZONE_LEFT        0xee
#define MOUSE_AXIS_DEADZONE_DOWN        0x12
#define MOUSE_AXIS_DEADZONE_RIGHT       0x12

#define MOUSE_BUTTON_DEBOUNCE_TIME  50
#define MOUSE_TIMEOUT_THRESHOLD     100 //ms

#endif

//function prototype
void mouse_timeout();

//bool is_keyboard(uint8_t);
void process_mouse(uint8_t const*, uint16_t, uint8_t, uint8_t);

void mouse_mount(uint8_t, uint8_t);
void mouse_umount(uint8_t, uint8_t);

void mouse_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void mouse_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);

void mouse_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool mouse_has_activity();

void mouse_setupTick  (holding_left_right, uint8_t, uint8_t);
void mouse_visualTick  (holding_left_right, uint8_t, uint8_t);
void mouse_tactileTick(holding_left_right, uint8_t, uint8_t);
void mouse_executeTick (holding_left_right, uint8_t, uint8_t);

game_controller_data mouse_currentReading();
