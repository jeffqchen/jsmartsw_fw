#include "common/tusb_compiler.h"
#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_NINTENDO_SWITCH_PRO_H
#define CONTROLLER_NINTENDO_SWITCH_PRO_H

struct switch_pro_init_states
{
  bool switchPro_baudRateSet;

  bool hciSleeping;
  bool rumbleEnabled;
  bool imuEnabled;
  bool fullReportMode;

  bool homeLedSet;
  bool playerLedSet;

  switch_pro_init_states() {
    hciSleeping = rumbleEnabled = imuEnabled = fullReportMode = homeLedSet = playerLedSet = false;
  }
};

//analog stick values
#define PRO_CON_ANALOG_UP_RIGHT_ACTIVE_VALUE  3000
#define PRO_CON_ANALOG_DOWN_LEFT_ACTIVE_VALUE 1000

////////////////////////////////////
//8bitdo controller button renaming
#define M30_8BITDO_BUTTON_A         m30_8bitdo_report.button_a
#define M30_8BITDO_BUTTON_B         m30_8bitdo_report.button_b
#define M30_8BITDO_BUTTON_C         m30_8bitdo_report.button_r
#define M30_8BITDO_BUTTON_X         m30_8bitdo_report.button_x
#define M30_8BITDO_BUTTON_Y         m30_8bitdo_report.button_y
#define M30_8BITDO_BUTTON_Z         m30_8bitdo_report.button_l

#define M30_8BITDO_BUTTON_START     m30_8bitdo_report.button_plus
#define M30_8BITDO_BUTTON_STAR      m30_8bitdo_report.button_cap
#define M30_8BITDO_BUTTON_MINUS     m30_8bitdo_report.button_minus
#define M30_8BITDO_BUTTON_HEART     m30_8bitdo_report.button_home

#define M30_8BITDO_BUTTON_L         m30_8bitdo_report.button_zl
#define M30_8BITDO_BUTTON_R         m30_8bitdo_report.button_zr

#define M30_8BITDO_DIRECTION_UP     (m30_8bitdo_report.ly > PRO_CON_ANALOG_UP_RIGHT_ACTIVE_VALUE  )
#define M30_8BITDO_DIRECTION_DOWN   (m30_8bitdo_report.ly < PRO_CON_ANALOG_DOWN_LEFT_ACTIVE_VALUE )
#define M30_8BITDO_DIRECTION_LEFT   (m30_8bitdo_report.lx < PRO_CON_ANALOG_DOWN_LEFT_ACTIVE_VALUE )
#define M30_8BITDO_DIRECTION_RIGHT  (m30_8bitdo_report.lx > PRO_CON_ANALOG_UP_RIGHT_ACTIVE_VALUE  )

#endif

//function prototype

//API
void switchProCon_mount(uint8_t, uint8_t);
void switchProCon_umount(uint8_t, uint8_t);

void switchProCon_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void switchProCon_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void switchProCon_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool switchProCon_has_activity();

void switchProCon_setupTick  (holding_left_right, uint8_t, uint8_t);
void switchProCon_visualTick  (holding_left_right, uint8_t, uint8_t);
void switchProCon_tactileTick(holding_left_right, uint8_t, uint8_t);
void switchProCon_executeTick (holding_left_right, uint8_t, uint8_t);

///////////////////
//bool switchProCon_setPlayerLedNum(const uint8_t*, const uint8_t*);
