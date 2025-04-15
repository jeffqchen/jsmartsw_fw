#include "mydebug.h"

#include "tusb.h"
#include "usb_controller.h"

#ifndef CONTROLLER_NINTENDO_SWITCH_PRO_H
#define CONTROLLER_NINTENDO_SWITCH_PRO_H

///////////////////////////////
//Controller reports definition
///////////////////////////////


// Nintendo Switch Pro Controller Reports
#define PRO_USB_CONNECTION_STATUS 0x01
#define PRO_USB_CMD_BAUDRATE_3M   0x03
#define PRO_USB_CMD_HANDSHAKE     0x02
#define PRO_USB_CMD_NO_TIMEOUT    0x04
#define PRO_USB_CMD_YES_TIMEOUT   0x05
#define PRO_USB_CMD_RESET         0x06
#define PRO_USB_PRE_HANDSHAKE     0x92
#define PRO_USB_SEND_UART         0x92

#define PRO_USB_RESET_WAIT  1000

#define SWPROCON_RUMBLE_DATA_SIZE           8
#define SWPROCON_RUMBLE_NONE                0x00, 0x01, 0x40, 0x40  //no freq, no amp
#define SWPROCON_RUMBLE_FREQ_LOW            0x00, 0x10
#define SWPROCON_RUMBLE_FREQ_HIGH           0xfc, 0x01
#define SWPROCON_RUMBLE_AMP_BIG             0xb4, 0x6d
#define SWPROCON_RUMBLE_AMP_SMALL           0x50, 0x54
#define SWPROCON_RUMBLE_DATA_DEFAULT        {SWPROCON_RUMBLE_NONE,                              SWPROCON_RUMBLE_NONE}
#define SWPROCON_RUMBLE_DATA_VIBRATE_LEFT   {SWPROCON_RUMBLE_FREQ_LOW, SWPROCON_RUMBLE_AMP_BIG, SWPROCON_RUMBLE_NONE}
#define SWPROCON_RUMBLE_DATA_VIBRATE_RIGHT  {SWPROCON_RUMBLE_NONE,                              SWPROCON_RUMBLE_FREQ_HIGH, SWPROCON_RUMBLE_AMP_SMALL}

///////////////////////////////
// Home LED data
#define HOME_LED_DATA_NUM_OF_MINI_CYCLES              0x2 //HIGH
#define HOME_LED_DATA_GLOBAL_MINI_CYCLE_DURATION_SLOW 0x6 //LOW
#define HOME_LED_DATA_GLOBAL_MINI_CYCLE_DURATION_FAST 0x1 //LOW

#define HOME_LED_DATA_START_INTENSITY                 0xf //HIGH
#define HOME_LED_DATA_NUM_OF_FULL_CYCLES              0x0 //LOW     0 = infinite

#define HOME_LED_MINI_CYCLE_TARGET_INTENSITY_DARK     0x0 //HIGH & LOW
#define HOME_LED_MINI_CYCLE_TARGET_INTENSITY_LIGHT    0xf //HIGH & LOW

#define HOME_LED_MINI_CYCLE_DURATION_FADING_SLOW      0xf //HIGH
#define HOME_LED_MINI_CYCLE_DURATION_STATIC_SLOW      0xf //LOW

#define HOME_LED_MINI_CYCLE_DURATION_FADING_FAST      0xa //HIGH
#define HOME_LED_MINI_CYCLE_DURATION_STATIC_FAST      0xa //LOW

#define SWPROCON_HOME_LED_DATA_DEFAULT  { HOME_LED_DATA_NUM_OF_MINI_CYCLES          << 4 | HOME_LED_DATA_GLOBAL_MINI_CYCLE_DURATION_SLOW, \
                                          HOME_LED_DATA_START_INTENSITY             << 4 | HOME_LED_DATA_NUM_OF_FULL_CYCLES,              \
                                          HOME_LED_MINI_CYCLE_TARGET_INTENSITY_DARK << 4 | HOME_LED_MINI_CYCLE_TARGET_INTENSITY_LIGHT,    \
                                          HOME_LED_MINI_CYCLE_DURATION_FADING_SLOW  << 4 | HOME_LED_MINI_CYCLE_DURATION_STATIC_SLOW,      \
                                          HOME_LED_MINI_CYCLE_DURATION_FADING_SLOW  << 4 | HOME_LED_MINI_CYCLE_DURATION_STATIC_SLOW,      \
                                          0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }  //slow breathing light
#define SWPROCON_HOME_LED_DATA_FAST     { HOME_LED_DATA_NUM_OF_MINI_CYCLES          << 4 | HOME_LED_DATA_GLOBAL_MINI_CYCLE_DURATION_FAST, \
                                          HOME_LED_DATA_START_INTENSITY             << 4 | HOME_LED_DATA_NUM_OF_FULL_CYCLES,              \
                                          HOME_LED_MINI_CYCLE_TARGET_INTENSITY_DARK << 4 | HOME_LED_MINI_CYCLE_TARGET_INTENSITY_LIGHT,    \
                                          HOME_LED_MINI_CYCLE_DURATION_FADING_FAST  << 4 | HOME_LED_MINI_CYCLE_DURATION_STATIC_FAST,      \
                                          HOME_LED_MINI_CYCLE_DURATION_FADING_FAST  << 4 | HOME_LED_MINI_CYCLE_DURATION_STATIC_FAST,      \
                                          0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }  //fast breathing light

#define SWPROCON_HOME_LED_DATA_SIZE     25
//////////////////////////////////////////
// Input reports
TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{
  uint16_t accel_x;
  uint16_t accel_y;
  uint16_t accel_z;

  uint16_t gyro_x;
  uint16_t gyro_y;
  uint16_t gyro_z;
} nintendo_switchProCon_imu_data;

typedef struct TU_ATTR_PACKED
{
  //byte1
  uint8_t counter;            //0 - 255

  //byte2
  uint8_t conn_info     :4;   // Connection info. (con_info >> 1) & 3
                              // 3=JC, 0=Pro/ChrGrip. con_info & 1 - 1=Switch/USB powered.
  uint8_t battery_level :4;   //Battery level. 8=full, 6=medium, 4=low, 2=critical, 0=empty. LSB=Charging.

  //byte 3
  struct {
    uint8_t button_y    : 1;
    uint8_t button_x    : 1;
    uint8_t button_b    : 1;
    uint8_t button_a    : 1;

    uint8_t button_sr_r : 1;  // N/A on pro, 0
    uint8_t button_sl_r : 1;  // N/A on pro, 0
    uint8_t button_r    : 1;
    uint8_t button_zr   : 1;
    //report count 2
  };

  //byte 4
  struct {
    uint8_t button_minus: 1;
    uint8_t button_plus : 1;
    uint8_t button_r3   : 1;
    uint8_t button_l3   : 1;

    uint8_t button_home : 1;
    uint8_t button_cap  : 1;
    uint8_t             : 1;  //  pad stuck at 0
    uint8_t             : 1;  //  pad stuck at 1
    //report count 2
  };

  //byte 5
  struct {
    uint8_t dpad_down   : 1;
    uint8_t dpad_up     : 1;
    uint8_t dpad_right  : 1;
    uint8_t dpad_left   : 1;

    uint8_t button_sr_l : 1;  //  stuck at 0, N/A on pro
    uint8_t button_sl_l : 1;  //  stuck at 0, N/A on pro
    uint8_t button_l    : 1;  //  l
    uint8_t button_zl   : 1;  //  zl
    //report count 4
  };

  //byte 6 to 12
  #pragma pack(push,1)
  struct {
    uint16_t lx         :12;
    uint16_t ly         :12;
    uint16_t rx         :12;
    uint16_t ry         :12;
  };
  #pragma pack(pop)

  uint8_t vibrator_input_report;
} nintendo_switchProCon_input_report_common_data;

typedef struct TU_ATTR_PACKED
{
  nintendo_switchProCon_input_report_common_data commonData;

  nintendo_switchProCon_imu_data imu_data[3];

  uint8_t pad_end[15];
} nintendo_switchProCon_input_report_30_t;

typedef struct TU_ATTR_PACKED
{
  //to byte 12
  nintendo_switchProCon_input_report_common_data commonData;

  //byte 13
  uint8_t ack_byte; // ACK byte for subcmd reply.
                    // ACK: MSB is 1, NACK: MSB is 0.
                    // If reply is ACK and has data, byte12 & 0x7F gives as the type of data.
                    // If simple ACK or NACK, the data type portion is x00

  //byte 14
  uint8_t reply_subCmd_id; // replying to which subcommand id

  //byte 15 - 49
  uint8_t subCmd_reply_data[35];

  //padding at the end.
  uint8_t pad_end[14];
} nintendo_switchProCon_input_report_21_t;

//////////////////////////////////////////
// Output Report
typedef struct TU_ATTR_PACKED
{
  uint8_t cmd;
} nintendo_switchProCon_output_report_80_t;

//////////////////////////////////////////
// Subcommand
typedef struct TU_ATTR_PACKED
{
  uint8_t packetNum;
  uint8_t rumbleData[SWPROCON_RUMBLE_DATA_SIZE];
  uint8_t subCmdId;
  uint8_t data[];
} nintendo_switchProCon_subCmd_report_t;

typedef struct TU_ATTR_PACKED
{
  uint8_t packetNum;
  uint8_t rumbleData[SWPROCON_RUMBLE_DATA_SIZE];
} nintendo_switchProCon_rumble_report_t;

typedef struct TU_ATTR_PACKED
{
  //byte 1
  uint8_t mini_cycle_2_intensity        :4;
  uint8_t mini_cycle_1_intensity        :4;

  //byte 2
  uint8_t mini_cycle_1_duration_multiplayer       :4; // 0 = 1 times
  uint8_t mini_cycle_1_fading_transition_duration :4;

  //byte 3
  uint8_t mini_cycle_2_duration_multiplayer       :4;
  uint8_t mini_cycle_2_fading_transition_duration :4;
} nintendo_switchProCon_home_led_mini_cycle_data;

typedef struct TU_ATTR_PACKED
{
  //byte 0
  uint8_t global_mini_cycle_duration    :4; //0=off, 8 - 175ms
  uint8_t global_mini_cycle_num         :4;

  //byte 1
  uint8_t global_full_cycle_num         :4;
  uint8_t start_intensity               :4; //0xf = 100%

  //byte 2
  nintendo_switchProCon_home_led_mini_cycle_data mini_cycle_array[7];

  //byte 23
  uint8_t padding1                      :4;
  uint8_t mini_cycle_15_intensity       :4;

  //byte 24
  uint8_t mini_cycle_15_duration_multiplayer       :4;
  uint8_t mini_cycle_15_fading_transition_duration :4;
} nintendo_switchProCon_home_led_data;  //25bytes
// Reference: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_subcommands_notes.md

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

#define SWPROCON_REPORT_INTERVAL          47 //(ms)

//output report ids
#define SWPROCON_OUTPUT_RUMBLE_AND_SUBCMD 0x01
#define SWPROCON_OUTPUT_RUMBLE_ONLY       0x10
#define SWPROCON_OUTPUT_USB_CMD           0x80

//sub commands
#define SWPROCON_SUBCMD_STATE                           0x0
#define SWPROCON_SUBCMD_DATA_SIZE_STATE                 1

#define SWPROCON_SUBCMD_QUERY_DEV_INFO                  0x02
#define SWPROCON_SUBCMD_DATA_SIZE_QUERY_DEV_INFO        1

#define SWPROCON_SUBCMD_SET_REPORT_MODE                 0x03
#define SWPROCON_SUBCMD_DATA_SIZE_SET_REPORT_MODE       2

#define SWPROCON_SUBCMD_SET_HCI_STATE                   0x06
#define SWPROCON_SUBCMD_DATA_SIZE_SET_HCI_STATE         2

#define SWPROCON_SUBCMD_HOME_LED                        0x38
#define SWPROCON_SUBCMD_DATA_SIZE_HOME_LED              26

#define SWPROCON_SUBCMD_SET_PLAYER_LEDS                 0x30
#define SWPROCON_SUBCMD_DATA_SIZE_SET_PLAYER_LEDS       2

#define SWPROCON_SUBCMD_SET_IMU                         0x40
#define SWPROCON_SUBCMD_DATA_SIZE_SET_IMU               2

#define SWPROCON_SUBCMD_ENABLE_VIBRATION                0x48
#define SWPROCON_SUBCMD_DATA_SIZE_SET_ENABLE_VIBRATION  2

#define SWPROCON_NO_DATA  NULL

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
