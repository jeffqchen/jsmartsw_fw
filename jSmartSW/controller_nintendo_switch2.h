/*
Special Thanks

- ProCon 2 Enabler Tool
https://handheldlegend.github.io/procon2tool/

- shinyquagsire23
https://gist.github.com/shinyquagsire23/66f006b46c56216acbaac6c1e2279b64

*/

#include <sys/types.h>
#include <sys/_stdint.h>

#include "mydebug.h"

#include "usb_hid.h"

#ifndef CONTROLLER_NINTENDO_SWITCH2_H
#define CONTROLLER_NINTENDO_SWITCH2_H


enum {
  SW2_IDX_JOYCON_R = 0,
  SW2_IDX_JOYCON_L,
  SW2_IDX_PROCON,
  //-----------------
  SW2_IDX_UNKNOWN,
  //-----------------
  SW2_IDX_END
};  //array index

enum {
  SW2_TYPE_NOTSW2 = 0,
  SW2_TYPE_JOYCON_LEFT,
  SW2_TYPE_JOYCON_RIGHT,
  SW2_TYPE_PROCON,
  SW2_TYPE_UNKNOWN
};

#define NUM_OF_SW2_JOYCONS  SW2_IDX_END

#define SW2_VID                 0x057e
#define SW2_PID_JOYCON_LEFT     0x2067
#define SW2_PID_JOYCON_RIGHT    0x2066
#define SW2_PID_PROCON          0x2069

#define MAC_ADDR_NUM_OF_BYTES           6
#define SW2_DEV_ADDR_UNDEFINED      255
#define SW2_DEV_INSTANCE_UNDEFINED  255

// Endpoint
#define SW2_ENDPOINT_ADDR_IN        0x82
#define SW2_ENDPOINT_ADDR_OUTPUT    0x2
#define SW2_CMD_INITIAL_DELAY       1
#define SW2_CMD_INTERVAL            23

// Input Report Values
#define SW2_INPUT_REPORT_ID_JOYCON_LEFT       0x7
#define SW2_INPUT_REPORT_ID_JOYCON_RIGHT      0x8
#define SW2_INPUT_REPORT_ID_PROCON            0x9

//// Charing status
#define SW2_INPUT_REPORT_NOTCHARGING      0x0
#define SW2_INPUT_REPORT_CHARGER_PRESENT  0x1
#define SW2_INPUT_REPORT_CHARGING         0x2

#define SW2_JOYSTICK_AXIS_NEUTURAL           0x07FA
#define SW2_JOYSTICK_AXIS_DEADZONE           0x02FA
#define SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP    (SW2_JOYSTICK_AXIS_NEUTURAL + SW2_JOYSTICK_AXIS_DEADZONE)
#define SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN   (SW2_JOYSTICK_AXIS_NEUTURAL - SW2_JOYSTICK_AXIS_DEADZONE)

////////////////////////////////////////////////////////////
// Commands
#define SW2_CMD_DIRECTION_OUT       0x91
#define SW2_CMD_SIZE                0x0008
#define SW2_CMD_ARGUMENT_SIZE_MAX   0x0008

#define SW2_CMD_CATEGORY_CONNECTION       0x03

  #define SW2_SUBCMD_ENABLE_IMU           0x0a

  #define SW2_SUBCMD_TOGGLE_HID           0x0d
    #define SW2_SUBCMD_TOGGLE_HID_ENABLE    0x0001
    #define SW2_SUBCMD_TOGGLE_HID_DISABLE   0x0002

#define SW2_CMD_CATEGORY_EXTRA_FEATURES   0x08
  #define SW2_SUBCMD_ENABLE_JOYCON_G_BUTTONS     0x02
    #define SW2_JOYCON_G_BUTTONS_ENABLE   0x01

#define SW2_CMD_CATEGORY_LED        0x09

  #define SW2_SUBCMD_SET_PLAYER_LED       0x07
    // LED data
    #define SW2_PLAYER_LED_1P 0x11
    #define SW2_PLAYER_LED_2P 0x22
    #define SW2_PLAYER_LED_3P 0x44
    #define SW2_PLAYER_LED_4P 0x88
    #define SW2_PLAYER_LED_DEFAULT            (SW2_PLAYER_LED_1P | SW2_PLAYER_LED_2P | SW2_PLAYER_LED_3P | SW2_PLAYER_LED_4P)
    #define SW2_PLAYER_LED_JOYCON_COMPLETE     SW2_PLAYER_LED_DEFAULT
    #define SW2_PLAYER_LED_JOYCON_INCOMPLETE  (SW2_PLAYER_LED_1P | SW2_PLAYER_LED_3P)

#define SW2_CMD_CATEGORY_VIBRATION  0x0a
 
  #define SW2_SUBCMD_SIMPLE_VIBRATION       0x02
  enum {
    SW2_VIBRATION_PROJOY_OFF = 0x0,
    SW2_VIBRATION_PROJOY_1S,            //1 second medium
    SW2_VIBRATION_PROJOY_FIND,          //1 second + 2 beep sounds
    SW2_VIBRATION_PROJOY_LIGHT_TICK,    //tiny click vibration
    SW2_VIBRATION_PROJOY_PAIRING,       //pairing ka-ching
    SW2_VIBRATION_PROJOY_HEAVY_TICK,    //twerk with no sound
    SW2_VIBRATION_PROJOY_DINK1,
    SW2_VIBRATION_PROJOY_DINK2,
    SW2_VIBRATION_PROJOY_END
  };

#define SW2_CMD_CATEGORY_FEATURE  0x0c

  #define SW2_SUBCMD_ENABLE_FULL_INPUT_MODE       0x04
    #define SW2_FEATURE_MASK_UNKNON_1     0x01
    #define SW2_FEATURE_MASK_UNKNON_2     0x02
    #define SW2_FEATURE_MASK_IMU          0x04
    #define SW2_FEATURE_MASK_UNKNON_4     0x08

    #define SW2_FEATURE_MASK_MOUSE_INPUT  0x10
    #define SW2_FEATURE_MASK_UNKNON_6     0x20
    #define SW2_FEATURE_MASK_UNKNON_7     0x40
    #define SW2_FEATURE_MASK_UNKNON_8     0x80

////////////////////////////////////////////////////////////
// Vibration name assignment
#define SW2_CONNECTION_VIBRATION_LEFT     SW2_VIBRATION_PROJOY_DINK1
#define SW2_CONNECTION_VIBRATION_RIGHT    SW2_VIBRATION_PROJOY_DINK2
#define SW2_CONNECTION_VIBRATION_PRO      SW2_VIBRATION_PROJOY_PAIRING
#define SW2_GRIP_COMPLETION_VIBRATION     SW2_VIBRATION_PROJOY_LIGHT_TICK

#define SW2_TICK_VIBRATION_LEFT     SW2_VIBRATION_PROJOY_LIGHT_TICK
#define SW2_TICK_VIBRATION_RIGHT    SW2_VIBRATION_PROJOY_HEAVY_TICK

TU_ATTR_PACKED_BEGIN
TU_ATTR_BIT_FIELD_ORDER_BEGIN

typedef struct TU_ATTR_PACKED
{
  //byte 0  - report id, already removed

  //byte 1
  uint8_t packet_id;

  //byte 2
  //uint8_t connection_status;  // 0x23 charging, 0x20 charged
  struct {
    uint8_t isPowered     :1;
    uint8_t isCharging    :1;
    uint8_t chargeLevel   :4;
    uint8_t               :2;
  };
} nintendo_switch2_output_report_header;

typedef struct TU_ATTR_PACKED
{
  uint8_t faceBttn_south  : 1;
  uint8_t faceBttn_east   : 1;
  uint8_t faceBttn_west   : 1;
  uint8_t faceBttn_north  : 1;

  uint8_t shoulder        : 1;
  uint8_t bumper          : 1;
  uint8_t signBttn        : 1;
  uint8_t stickBttn       : 1;
} nintendo_switch2_output_report_common_buttons;

typedef struct TU_ATTR_PACKED
{
  uint8_t home_cap        : 1;
  uint8_t                 : 1;
  uint8_t gBttn           : 1;
  uint8_t                 : 1;

  uint8_t chatBttn        : 1;
  uint8_t                 : 1;
  uint8_t sr              : 1;
  uint8_t sl              : 1;
} nintendo_switch2_output_report_common_misc_buttons;

typedef struct TU_ATTR_PACKED
{
  #pragma pack(push,1)
  uint16_t axis_x         :12;
  uint16_t axis_y         :12;
  #pragma pack(pop)
} nintendo_switch2_output_report_common_analog_stick;

typedef struct TU_ATTR_PACKED
{
  nintendo_switch2_output_report_header header;
  nintendo_switch2_output_report_common_buttons buttons;

  struct {
    uint8_t home_cap  : 1;
    uint8_t           : 1;
    uint8_t gBttn     : 1;
    uint8_t           : 1;

    uint8_t chatBttn  : 1;
    uint8_t           : 1;
    uint8_t sr        : 1;
    uint8_t sl        : 1;
  };

  //byte 5
  uint8_t             : 8;

  nintendo_switch2_output_report_common_analog_stick astick;

  //byte 9
  uint8_t dataValidity;

  //byte 10 - 13
  struct {
    uint16_t mouseAxis_x  :16;
    uint16_t mouseAxis_y  :16;
  };
} nintendo_switch2_input_report_joycon;

typedef struct TU_ATTR_PACKED
{
  nintendo_switch2_output_report_header header;
  nintendo_switch2_output_report_common_buttons buttons_right;
  nintendo_switch2_output_report_common_buttons buttons_left;  
  
  struct {
    uint8_t home      : 1;
    uint8_t cap       : 1;
    uint8_t gr        : 1;
    uint8_t gl        : 1;

    uint8_t chatBttn  : 1;
    uint8_t           : 1;
    uint8_t           : 1;
    uint8_t           : 1;
  };

  nintendo_switch2_output_report_common_analog_stick astick_left;
  nintendo_switch2_output_report_common_analog_stick astick_right;

  uint8_t dataValidity;

  uint8_t analog_zl;
  uint8_t analog_zr;
} nintendo_switch2_input_report_procon;

typedef struct TU_ATTR_PACKED
{
  uint8_t   cmdId;
  uint8_t   direction;      // 0x91 host to device, 0x01 device to host
  uint8_t   connType;       // (???) 00 USB, 0x01 BTLE
  uint8_t   subCmdId;
  uint8_t   responseType;   // (???) 00 USB, 0x10 BTLE
  uint16_t  argumentSize;
  uint8_t   dataSize;

  uint8_t   argumentData[];
} nintendo_switch2_joycon_output_report;

#define MAC_ADDR_NUM_OF_BYTES   6

struct TU_ATTR_PACKED nintendo_switch2_cmd_data_toggle_hid
{
  uint16_t argument;
  uint8_t macAddr[MAC_ADDR_NUM_OF_BYTES];

  nintendo_switch2_cmd_data_toggle_hid()
  {
    argument = 0;
    
    memset(macAddr, 0xff, MAC_ADDR_NUM_OF_BYTES);
  }
};

struct TU_ATTR_PACKED nintendo_switch2_cmd_data_player_led
{  
  uint8_t player_led_data;

  #define PLAYERLED_PADDING   3
  uint8_t unknown[PLAYERLED_PADDING];

  nintendo_switch2_cmd_data_player_led()
  {
    player_led_data = 0;

    memset(unknown, 0, PLAYERLED_PADDING);
  }
};

struct TU_ATTR_PACKED nintendo_switch2_cmd_data_simple_vibration
{
  uint8_t vibrationId;

  #define VIBRATION_PADDING   3
  uint8_t unknown[VIBRATION_PADDING];

  
  nintendo_switch2_cmd_data_simple_vibration()
  {
    vibrationId = SW2_VIBRATION_PROJOY_OFF;

    memset(unknown, 0, VIBRATION_PADDING);
  }
  
};

struct TU_ATTR_PACKED nintendo_switch2_cmd_data_enable_full_mode
{
  uint8_t featureMask;

  #define FULLMODE_PADDING    3
  uint8_t unknown[FULLMODE_PADDING];

  nintendo_switch2_cmd_data_enable_full_mode()
  {
    featureMask = 0;

    memset(unknown, 0, FULLMODE_PADDING);
  }
};

struct TU_ATTR_PACKED nintendo_switch2_cmd_data_enable_imu
{
  uint8_t controllerId;

  #define ENABLEIMU_PADDING   3
  uint8_t unknown[ENABLEIMU_PADDING];

  nintendo_switch2_cmd_data_enable_imu()
  {
    controllerId = 0;

    memset(unknown, 0, ENABLEIMU_PADDING);
  }
};

struct TU_ATTR_PACKED nintendo_switch2_cmd_data_enable_joycon_g_buttons
{
  uint8_t toggle;

  #define ENABLE_JOYCON_G_PADDING   3
  uint8_t unknown[ENABLE_JOYCON_G_PADDING];

  nintendo_switch2_cmd_data_enable_joycon_g_buttons()
  {
    toggle = 0;

    memset(unknown, 0, ENABLE_JOYCON_G_PADDING);
  }
};

TU_ATTR_BIT_FIELD_ORDER_END
TU_ATTR_PACKED_END


////////////////////////////////////////////////  
struct switch2Controller_init_state
{
  bool fullInputMode;
  bool playerLedSet;
  bool connectJinglePlayed;
  bool imuEnabled;
  bool gBttnsEnabled;

  bool testCmd;

  switch2Controller_init_state () {
    fullInputMode = playerLedSet = connectJinglePlayed = imuEnabled = gBttnsEnabled = testCmd = false;
  }
};

struct switch2Controller_instance
{
  uint8_t dev_addr;
  uint8_t dev_instance;
  bool present;
  long mountTime;

  bool reportUpdated;
  bool deviceActivity;

  uint8_t playerLedData;
  
  uint8_t macAddrLE[MAC_ADDR_NUM_OF_BYTES];
  uint8_t chargeLevel;
  bool isCharging;

  switch2Controller_init_state initState;

  /////////////////////////
  // Initializer
  /////////////////////////
  switch2Controller_instance()
  {
    dev_addr      = SW2_DEV_ADDR_UNDEFINED;
    dev_instance  = SW2_DEV_INSTANCE_UNDEFINED;
    present       = false;
    reportUpdated = false;
    chargeLevel = 0;
    isCharging = false;

    memset(macAddrLE, 0, MAC_ADDR_NUM_OF_BYTES);
    mountTime = millis();
    initState = switch2Controller_init_state();
    
    playerLedData = 0;
  }
};

#endif

//function prototype
//bool is_sony_ds3(uint8_t);

bool sw2Con_mount(uint8_t, uint8_t);
void sw2Con_umount(uint8_t, uint8_t);

void sw2Con_hid_set_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);
void sw2Con_hid_get_report_complete(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t);

void sw2Con_hid_report_received(uint8_t, uint8_t, uint8_t const*, uint16_t);

bool sw2Con_has_activity();

void sw2Con_setupTick    (holding_left_right, uint8_t, uint8_t);
void sw2Con_visualTick   (holding_left_right, uint8_t, uint8_t);
void sw2Con_tactileTick  (holding_left_right, uint8_t, uint8_t);
void sw2Con_executeTick  (holding_left_right, uint8_t, uint8_t);

void sw2Con_housekeepingRoutine ();
