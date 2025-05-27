#include <sys/types.h>
#include <sys/_stdint.h>
#include "common/tusb_compiler.h"

#ifndef NINTENDO_SWITCH_DEFINITIONS_H
#define NINTENDO_SWITCH_DEFINITIONS_H

///////////////////////////////
//Controller reports definition
///////////////////////////////

#define SWITCH_SEND_REPORT_INTERVAL          40 //(ms)
#define SWITCH_POLL_INTERVAL            25

#define MAC_ADDR_NUM_OF_BYTES           6
#define SWITCH_DEV_ADDR_UNDEFINED       255
#define SWITCH_DEV_INSTANCE_UNDEFINED   255

//report ids
#define SWITCH_OUTPUT_RUMBLE_AND_SUBCMD 0x01
#define SWITCH_OUTPUT_RUMBLE_ONLY       0x10
#define SWITCH_OUTPUT_USB_CMD           0x80

#define SWITCH_ERROR_RESPONSE           0x00
#define SWITCH_INPUT_DATA               0x21
#define SWITCH_RESPONSE                 0x81

#define SWITCH_JOYCON_ADD               0x00
#define SWITCH_JOYCON_REMOVE            0x03

//sub commands
#define SWITCH_SUBCMD_STATE                           0x0
#define SWITCH_SUBCMD_DATA_SIZE_STATE                 1

#define SWITCH_SUBCMD_QUERY_DEV_INFO                  0x02
#define SWITCH_SUBCMD_DATA_SIZE_QUERY_DEV_INFO        1
#define SWITCH_SUBCMD_QUERY_DEV_INFO_RESPONSE         0x0282

#define SWITCH_SUBCMD_SET_REPORT_MODE                 0x03
#define SWITCH_SUBCMD_DATA_SIZE_SET_REPORT_MODE       2

#define SWITCH_SUBCMD_SET_HCI_STATE                   0x06
#define SWITCH_SUBCMD_DATA_SIZE_SET_HCI_STATE         2

#define SWITCH_SUBCMD_HOME_LED                        0x38
#define SWITCH_SUBCMD_DATA_SIZE_HOME_LED              26

#define SWITCH_SUBCMD_SET_PLAYER_LEDS                 0x30
#define SWITCH_SUBCMD_DATA_SIZE_SET_PLAYER_LEDS       2

#define SWITCH_SUBCMD_SET_IMU                         0x40
#define SWITCH_SUBCMD_DATA_SIZE_SET_IMU               2

#define SWITCH_SUBCMD_ENABLE_VIBRATION                0x48
#define SWITCH_SUBCMD_DATA_SIZE_SET_ENABLE_VIBRATION  2

#define SWITCH_NO_DATA  NULL

// Nintendo Switch Pro Controller Reports
#define SWITCH_USB_CONNECTION_STATUS 0x01
#define SWITCH_USB_CMD_BAUDRATE_3M   0x03
#define SWITCH_USB_CMD_HANDSHAKE     0x02
#define SWITCH_USB_CMD_NO_TIMEOUT    0x04
#define SWITCH_USB_CMD_YES_TIMEOUT   0x05
#define SWITCH_USB_CMD_RESET         0x06
#define SWITCH_USB_CMD_MCU_RESET     0x20
#define SWITCH_USB_PRE_HANDSHAKE     0x92
#define SWITCH_USB_SEND_UART         0x92

#define SWITCH_USB_RESET_WAIT  1000

#define SWITCH_RUMBLE_DATA_SIZE           8
#define SWITCH_RUMBLE_NONE                0x00, 0x01, 0x40, 0x40  //no freq, no amp
#define SWITCH_RUMBLE_FREQ_LOW            0x00, 0x10
#define SWITCH_RUMBLE_FREQ_HIGH           0xfc, 0x01
#define SWITCH_RUMBLE_AMP_BIG             0xb4, 0x6d
#define SWITCH_RUMBLE_AMP_SMALL           0x50, 0x54
#define SWITCH_RUMBLE_DATA_DEFAULT        {SWITCH_RUMBLE_NONE,                              SWITCH_RUMBLE_NONE}
#define SWITCH_RUMBLE_DATA_VIBRATE_LEFT   {SWITCH_RUMBLE_FREQ_LOW, SWITCH_RUMBLE_AMP_BIG, SWITCH_RUMBLE_NONE}
#define SWITCH_RUMBLE_DATA_VIBRATE_RIGHT  {SWITCH_RUMBLE_NONE,                              SWITCH_RUMBLE_FREQ_HIGH, SWITCH_RUMBLE_AMP_SMALL}

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

#define SWITCH_HOME_LED_DATA_DEFAULT    { HOME_LED_DATA_NUM_OF_MINI_CYCLES          << 4 | HOME_LED_DATA_GLOBAL_MINI_CYCLE_DURATION_SLOW, \
                                          HOME_LED_DATA_START_INTENSITY             << 4 | HOME_LED_DATA_NUM_OF_FULL_CYCLES,              \
                                          HOME_LED_MINI_CYCLE_TARGET_INTENSITY_DARK << 4 | HOME_LED_MINI_CYCLE_TARGET_INTENSITY_LIGHT,    \
                                          HOME_LED_MINI_CYCLE_DURATION_FADING_SLOW  << 4 | HOME_LED_MINI_CYCLE_DURATION_STATIC_SLOW,      \
                                          HOME_LED_MINI_CYCLE_DURATION_FADING_SLOW  << 4 | HOME_LED_MINI_CYCLE_DURATION_STATIC_SLOW,      \
                                          0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }  //slow breathing light
                                          
#define SWITCH_HOME_LED_DATA_FAST       { HOME_LED_DATA_NUM_OF_MINI_CYCLES          << 4 | HOME_LED_DATA_GLOBAL_MINI_CYCLE_DURATION_FAST, \
                                          HOME_LED_DATA_START_INTENSITY             << 4 | HOME_LED_DATA_NUM_OF_FULL_CYCLES,              \
                                          HOME_LED_MINI_CYCLE_TARGET_INTENSITY_DARK << 4 | HOME_LED_MINI_CYCLE_TARGET_INTENSITY_LIGHT,    \
                                          HOME_LED_MINI_CYCLE_DURATION_FADING_FAST  << 4 | HOME_LED_MINI_CYCLE_DURATION_STATIC_FAST,      \
                                          HOME_LED_MINI_CYCLE_DURATION_FADING_FAST  << 4 | HOME_LED_MINI_CYCLE_DURATION_STATIC_FAST,      \
                                          0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }  //fast breathing light

#define SWITCH_HOME_LED_DATA_SIZE     25

// Controller type
enum {
  SWITCH_ID_JOYCON_LEFT   = 0x1,
  SWITCH_ID_JOYCON_RIGHT,
  SWITCH_ID_PROCON
};

enum {
  BATTERY_LEVEL_CRITICAL  = 0x0,
  BATTERY_LEVEL_LOW       = 0x1,
  BATTERY_LEVEL_MEDIUM    = 0x2,
  BATTERY_LEVEL_FULL      = 0x4
};

//////////////////////////////////////////
// Input reports
//////////////////////////////////////////

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
} nintendo_switch_imu_data;

typedef struct TU_ATTR_PACKED
{
  uint8_t fw_ver_major;
  uint8_t fw_ver_minor;

  uint8_t controllerType;
  
  uint8_t unkown1;

  uint8_t macAddrBE[6];     // joycon MAC address

  uint8_t unkown2;

  uint8_t colorsInSPI;
} nintendo_switch_respond_report_dev_info_t;

typedef struct TU_ATTR_PACKED
{
  //byte1
  uint8_t counter;            //0 - 255

  //byte2
  uint8_t conn_info     :4;   // Connection info. (con_info >> 1) & 3
                              // 3=JC, 0=Pro/ChrGrip. con_info & 1 - 1=Switch/USB powered.
  uint8_t powerState    :4;   //Battery level. 8=full, 6=medium, 4=low, 2=critical, 0=empty. LSB=Charging.

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

  uint16_t responseType;
  union {
    nintendo_switch_respond_report_dev_info_t devInfo;
  };
} nintendo_switch_input_report_common_data;

typedef struct TU_ATTR_PACKED
{
  nintendo_switch_input_report_common_data commonData;

  nintendo_switch_imu_data imu_data[3];

  uint8_t pad_end[15];
} nintendo_switch_input_report_30_t;

typedef struct TU_ATTR_PACKED
{
  //to byte 12
  nintendo_switch_input_report_common_data commonData;

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
} nintendo_switch_input_report_21_t;

//////////////////////////////////////////
// Output Report
typedef struct TU_ATTR_PACKED
{
  uint8_t cmd;
} nintendo_switch_output_report_80_t;

//////////////////////////////////////////
// Response
typedef struct TU_ATTR_PACKED
{
  uint8_t controllerType; // 01 - JoyCon left, 02 - JoyCon right, 03 - Pro Controller
  uint8_t macAddrLE[6];     // joycon MAC address
} joycon_ident;

typedef struct TU_ATTR_PACKED
{
  uint8_t response;   // 0x81
  uint8_t status;     // 0x01
  uint8_t action;     // 00 add joycon, 03 remove joycon
  joycon_ident identity;
} nintendo_switch_respond_report_conn_status_t;

//////////////////////////////////////////
// Subcommand
typedef struct TU_ATTR_PACKED
{
  uint8_t packetNum;
  uint8_t rumbleData[SWITCH_RUMBLE_DATA_SIZE];
  uint8_t subCmdId;
  uint8_t data[];
} nintendo_switch_subCmd_report_t;

typedef struct TU_ATTR_PACKED
{
  uint8_t packetNum;
  uint8_t rumbleData[SWITCH_RUMBLE_DATA_SIZE];
} nintendo_switch_rumble_report_t;

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
} nintendo_switch_home_led_mini_cycle_data;

typedef struct TU_ATTR_PACKED
{
  //byte 0
  uint8_t global_mini_cycle_duration    :4; //0=off, 8 - 175ms
  uint8_t global_mini_cycle_num         :4;

  //byte 1
  uint8_t global_full_cycle_num         :4;
  uint8_t start_intensity               :4; //0xf = 100%

  //byte 2
  nintendo_switch_home_led_mini_cycle_data mini_cycle_array[7];

  //byte 23
  uint8_t padding1                      :4;
  uint8_t mini_cycle_15_intensity       :4;

  //byte 24
  uint8_t mini_cycle_15_duration_multiplayer       :4;
  uint8_t mini_cycle_15_fading_transition_duration :4;
} nintendo_switch_home_led_data;  //25bytes
// Reference: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_subcommands_notes.md

TU_ATTR_PACKED_END
TU_ATTR_BIT_FIELD_ORDER_END

struct switch_controller_init_states
{
  bool baudRateSet;
  bool devInfoAcquired;

  bool hciSleeping;
  bool rumbleEnabled;
  bool imuEnabled;
  bool fullReportMode;

  bool homeLedSet;
  bool playerLedSet;

  unsigned long lastReportSendTime;

  switch_controller_init_states() {
    devInfoAcquired = hciSleeping = rumbleEnabled = imuEnabled = fullReportMode = homeLedSet = playerLedSet = false;
    lastReportSendTime = 0;
  }
};

struct switchController_instance
{
  int controllerType;
  uint8_t dev_addr;
  uint8_t dev_instance;
  uint8_t macAddrLE[MAC_ADDR_NUM_OF_BYTES];
  uint8_t chargeLevel;
  bool isCharging;

  bool devReinitilized;
  bool present;
  
  switch_controller_init_states initState;

  uint8_t playerLedData;
  const uint8_t *rumbleData;
  const uint8_t *homeLedData;

  bool reportUpdated;

  /////////////////////////
  // Initializer
  /////////////////////////
  switchController_instance()
  {
    controllerType  = 0;
    dev_addr        = SWITCH_DEV_ADDR_UNDEFINED;
    dev_instance    = SWITCH_DEV_INSTANCE_UNDEFINED;
    for (int i = 0; i < MAC_ADDR_NUM_OF_BYTES; i++)
    {
      macAddrLE[i] = 0;
    }
    chargeLevel = 0;
    isCharging = false;

    present = false;

    initState = switch_controller_init_states();
    playerLedData = 0;
    rumbleData = NULL;
    homeLedData = NULL;
    
    reportUpdated = false;
  }
};

#endif

// function prototype

bool switchController_sendSubCmd        (switchController_instance *);
bool switchController_updateHomeLed     (switchController_instance *);
bool switchController_enableRumble      (switchController_instance *);
bool switchController_setFullReportMode (switchController_instance *);
bool switchController_setPlayerLedNum   (switchController_instance *);
bool switchController_queryDeviceInfo   (switchController_instance *);
bool switchController_sleepHci          (switchController_instance *);
bool switchController_setImu            (switchController_instance *);
void switchController_renewReport       (switchController_instance *);

void switchController_applyControllerSettings(switchController_instance *);

void switchController_sendHidCommand(uint8_t, uint8_t, uint8_t);