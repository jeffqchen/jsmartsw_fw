#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_SONY_DUALSENSE_H
#define CONTROLLER_SONY_DUALSENSE_H

///////////////////////////////
//Controller reports definition
///////////////////////////////
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

// Sony DualSense Reports
#define DUALSENSE_OUTPUT_REPORT_2 0x02

typedef struct TU_ATTR_PACKED
{
  //byte 1 to 7
  uint8_t x, y, z, rz, rx, ry;// joystick
  //report count 6
  uint8_t venderDefined1;
  //report count 1

  //byte 8
  struct {
    uint8_t dpad     : 4; // (hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)
    //report count 1

    uint8_t square   : 1; // west
    uint8_t cross    : 1; // south
    uint8_t circle   : 1; // east
    uint8_t triangle : 1; // north
  };

  //byte 9
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

  //byte 10
  struct {
    uint8_t ps      : 1; // playstation button
    uint8_t tpad    : 1; // track pad click
    uint8_t mute    : 1; // mute button
    //report count 15

    uint8_t counter : 5; // +1 each report
    uint8_t vendorDefined3[13-1];
    //report count 130
  };

  //byte 12
  //uint8_t vendorDefined4[52];
  //report count 52

} sony_dualSense_input_report_t;

typedef struct TU_ATTR_PACKED {
  //byte 1
  struct {
    // First 16 bits set what data is pertinent in this structure (1 = set; 0 = not set)
    uint8_t set_rumble_graceful     : 1;
    uint8_t set_rumble_continued    : 1;

    uint8_t set_right_trigger_motor : 1;
    uint8_t set_left_trigger_motor  : 1;

    uint8_t change_audio_volume     : 1;
    uint8_t toggle_speaker          : 1;
    uint8_t set_mic_volume          : 1;

    uint8_t set_audio_control_1      : 1;
  };

  //byte 2
  struct {
    uint8_t toggle_mic_led      : 1;
    uint8_t toggle_mute         : 1;
    uint8_t toggle_tpad_led     : 1;
    uint8_t led_blackout        : 1;
    uint8_t toggle_player_led   : 1;
    uint8_t haptic_lpf          : 1;
    uint8_t haptic_adjust       : 1;
    uint8_t set_audio_control_2 : 1;
  };

  //byte 3 & 4
  //low freq motors, 0x-255
  uint8_t motor_right;
  uint8_t motor_left;

  //byte 5 - 8
  uint8_t vol_headphone;
  uint8_t vol_speaker;
  uint8_t vol_mic;

  uint8_t mic_select            :2; // 0 = auto, 1 = internal, 2 = external
  uint8_t echo_cancel           :1;
  uint8_t noise_cancel          :1;
  uint8_t output_path           :2;
  uint8_t input_path            :2;

  //byte 9
  uint8_t mic_led;

  //byte 10
  uint8_t powersave_touch       :1;
  uint8_t powersave_motion      :1;
  uint8_t powersave_haptic      :1;
  uint8_t powersave_audio       :1;
  uint8_t mute_mic              :1;
  uint8_t mute_speaker          :1;
  uint8_t mute_headphone        :1;
  uint8_t mute_haptic           :1;

  //byte 11 - 21
  uint8_t right_trigger_mode;
  uint8_t right_trigger_forces[10];

  //byte 22 - 32
  uint8_t left_trigger_mode;
  uint8_t left_trigger_forces[10];

  //byte 33
  uint32_t time_stamp;

  //byte 37
  uint8_t motor_power_reduce_trigger  :4;
  uint8_t motor_power_reduce_rumble   :4;

  //byte 38
  uint8_t speaker_boost         :3;
  uint8_t enable_beam_forming   :1;
  uint8_t unkonwn_audio_control :4;

  //byte 39
  uint8_t allow_light_brightness_change :1;
  uint8_t allow_touchpad_led_fade       :1;
  uint8_t improved_rumble_emulation     :1;
  uint8_t unknown_5_bits                :5;

  //byte 40
  uint8_t haptic_lfp            :1;
  uint8_t unknown_7_bits        :7;

  //byte 41
  uint8_t unknown_byte_41;

  //byte 42
  uint8_t led_pulse_option;

  //byte 43
  uint8_t led_brightness;

  //byte 44
  uint8_t led_player_num        :5;
  uint8_t player_led_fade       :1;
  uint8_t unknown_2_bits        :2;

  uint8_t led_tpad_color_red;
  uint8_t led_tpad_color_green;
  uint8_t led_tpad_color_blue;
} sony_dualSense_output_report_02_t;

TU_ATTR_PACKED_END
TU_ATTR_BIT_FIELD_ORDER_END

#endif

//function prototypes
//bool is_sony_dualSense(uint8_t);
void dualSense_defaultLedRumbleState();
void process_sony_dualSense (uint8_t const*, uint16_t, uint8_t, uint8_t);

void dualSense_mount(uint8_t, uint8_t);
void dualSense_umount(uint8_t, uint8_t);

void dualSense_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void dualSense_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void dualSense_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool dualSense_has_activity();

void dualSense_setupTick  (holding_left_right, uint8_t, uint8_t);
void dualSense_visualTick  (holding_left_right, uint8_t, uint8_t);
void dualSense_tactileTick(holding_left_right, uint8_t, uint8_t);
void dualSense_executeTick (holding_left_right, uint8_t, uint8_t);
