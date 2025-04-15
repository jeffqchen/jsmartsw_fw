#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_SONY_DS4_H
#define CONTROLLER_SONY_DS4_H

///////////////////////////////
//Controller reports definition
///////////////////////////////
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

enum {
  SONY_DS4_1 = 0,
  SONY_DS4_2,
  SONY_DS4_WIRELESS_ADAPTER,
  HORI_FC4,
  HORI_PS4_MINI,
  ASW_GG_XRD,
  DS4_END
};

#define NUM_OF_DS4_DEVICES  DS4_END

#define DS4_SONY_VID                0x054C
  #define DS4_SONY_DS4_1            0x05C4
  #define DS4_SONY_DS4_2            0x09CC
  #define DS4_SONY_ADAPTER_PID      0x0BA0

#define DS4_HORI_VID                0x0F0D
  #define DS4_HORI_FC4_PID          0x005E
  #define DS4_HORI_PS4_MINI_PID     0x00EE

#define DS4_ASW_VID                 0x1F4F
  #define DS4_ASW_GG_XRD_PID        0x1002

#define DS4_VID_PID_ARRAY           { {DS4_SONY_VID, DS4_SONY_DS4_1},            \
                                      {DS4_SONY_VID, DS4_SONY_DS4_2},            \
                                      {DS4_SONY_VID, DS4_SONY_ADAPTER_PID}, \
                                      {DS4_HORI_VID, DS4_HORI_FC4_PID},         \
                                      {DS4_HORI_VID, DS4_HORI_PS4_MINI_PID},    \
                                      {DS4_ASW_VID , DS4_ASW_GG_XRD_PID}        }

typedef struct
{
  uint16_t vid;
  uint16_t pid;
} device_id_entry;

#define DS4_OUTPUT_REPORT_5 0x5

typedef struct TU_ATTR_PACKED
{
  //byte 1-4
  uint8_t x, y, z, rz; // joystick

  //byte 5
  struct {
    uint8_t dpad     : 4; // (hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)

    uint8_t square   : 1; // west
    uint8_t cross    : 1; // south
    uint8_t circle   : 1; // east
    uint8_t triangle : 1; // north
  };

  //byte 6
  struct {
    uint8_t l1     : 1;
    uint8_t r1     : 1;
    uint8_t l2     : 1;
    uint8_t r2     : 1;
    uint8_t share  : 1;
    uint8_t option : 1;
    uint8_t l3     : 1;
    uint8_t r3     : 1;
  };

  //byte 7
  struct {
    uint8_t ps      : 1; // playstation button
    uint8_t tpad    : 1; // track pad click
    uint8_t counter : 6; // +1 each report
  };

  //byte 8 & 9
  uint8_t l2_trigger; // 0 released, 0xff fully pressed
  uint8_t r2_trigger; // as above

} sony_ds4_report_t;

typedef struct TU_ATTR_PACKED {
  // First 16 bits set what data is pertinent in this structure (1 = set; 0 = not set)
  uint8_t set_rumble          :1;
  uint8_t set_led             :1;
  uint8_t set_led_blink       :1;
  uint8_t set_ext_write       :1;
  uint8_t set_left_volume     :1;
  uint8_t set_right_volume    :1;
  uint8_t set_mic_volume      :1;
  uint8_t set_speaker_volume  :1;

  uint8_t set_flags2;
  uint8_t reserved;

  uint8_t motor_right;
  uint8_t motor_left;

  uint8_t lightbar_red;
  uint8_t lightbar_green;
  uint8_t lightbar_blue;
  uint8_t lightbar_blink_on;
  uint8_t lightbar_blink_off;

  uint8_t ext_data[8];

  uint8_t volume_left;
  uint8_t volume_right;
  uint8_t volume_mic;
  uint8_t volume_speaker;

  uint8_t other[9];
} sony_ds4_output_report_5_t;

typedef struct {
  int red;
  int green;
  int blue;
} ds4_light_bar_color;
#define DS4_LIGHT_BAR_RGB_MIN       0
#define DS4_LIGHT_BAR_RGB_MAX       255
#define DS4_LIGHT_BAR_DEFAULT       {10,  25, 160}
#define DS4_LIGHT_BAR_RAINBOW_START {255, 0,  0}
#define DS4_LIGHT_BAR_FRACTION      2
#define DS4_RGB_STEP 16

#endif

//function prototypes
//bool is_sony_ds4(uint8_t);
void ds4_defaultLedRumbleState();
void process_sony_ds4(uint8_t const*, uint16_t, uint8_t, uint8_t);

void ds4_mount(uint8_t, uint8_t);
void ds4_umount(uint8_t, uint8_t);

void ds4_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void ds4_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void ds4_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool ds4_has_activity();

void ds4_setupTick  (holding_left_right, uint8_t, uint8_t);
void ds4_visualTick  (holding_left_right, uint8_t, uint8_t);
void ds4_tactileTick(holding_left_right, uint8_t, uint8_t);
void ds4_executeTick (holding_left_right, uint8_t, uint8_t);
