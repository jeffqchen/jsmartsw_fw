#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_SONY_DS3_H
#define CONTROLLER_SONY_DS3_H

///////////////////////////////
//Controller definitions
///////////////////////////////

#define SONY_DS3_VID  0x054C
#define SONY_DS3_PID  0x0268

TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{
  //byte 0 is report ID
  //byte 1 is reserved
  uint8_t reserved;

  //byte 2
  uint8_t select      :1;
  uint8_t l3          :1;
  uint8_t r3          :1;
  uint8_t start       :1;
  uint8_t dpad_up     :1;
  uint8_t dpad_right  :1;
  uint8_t dpad_down   :1;
  uint8_t dpad_left   :1;

  //byte 3
  uint8_t l2          :1;
  uint8_t r2          :1;
  uint8_t l1          :1;
  uint8_t r1          :1;
  uint8_t triangle    :1;
  uint8_t circle      :1;
  uint8_t cross       :1;
  uint8_t square      :1;

  //byte 4
  uint8_t ps          :1;
  uint8_t unknonw1    :7;

  //byte 5
  uint8_t unknown2;

  //byte 6
  uint8_t lx;

  //byte 7
  uint8_t ly;

  //byte 8
  uint8_t rx;

  //byte 9
  uint8_t ry;
} sony_ds3_input_report_t;

typedef struct TU_ATTR_PACKED{
  uint8_t time_enabled;
  uint8_t duty_length;
  uint8_t enabled;
  uint8_t duty_off;
  uint8_t duty_on;
} ds3_led_parameters;

typedef struct {
  unsigned int maskValue;
  int stepSign;
} ds3_led_mask;

enum
{
  DS3_LED_1 = 0,
  DS3_LED_2,
  DS3_LED_3,
  DS3_LED_4,
  DS3_LED_END
};

#define DS3_LED_TIME_ENABLED    0xff
#define DS3_LED_DUTY_LENGTH     0
#define DS3_LED_ANIMATION_STEP  32
#define DS3_LED_DUTY_MIN        0
#define DS3_LED_DUTY_MAX        0xff
#define DS3_NUM_LEDS            DS3_LED_END

typedef struct TU_ATTR_PACKED {
  uint8_t padding1;             //byte 0

  //right motor
  uint8_t small_motor_duration; //byte 1
  uint8_t small_motor_strength; //byte 2

  //left motor
  uint8_t large_motor_duration; //byte 3
  uint8_t large_motor_strength; //byte 4

  uint8_t padding2[4];

  uint8_t led_bitmap;           //byte 9

  ds3_led_parameters led_parameter[DS3_NUM_LEDS];
  ds3_led_parameters reserved;

  uint8_t padding3[13];
} sony_ds3_output_report_01_t;

#define DS3_OUTPUT_REPORT_ID  0x1

#define DS3_LED_1_ON    0x02
#define DS3_LED_2_ON    0x04
#define DS3_LED_3_ON    0x08
#define DS3_LED_4_ON    0x10
#define DS3_LED_ALL_OFF 0x20

#define DS3_OUTPUT_REPORT_TEMPLATE  { 0x00,                         \
                                      0xFF, 0x00,                   \
                                      0xFF, 0x00,                   \
                                      0x00, 0x00, 0x00, 0x00,       \
                                      0x00,                         \
                                      0xFF, 0x27, 0x10, 0x00, 0x32, \
                                      0xFF, 0x27, 0x10, 0x00, 0x32, \
                                      0xFF, 0x27, 0x10, 0x00, 0x32, \
                                      0xFF, 0x27, 0x10, 0x00, 0x32, \
                                      0x00, 0x00, 0x00, 0x00, 0x00, \
                                      0x00, 0x00, 0x00, 0x00, 0x00, \
                                      0x00, 0x00, 0x00, 0x00, 0x00, \
                                      0x00, 0x00, 0x00              \
                                    }
#endif

//function prototype
//bool is_sony_ds3(uint8_t);
void ds3_defaultLedRumbleState();
void process_sony_ds3(uint8_t const*, uint16_t, uint8_t, uint8_t);

void ds3_mount(uint8_t, uint8_t);
void ds3_umount(uint8_t, uint8_t);

void ds3_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void ds3_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void ds3_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool ds3_has_activity();

void ds3_setupTick  (holding_left_right, uint8_t, uint8_t);
void ds3_visualTick  (holding_left_right, uint8_t, uint8_t);
void ds3_tactileTick(holding_left_right, uint8_t, uint8_t);
void ds3_executeTick (holding_left_right, uint8_t, uint8_t);
