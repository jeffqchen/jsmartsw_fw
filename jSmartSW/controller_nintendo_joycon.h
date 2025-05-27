#include <sys/_stdint.h>
#include "common/tusb_compiler.h"
#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"
#include "nintendo_switch_controller.h"

#ifndef CONTROLLER_NINTENDO_JOYCON_H
#define CONTROLLER_NINTENDO_JOYCON_H


enum {
  JOYCON_IDX_LEFT = 0x0,
  JOYCON_IDX_RIGHT,
  JOYCON_IDX_PRO,
  JOYCON_IDX_END
};

#define NUM_OF_CONTROLLERS  JOYCON_IDX_END

#define JOYCON_ANALOG_UP_RIGHT_ACTIVE_VALUE  2500
#define JOYCON_ANALOG_DOWN_LEFT_ACTIVE_VALUE 1500

#endif

//function prototype

//API
void switchJoyCon_mount(uint8_t, uint8_t);
void switchJoyCon_umount(uint8_t, uint8_t);

void switchJoyCon_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void switchJoyCon_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void switchJoyCon_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool switchJoyCon_has_activity();

void switchJoyCon_setupTick  (holding_left_right, uint8_t, uint8_t);
void switchJoyCon_visualTick  (holding_left_right, uint8_t, uint8_t);
void switchJoyCon_tactileTick(holding_left_right, uint8_t, uint8_t);
void switchJoyCon_executeTick (holding_left_right, uint8_t, uint8_t);