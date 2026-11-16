#include <cstdlib>
#include "host/usbh.h"
#include "common/tusb_types.h"
#include <sys/_stdint.h>
#include "mydebug.h"

#include <Arduino.h>

#include "controller_nintendo_switch2.h"
#include "usb_hid.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;
static game_controller_data tempControllerData = game_controller_data();

// API
controller_api_interface sw2_api = {&sw2Con_mount,
                                    &sw2Con_umount,
                                    &sw2Con_hid_set_report_complete,
                                    &sw2Con_hid_get_report_complete,
                                    &sw2Con_hid_report_received,
                                    &sw2Con_has_activity,
                                    &sw2Con_setupTick,
                                    &sw2Con_visualTick,
                                    &sw2Con_tactileTick,
                                    &sw2Con_executeTick,
                                    &sw2Con_housekeepingRoutine
                                    };

// Device
switch2Controller_instance sw2Controllers[NUM_OF_SW2_JOYCONS] = {switch2Controller_instance()};
static bool deviceActivity = false;
static long lastCmdSendTime = 0;

// Tick
static const uint8_t sw2_playerLed[2][4] = {{0x7, 0xB, 0xD, 0xE},
                                            {0xE, 0xD, 0xB, 0x7}};

static unsigned long rumbleStartTime = 0;
static int sw2_playerLedOffset = 0;
static bool sw2ConTicking = false;
static int numOfTicks = 0;

////////////////////////////////////////////////////
// Switch2 Joycon Hardware check
int sw2Con_daddrToType(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  if (SW2_VID == vid)
  {
    if (SW2_PID_JOYCON_LEFT   == pid)
    {
      return SW2_TYPE_JOYCON_LEFT;
    }
    if (SW2_PID_JOYCON_RIGHT  == pid)
    {
      return SW2_TYPE_JOYCON_RIGHT;
    }
    if (SW2_PID_PROCON        == pid)
    {
      return SW2_TYPE_PROCON;
    }
  }

  return SW2_TYPE_NOTSW2;
}

bool sw2Con_is_joyCon(uint8_t dev_addr)
{
  int controllerType = sw2Con_daddrToType(dev_addr);

  return ((SW2_TYPE_JOYCON_LEFT   == controllerType)  ||
          (SW2_TYPE_JOYCON_RIGHT  == controllerType)  );
}

bool sw2Con_is_proCon(uint8_t dev_addr)
{
  int controllerType = sw2Con_daddrToType(dev_addr);

  return (SW2_TYPE_PROCON == controllerType);
}

bool sw2Con_is_hardware(uint8_t dev_addr)
{
  return (sw2Con_is_proCon(dev_addr)  ||
          sw2Con_is_joyCon(dev_addr)  );
}

bool sw2Con_joyConComplete()
{
  return (sw2Controllers[SW2_IDX_JOYCON_R].present && 
          sw2Controllers[SW2_IDX_JOYCON_L].present);
}

#ifdef SW2_DBG
  String sw2Con_typeToDbgName(int controllerType)
  {
    if (SW2_TYPE_NOTSW2 != controllerType)
    {
      if (SW2_TYPE_JOYCON_RIGHT == controllerType)
      {
        return "JoyCon2 R";
      } else
      if (SW2_TYPE_JOYCON_LEFT == controllerType)
      {
        return "JoyCon2 L";
      } else 
      if (SW2_TYPE_PROCON == controllerType)
      {
        return " ProCon2 ";
      } else {
        return "JoyCon2 Unknown";
      }
    }

    return "Not JoyCon2";
  }
#endif

int sw2Con_controllerTypeToIndex(int controllerType)
{
  if (SW2_TYPE_JOYCON_RIGHT == controllerType)
  {
    return SW2_IDX_JOYCON_R;
  } else
  if (SW2_TYPE_JOYCON_LEFT == controllerType)
  {
    return SW2_IDX_JOYCON_L;
  } else 
  if (SW2_TYPE_PROCON == controllerType)
  {
    return SW2_IDX_PROCON;
  }

  return SW2_IDX_UNKNOWN;
}

void sw2Con_openEndpoints(uint8_t dev_addr)
{
  tusb_desc_endpoint_t outEp;

  outEp.bLength             = 7;
  outEp.bDescriptorType     = TUSB_DESC_CS_ENDPOINT;
  outEp.bEndpointAddress    = SW2_ENDPOINT_ADDR_OUTPUT;
  outEp.bmAttributes.xfer   = TUSB_XFER_BULK;  
  outEp.bmAttributes.sync   = TUSB_ISO_EP_ATT_NO_SYNC;
  outEp.bmAttributes.usage  = TUSB_ISO_EP_ATT_DATA;
  outEp.wMaxPacketSize      = 64;
  outEp.bInterval           = 0;
  
  tuh_edpt_open(dev_addr, &outEp);

  /*
  outEp.bLength             = 7;
  outEp.bDescriptorType     = TUSB_DESC_CS_ENDPOINT;
  outEp.bEndpointAddress    = SW2_ENDPOINT_ADDR_IN;
  outEp.bmAttributes.xfer   = TUSB_XFER_BULK;  
  outEp.bmAttributes.sync   = TUSB_ISO_EP_ATT_NO_SYNC;
  outEp.bmAttributes.usage  = TUSB_ISO_EP_ATT_DATA;
  outEp.wMaxPacketSize      = 64;
  outEp.bInterval           = 0;

  tuh_edpt_open(dev_addr, &outEp);
  */
}

inline uint8_t max(uint8_t number1, uint8_t number2)
{
  return (number1 > number2 ? number1 : number2);
}

bool sw2Con_sendOutputReport(uint8_t dev_addr, uint8_t cmdId, uint8_t subCmdId, uint8_t * cmdData, uint8_t dataSize)
{
  #ifdef SW2_VERBOSE_DBG
    debugPort->println("[" + sw2Con_typeToDbgName(sw2Con_daddrToType(dev_addr)) + "] Sending report!");
  #endif

  tuh_xfer_t newXfer;
  newXfer.daddr       = dev_addr;  
  newXfer.complete_cb = NULL; //sw2Con_responseReport;
  newXfer.ep_addr     = SW2_ENDPOINT_ADDR_OUTPUT;

  uint8_t * buffer = (uint8_t *)malloc(SW2_CMD_SIZE + dataSize);

  nintendo_switch2_joycon_output_report tempReport;
  tempReport.cmdId        = cmdId;
  tempReport.direction    = SW2_CMD_DIRECTION_OUT;
  tempReport.connType     = 0x0;
  tempReport.subCmdId     = subCmdId;
  tempReport.responseType = 0x0;
  tempReport.dataSize     = 0;

  if (NULL != cmdData)
  {
    //tempReport.argumentSize = max(SW2_CMD_ARGUMENT_SIZE_MAX, dataSize);
    tempReport.argumentSize = dataSize;
    memcpy(buffer + SW2_CMD_SIZE, cmdData, dataSize);
  } else {
    tempReport.argumentSize = 0;
  }

  memcpy(buffer, &tempReport, SW2_CMD_SIZE);
  newXfer.buffer = buffer;

  newXfer.actual_len = dataSize;
  newXfer.buflen = SW2_CMD_SIZE + dataSize;
  
  while(!tuh_ready(dev_addr)) ;
  return tuh_edpt_xfer(&newXfer);
}

bool sw2Con_toggleHidReport(switch2Controller_instance sw2Con, uint8_t argument)
{
  nintendo_switch2_cmd_data_toggle_hid cmdData;
  cmdData.argument = argument;

  return sw2Con_sendOutputReport(sw2Con.dev_addr, SW2_CMD_CATEGORY_CONNECTION, SW2_SUBCMD_TOGGLE_HID, (uint8_t *)&cmdData, sizeof(nintendo_switch2_cmd_data_toggle_hid));
}

bool sw2Con_enableHidReport(switch2Controller_instance sw2Con)
{
  #ifdef SW2_DBG
    String sw2Con_dbgName = sw2Con_typeToDbgName(sw2Con_daddrToType(sw2Con.dev_addr));
    debugPort->println("[" + sw2Con_dbgName + "] Enabling HID report");
  #endif
  
  deviceActivity = true;

  return sw2Con_toggleHidReport(sw2Con, SW2_SUBCMD_TOGGLE_HID_ENABLE);
}

#ifdef SW2_DBG
  bool sw2Con_testCmd(switch2Controller_instance * sw2Con)
  {
    #define SW2_CMD_CATEGORY_TEST_CMD     0x08
    #define SW2_CMD_CATEGORY_TEST_SUBCMD  0x01
    #define TEST_CMD_BUFFER_LEN   4

    uint8_t *testBuf = NULL;

    if (TEST_CMD_BUFFER_LEN > 0)
    {
      testBuf = (uint8_t *)malloc(TEST_CMD_BUFFER_LEN * sizeof(uint8_t));
      memset(testBuf, 0, TEST_CMD_BUFFER_LEN);
      testBuf[0] = 0x20;
    }
    
    String sw2Con_dbgName = sw2Con_typeToDbgName(sw2Con_daddrToType(sw2Con->dev_addr));
    debugPort->println("[" + sw2Con_dbgName + "] Test Cmd 0x" + String(SW2_CMD_CATEGORY_TEST_CMD, HEX) + ", SubCmd 0x" + String(SW2_CMD_CATEGORY_TEST_SUBCMD, HEX));
    
    return sw2Con_sendOutputReport(sw2Con->dev_addr, SW2_CMD_CATEGORY_TEST_CMD, SW2_CMD_CATEGORY_TEST_SUBCMD, testBuf, TEST_CMD_BUFFER_LEN);
    //return true;
  }
#endif

bool sw2Con_enableJoyconGButtons(switch2Controller_instance * sw2Con)
{
  nintendo_switch2_cmd_data_enable_joycon_g_buttons toggleData;
  toggleData.toggle = SW2_JOYCON_G_BUTTONS_ENABLE;

  #ifdef SW2_DBG
    int controllerType = sw2Con_daddrToType(sw2Con->dev_addr);
    String sw2Con_dbgName = sw2Con_typeToDbgName(controllerType);    
    debugPort->println("[" + sw2Con_dbgName + "] Enable G button");
  #endif

  return sw2Con_sendOutputReport(sw2Con->dev_addr, SW2_CMD_CATEGORY_EXTRA_FEATURES, SW2_SUBCMD_ENABLE_JOYCON_G_BUTTONS, (uint8_t *)&toggleData, sizeof(nintendo_switch2_cmd_data_enable_joycon_g_buttons));
}

bool sw2Con_enableImu(switch2Controller_instance * sw2Con)
{
  nintendo_switch2_cmd_data_enable_imu tempData;

  int controllerType = sw2Con_daddrToType(sw2Con->dev_addr);

  switch(controllerType)
  {
    case SW2_TYPE_JOYCON_LEFT:
      tempData.controllerId = SW2_INPUT_REPORT_ID_JOYCON_LEFT;
      break;

    case SW2_TYPE_JOYCON_RIGHT:
      tempData.controllerId = SW2_INPUT_REPORT_ID_JOYCON_RIGHT;
      break;

    case SW2_TYPE_PROCON:
      tempData.controllerId = SW2_INPUT_REPORT_ID_PROCON;
      break;
  }

  #ifdef SW2_DBG
    String sw2Con_dbgName = sw2Con_typeToDbgName(sw2Con_daddrToType(sw2Con->dev_addr));
    debugPort->println("[" + sw2Con_dbgName + "] Enabling IMU Data with data 0x" + String(tempData.controllerId, HEX));
  #endif

  return sw2Con_sendOutputReport(sw2Con->dev_addr, SW2_CMD_CATEGORY_CONNECTION, SW2_SUBCMD_ENABLE_IMU, (uint8_t *)&tempData, sizeof(nintendo_switch2_cmd_data_enable_imu));
}

bool sw2Con_enableFullHidReport(switch2Controller_instance * sw2Con)
{
  nintendo_switch2_cmd_data_enable_full_mode featureData;

  if (sw2Con_is_joyCon(sw2Con->dev_addr))
  {
    featureData.featureMask = SW2_FEATURE_MASK_UNKNON_1     |
                              SW2_FEATURE_MASK_UNKNON_2     |
                              SW2_FEATURE_MASK_IMU          |
                              //SW2_FEATURE_MASK_UNKNON_4     |
                              SW2_FEATURE_MASK_MOUSE_INPUT  |                              
                              SW2_FEATURE_MASK_UNKNON_6     |
                              //SW2_FEATURE_MASK_UNKNON_7     |
                              //SW2_FEATURE_MASK_UNKNON_8     |
                              0x0;
  } else {
    featureData.featureMask = SW2_FEATURE_MASK_UNKNON_1     |
                              SW2_FEATURE_MASK_UNKNON_2     |
                              SW2_FEATURE_MASK_IMU          |
                              //SW2_FEATURE_MASK_UNKNON_4     |
                              //SW2_FEATURE_MASK_MOUSE_INPUT  |                              
                              SW2_FEATURE_MASK_UNKNON_6     |
                              //SW2_FEATURE_MASK_UNKNON_7     |
                              //SW2_FEATURE_MASK_UNKNON_8       |
                              0x0;
  }

  #ifdef SW2_DBG
    String sw2Con_dbgName = sw2Con_typeToDbgName(sw2Con_daddrToType(sw2Con->dev_addr));
    debugPort->println("[" + sw2Con_dbgName + "] Enabling full input mode 0x" + String(featureData.featureMask, HEX));
  #endif

  return sw2Con_sendOutputReport(sw2Con->dev_addr, SW2_CMD_CATEGORY_FEATURE, SW2_SUBCMD_ENABLE_FULL_INPUT_MODE, (uint8_t *)&featureData, sizeof(nintendo_switch2_cmd_data_enable_full_mode));
}

void sw2Con_disableHidReport(switch2Controller_instance sw2Con)
{
  sw2Con_toggleHidReport(sw2Con, SW2_SUBCMD_TOGGLE_HID_DISABLE);
}

bool sw2Con_setPlayerLed(switch2Controller_instance * sw2Con)
{
  #ifdef SW2_DBG
    int controllerType = sw2Con_daddrToType(sw2Con->dev_addr);
    String sw2Con_dbgName = sw2Con_typeToDbgName(controllerType);    
    debugPort->println("[" + sw2Con_dbgName + "] Set player LED");
  #endif

  nintendo_switch2_cmd_data_player_led playerLedData;
  playerLedData.player_led_data = sw2Con->playerLedData;

  return sw2Con_sendOutputReport(sw2Con->dev_addr, SW2_CMD_CATEGORY_LED, SW2_SUBCMD_SET_PLAYER_LED, (uint8_t *)&playerLedData, sizeof(nintendo_switch2_cmd_data_player_led));
}

void sw2Con_resetPlayerLedToDefault(switch2Controller_instance * sw2Con)
{
  #ifdef SW2_DBG    
    String sw2Con_dbgName = sw2Con_typeToDbgName(sw2Con_daddrToType(sw2Con->dev_addr));
  #endif

  if (sw2Con_is_joyCon(sw2Con->dev_addr))
  {
    if (sw2Con_joyConComplete())
    {
      #ifdef SW2_DBG
        debugPort->println("[" + sw2Con_dbgName + "] Set both JoyCons to complete LED");
      #endif

      sw2Controllers[SW2_IDX_JOYCON_L].playerLedData = 
      sw2Controllers[SW2_IDX_JOYCON_R].playerLedData = SW2_PLAYER_LED_JOYCON_COMPLETE;

      sw2Controllers[SW2_IDX_JOYCON_L].initState.playerLedSet = 
      sw2Controllers[SW2_IDX_JOYCON_R].initState.playerLedSet = false;
    } else {

      #ifdef SW2_DBG
        debugPort->println("[" + sw2Con_dbgName + "] Set JoyCons to incomplete LED");
      #endif

      sw2Con->playerLedData = SW2_PLAYER_LED_JOYCON_INCOMPLETE;
      sw2Con->initState.playerLedSet = false;
    }
  } else 
  if (sw2Con_is_proCon(sw2Con->dev_addr))
  {
    #ifdef SW2_DBG
      debugPort->println("[" + sw2Con_dbgName + "] Set Pro Controller to default LED");
    #endif

    sw2Con->playerLedData = SW2_PLAYER_LED_DEFAULT;
    sw2Con->initState.playerLedSet = false;
  }
}

bool sw2Con_simpleVibrate(switch2Controller_instance * sw2Con, uint8_t vibType)
{
  #ifdef SW2_VERBOSE_DBG
    debugPort->println("[" + sw2Con_typeToDbgName(sw2Con_daddrToType(sw2Con->dev_addr)) + "] Rumble!");
  #endif

  nintendo_switch2_cmd_data_simple_vibration vibReport;
  vibReport.vibrationId = vibType;

  return sw2Con_sendOutputReport(sw2Con->dev_addr, SW2_CMD_CATEGORY_VIBRATION, SW2_SUBCMD_SIMPLE_VIBRATION, (uint8_t *)&vibReport, sizeof(nintendo_switch2_cmd_data_simple_vibration));
}

bool sw2Con_connectJingle(switch2Controller_instance * sw2Con)
{
  int controllerType = sw2Con_daddrToType(sw2Con->dev_addr);

  uint8_t jingleType = SW2_VIBRATION_PROJOY_OFF;

  if (SW2_TYPE_JOYCON_LEFT == controllerType)
  {
    #ifdef SW2_DBG    
      debugPort->println("[JoyCon2 L] Play left connection jingle");
    #endif

    jingleType = SW2_CONNECTION_VIBRATION_LEFT;
  } else 
  if (SW2_TYPE_JOYCON_RIGHT == controllerType)
  {
    #ifdef SW2_DBG    
      debugPort->println("[JoyCon2 R] Play right connection jingle");
    #endif

    jingleType = SW2_CONNECTION_VIBRATION_RIGHT;
  } else
  if (SW2_TYPE_PROCON == controllerType)
  {
    #ifdef SW2_DBG    
      debugPort->println("[ ProCon2 ] Play pro connection jingle");
    #endif

    jingleType = SW2_CONNECTION_VIBRATION_PRO;
  }

  return sw2Con_simpleVibrate(sw2Con, jingleType);
}

bool sw2Con_vibarateJoyConComplete()
{
  #ifdef SW2_DBG    
    debugPort->println("[JoyCon2 L] Play grip complete jingle");
  #endif

  return sw2Con_simpleVibrate(&sw2Controllers[SW2_IDX_JOYCON_L], SW2_GRIP_COMPLETION_VIBRATION);
}


void processReport_joyCon(uint8_t const* report)
{
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Clear corresponding side of data when controller is not present
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if (false == sw2Controllers[SW2_IDX_JOYCON_L].present)
  {
    #ifdef SW2_VERBOSE_DBG
      debugPort->println("[JoyCon2 L] Clearing data");
    #endif

    tempControllerData.dpad_left      = false;
    tempControllerData.dpad_right     = false;
    tempControllerData.dpad_up        = false;
    tempControllerData.dpad_down      = false;
    tempControllerData.button_l1      = false;
    tempControllerData.button_l3      = false;
    tempControllerData.button_select  = false;

    sw2Controllers[SW2_IDX_JOYCON_L].reportUpdated = true;
  }

  if (false == sw2Controllers[SW2_IDX_JOYCON_R].present)
  {
    #ifdef SW2_VERBOSE_DBG
      debugPort->println("[JoyCon2 R] Clearing data");
    #endif

    tempControllerData.apad_west    = false;
    tempControllerData.apad_north   = false;
    tempControllerData.apad_east    = false;
    tempControllerData.apad_south   = false;
    tempControllerData.button_r1    = false;
    tempControllerData.button_r3    = false;
    tempControllerData.button_start = false;
    tempControllerData.button_home  = false;

    sw2Controllers[SW2_IDX_JOYCON_R].reportUpdated = true;
  }
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Actual process JoyCon reports
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  nintendo_switch2_input_report_joycon tempReportData;
  memcpy(&tempReportData, &report[1], sizeof(nintendo_switch2_input_report_joycon));

  if (tempReportData.astick.axis_x > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP  ||
      tempReportData.astick.axis_y > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP  ||
      tempReportData.astick.axis_x < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN ||
      tempReportData.astick.axis_y < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN ||
      tempReportData.buttons.stickBttn      ||      
      tempReportData.buttons.faceBttn_east  ||
      tempReportData.buttons.faceBttn_west  ||
      tempReportData.buttons.faceBttn_north ||
      tempReportData.buttons.faceBttn_south ||      
      tempReportData.buttons.signBttn       ||
      tempReportData.buttons.bumper         ||
      tempReportData.buttons.shoulder       ||
      tempReportData.home_cap               ||
      tempReportData.chatBttn               ||
      tempReportData.gBttn                  )
  {
    if (SW2_INPUT_REPORT_ID_JOYCON_LEFT == report[0])
    {
      sw2Controllers[SW2_IDX_JOYCON_L].deviceActivity = true;
    } else {
      sw2Controllers[SW2_IDX_JOYCON_R].deviceActivity = true;
    }
  }

  if (SW2_INPUT_REPORT_ID_JOYCON_LEFT == report[0])
  {
    if (false == sw2Controllers[SW2_IDX_JOYCON_L].reportUpdated)
    {
      #ifdef SW2_VERBOSE_DBG
        debugPort->println("[JoyCon2 L] Merging data");
      #endif

      #ifdef SW2_VERBOSE_DBG
        debugPort->println("[JoyCon2 L] X = " + String(tempReportData.astick.axis_x) + ", Y = " + String(tempReportData.astick.axis_y));
      #endif
      
      tempControllerData.dpad_right = tempReportData.buttons.faceBttn_east  || tempReportData.astick.axis_x > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP;
      tempControllerData.dpad_up    = tempReportData.buttons.faceBttn_north || tempReportData.astick.axis_y > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP;
      tempControllerData.dpad_left  = tempReportData.buttons.faceBttn_west  || tempReportData.astick.axis_x < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN;
      tempControllerData.dpad_down  = tempReportData.buttons.faceBttn_south || tempReportData.astick.axis_y < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN;

      tempControllerData.button_l1  = tempReportData.buttons.signBttn || tempReportData.gBttn /*|| tempReportData.buttons.shoulder ||  tempReportData.buttons.bumper*/;

      tempControllerData.button_l3  = tempReportData.buttons.stickBttn;

      tempControllerData.button_select  = tempReportData.home_cap;

      sw2Controllers[SW2_IDX_JOYCON_L].reportUpdated = true;
    }
  } else
  if (SW2_INPUT_REPORT_ID_JOYCON_RIGHT == report[0])
  {
    if (false == sw2Controllers[SW2_IDX_JOYCON_R].reportUpdated)
    {
      #ifdef SW2_VERBOSE_DBG
        debugPort->println("[JoyCon2 R] Merging data");
      #endif

      #ifdef SW2_VERBOSE_DBG
        debugPort->println("[JoyCon2 L] X = " + String(tempReportData.astick.axis_x) + ", Y = " + String(tempReportData.astick.axis_y));
      #endif

      // use the joystick on the right JoyCon when the left one is missing
      if (false == sw2Controllers[SW2_IDX_JOYCON_L].present)
      {
        tempControllerData.dpad_right = tempReportData.astick.axis_x > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP;
        tempControllerData.dpad_up    = tempReportData.astick.axis_y > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP;
        tempControllerData.dpad_left  = tempReportData.astick.axis_x < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN;
        tempControllerData.dpad_down  = tempReportData.astick.axis_y < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN;
      }

      // Routine button mapping
      tempControllerData.apad_west  = tempReportData.buttons.faceBttn_west;
      tempControllerData.apad_north = tempReportData.buttons.faceBttn_north;

      tempControllerData.apad_east  = tempReportData.buttons.faceBttn_south;
      tempControllerData.apad_south = tempReportData.buttons.faceBttn_east;

      tempControllerData.button_r1  = tempReportData.buttons.signBttn || tempReportData.gBttn /*|| tempReportData.buttons.shoulder ||  tempReportData.buttons.bumper*/;
      tempControllerData.button_r3  = tempReportData.buttons.stickBttn;
      tempControllerData.button_start = tempReportData.home_cap;
      tempControllerData.button_home = tempReportData.home_cap;
      tempControllerData.button_touchPadButton = tempReportData.chatBttn;

      sw2Controllers[SW2_IDX_JOYCON_R].reportUpdated = true;
    }
  }
}

void processReport_proCon(uint8_t const* report)
{
  nintendo_switch2_input_report_procon tempReportData;
  memcpy(&tempReportData, &report[1], sizeof(nintendo_switch2_input_report_procon));

  if (tempReportData.astick_left.axis_x > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP  ||
      tempReportData.astick_left.axis_y > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP  ||
      tempReportData.astick_left.axis_x < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN ||
      tempReportData.astick_left.axis_y < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN ||
      tempReportData.astick_right.axis_x > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP  ||
      tempReportData.astick_right.axis_y > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP  ||
      tempReportData.astick_right.axis_x < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN ||
      tempReportData.astick_right.axis_y < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN ||
      tempReportData.buttons_left.stickBttn       ||      
      tempReportData.buttons_left.faceBttn_east   ||
      tempReportData.buttons_left.faceBttn_west   ||
      tempReportData.buttons_left.faceBttn_north  ||
      tempReportData.buttons_left.faceBttn_south  ||      
      tempReportData.buttons_left.signBttn        ||
      tempReportData.buttons_left.bumper          ||
      tempReportData.buttons_left.shoulder        ||
      tempReportData.buttons_right.stickBttn      ||      
      tempReportData.buttons_right.faceBttn_east  ||
      tempReportData.buttons_right.faceBttn_west  ||
      tempReportData.buttons_right.faceBttn_north ||
      tempReportData.buttons_right.faceBttn_south ||      
      tempReportData.buttons_right.signBttn       ||
      tempReportData.buttons_right.bumper         ||
      tempReportData.buttons_right.shoulder       ||
      tempReportData.home                         ||
      tempReportData.cap                          ||
      tempReportData.chatBttn                     ||
      tempReportData.gl                           ||
      tempReportData.gr                           )
  {
    sw2Controllers[SW2_IDX_PROCON].deviceActivity = true;
  }

  tempControllerData.dpad_right |= tempReportData.buttons_left.faceBttn_east  || tempReportData.astick_left.axis_x > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP;
  tempControllerData.dpad_up    |= tempReportData.buttons_left.faceBttn_north || tempReportData.astick_left.axis_y > SW2_JOYSTICK_AXIS_ACTIVE_RIGHT_UP;
  tempControllerData.dpad_left  |= tempReportData.buttons_left.faceBttn_west  || tempReportData.astick_left.axis_x < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN;
  tempControllerData.dpad_down  |= tempReportData.buttons_left.faceBttn_south || tempReportData.astick_left.axis_y < SW2_JOYSTICK_AXIS_ACTIVE_LEFT_DOWN;

  tempControllerData.button_l1  |= tempReportData.buttons_left.signBttn || tempReportData.buttons_left.shoulder || tempReportData.gl;
  tempControllerData.button_l3  |= tempReportData.buttons_left.stickBttn;
  tempControllerData.button_select  |= tempReportData.cap;

  tempControllerData.apad_west  |= tempReportData.buttons_right.faceBttn_west;
  tempControllerData.apad_north |= tempReportData.buttons_right.faceBttn_north;
  tempControllerData.apad_east  |= tempReportData.buttons_right.faceBttn_south;
  tempControllerData.apad_south |= tempReportData.buttons_right.faceBttn_east;

  tempControllerData.button_r1  |= tempReportData.buttons_right.signBttn || tempReportData.buttons_right.shoulder || tempReportData.gr;
  tempControllerData.button_r3  |= tempReportData.buttons_right.stickBttn;
  tempControllerData.button_start |= tempReportData.home;
  tempControllerData.button_home  |= tempReportData.chatBttn;

  sw2Controllers[SW2_IDX_PROCON].reportUpdated = true;
}

void sw2Con_commitControllerData()
{
  #ifdef SW2_VERBOSE_DBG
    debugPort->println("[JoyCon2  ] Controller data updated");
  #endif

  if (tempControllerData.dpad_left      ||
      tempControllerData.dpad_right     ||
      tempControllerData.dpad_up        ||
      tempControllerData.dpad_down      ||
      tempControllerData.apad_east      ||
      tempControllerData.apad_west      ||
      tempControllerData.apad_north     ||
      tempControllerData.apad_south     ||
      tempControllerData.button_l1      ||
      tempControllerData.button_r1      ||
      tempControllerData.button_l3      ||
      tempControllerData.button_r3      ||
      tempControllerData.button_start   ||
      tempControllerData.button_select  ||
      tempControllerData.button_home    ||
      tempControllerData.button_touchPadButton  )
  {
    #ifdef SW2_VERBOSE_DBG
      debugPort->println("[JoyCon2  ] Activity detected, filling data");
    #endif

    immediateControllerData.hasActivity = true;
    immediateControllerData.controllerType = CONTROLLER_SWITCHJOYCON;

    immediateControllerData.dpad_left     |= tempControllerData.dpad_left;
    immediateControllerData.dpad_right    |= tempControllerData.dpad_right;
    immediateControllerData.dpad_up       |= tempControllerData.dpad_up;
    immediateControllerData.dpad_down     |= tempControllerData.dpad_down;

    immediateControllerData.apad_east     |= tempControllerData.apad_east;
    immediateControllerData.apad_west     |= tempControllerData.apad_west;
    immediateControllerData.apad_north    |= tempControllerData.apad_north;
    immediateControllerData.apad_south    |= tempControllerData.apad_south;

    immediateControllerData.button_l1     |= tempControllerData.button_l1;
    immediateControllerData.button_r1     |= tempControllerData.button_r1;

    immediateControllerData.button_l3     |= tempControllerData.button_l3;
    immediateControllerData.button_r3     |= tempControllerData.button_r3;

    immediateControllerData.button_start  |= tempControllerData.button_start;
    immediateControllerData.button_select |= tempControllerData.button_select;
    immediateControllerData.button_home   |= tempControllerData.button_home;

    immediateControllerData.button_touchPadButton |= tempControllerData.button_touchPadButton;
  } else {
    #ifdef SW2_VERBOSE_DBG
      debugPort->println("[JoyCon2  ] No activity detected");
    #endif

    immediateControllerData.hasActivity = false;

    if (true == sw2ConTicking)
    {
      sw2_playerLedOffset = 0;
      sw2ConTicking = false;

      sw2Con_resetPlayerLedToDefault(&sw2Controllers[SW2_IDX_JOYCON_L]);
      sw2Con_resetPlayerLedToDefault(&sw2Controllers[SW2_IDX_JOYCON_R]);
      sw2Con_resetPlayerLedToDefault(&sw2Controllers[SW2_IDX_PROCON]);
    }
  }
}

void sw2Con_applyControllerState(switch2Controller_instance * sw2Con)
{
  if (millis() - sw2Con->mountTime  > SW2_CMD_INITIAL_DELAY   &&
      millis() - lastCmdSendTime    > SW2_CMD_INTERVAL          )
  {
    #ifdef SW2_DBG
      if (false == sw2Con->initState.testCmd)
      {
        // Set full report mode
        if (sw2Con_testCmd(sw2Con))
        {
          sw2Con->initState.testCmd = true;
          lastCmdSendTime = millis();
        }
      } else
    #endif

    if (false == sw2Con->initState.gBttnsEnabled)
    {
      // Set full report mode
      if (sw2Con_enableJoyconGButtons(sw2Con))
      {
        sw2Con->initState.gBttnsEnabled = true;
        lastCmdSendTime = millis();
      }
    } else 
    if (false == sw2Con->initState.imuEnabled)
    {
      // Set full report mode
      if (sw2Con_enableImu(sw2Con))
      {
        sw2Con->initState.imuEnabled = true;
        lastCmdSendTime = millis();
      }
    } else 
    if (false == sw2Con->initState.fullInputMode)
    {
      // Set full report mode
      if (sw2Con_enableFullHidReport(sw2Con))
      {
        sw2Con->initState.fullInputMode = true;
        lastCmdSendTime = millis();
      }
    } else 
    if (false == sw2Con->initState.playerLedSet)
    {
      // Set Player LED
      if (sw2Con_setPlayerLed(sw2Con))
      {
        sw2Con->initState.playerLedSet = true;
        lastCmdSendTime = millis();
      }
    } else
    if (false == sw2Con->initState.connectJinglePlayed)
    {
      // Vibrate on connect
      if (sw2Con_connectJingle(sw2Con))
      {
        sw2Con->initState.connectJinglePlayed = true;
        lastCmdSendTime = millis();
      }
    }
  }
}

////////////////////////////////////////////////////
//API functions

bool sw2Con_mount(uint8_t dev_addr, uint8_t instance)
{
  // JoyCon 2 Init
  if (sw2Con_is_hardware(dev_addr))
  {
    int controllerIndex = sw2Con_controllerTypeToIndex(sw2Con_daddrToType(dev_addr));

    #ifdef SW2_DBG
      String sw2Con_dbgName = sw2Con_typeToDbgName(sw2Con_daddrToType(dev_addr));
      debugPort->println("[" + sw2Con_dbgName + "] Controller recognized");
    #endif

    sw2Controllers[controllerIndex].dev_addr = dev_addr;
    sw2Controllers[controllerIndex].dev_instance = instance;
    sw2Controllers[controllerIndex].present = true;
    sw2Controllers[controllerIndex].initState.connectJinglePlayed = false;
    sw2Con_resetPlayerLedToDefault(&sw2Controllers[controllerIndex]);

    while(!tuh_hid_receive_ready(dev_addr, instance)) ;

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef SW2_DBG
        debugPort->println("[" + sw2Con_dbgName + "] Can't receive report");
      #endif
    } else {
      #ifdef SW2_DBG
        debugPort->println("[" + sw2Con_dbgName + "] Start receiving HID reports");
      #endif

      sw2Con_openEndpoints(dev_addr);
      sw2Con_enableHidReport(sw2Controllers[controllerIndex]);
      sw2Controllers[controllerIndex].mountTime = millis();

      deviceActivity = true;

      if (sw2Con_is_joyCon(dev_addr) &&
          sw2Con_joyConComplete()     )
      {
        sw2Con_vibarateJoyConComplete();
      }
    }

    return true;
  }

  return false;
}

void sw2Con_umount(uint8_t dev_addr, uint8_t instance)
{
  if (sw2Con_is_hardware(dev_addr))
  {
    int controllerIndex = sw2Con_controllerTypeToIndex(sw2Con_daddrToType(dev_addr));

    #ifdef SW2_DBG
      String sw2Con_dbgName = sw2Con_typeToDbgName(sw2Con_daddrToType(dev_addr));
      debugPort->println("[" + sw2Con_dbgName + "] Removing controller");
    #endif

    sw2Controllers[controllerIndex] = switch2Controller_instance();
    deviceActivity = false;

    // Set incomplete LED state
    if (sw2Con_is_joyCon(dev_addr))
    {
      sw2Controllers[SW2_IDX_JOYCON_L].playerLedData = 
      sw2Controllers[SW2_IDX_JOYCON_R].playerLedData = SW2_PLAYER_LED_JOYCON_INCOMPLETE;
      sw2Controllers[SW2_IDX_JOYCON_L].initState.playerLedSet = 
      sw2Controllers[SW2_IDX_JOYCON_R].initState.playerLedSet = false;
    }    
  }
}

void sw2Con_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (sw2Con_is_hardware(dev_addr))
  {
    int controllerType = sw2Con_daddrToType(dev_addr);
    int controllerIndex = sw2Con_controllerTypeToIndex(controllerType);

    #ifdef SW2_DBG
      String sw2Con_dbgName = sw2Con_typeToDbgName(controllerType);
    #endif

    nintendo_switch2_output_report_header tempHeader;
    memcpy(&tempHeader, &report[1], sizeof(nintendo_switch2_output_report_header));

    #ifdef SW2_DBG
      if ((sw2Controllers[controllerIndex].isCharging != tempHeader.isCharging)  ||
          (sw2Controllers[controllerIndex].chargeLevel != tempHeader.chargeLevel)   )
      {
        debugPort->print("[" + sw2Con_dbgName + "] Battery Level: " + String(tempHeader.chargeLevel));

        if (true == tempHeader.isCharging)
        {
          debugPort->print(". Charging.\n");
        } else {
          debugPort->print(". Not charging.\n");
        }
      }
    #endif

    sw2Controllers[controllerIndex].isCharging  = tempHeader.isCharging;
    sw2Controllers[controllerIndex].chargeLevel = tempHeader.chargeLevel;
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Process input reports accordingly
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if ((SW2_INPUT_REPORT_ID_JOYCON_LEFT  == report[0])     || 
        (SW2_INPUT_REPORT_ID_JOYCON_RIGHT == report[0])     )
    {
      processReport_joyCon(report);
    } else
    if (SW2_INPUT_REPORT_ID_PROCON == report[0])
    {
      processReport_proCon(report);
    }

    if (( (true == sw2Controllers[SW2_IDX_JOYCON_L].reportUpdated) && 
          (true == sw2Controllers[SW2_IDX_JOYCON_R].reportUpdated)  ) ||
        true == sw2Controllers[SW2_IDX_PROCON].reportUpdated            )
    {
      deviceActivity =  sw2Controllers[SW2_IDX_JOYCON_L].deviceActivity ||
                        sw2Controllers[SW2_IDX_JOYCON_R].deviceActivity ||
                        sw2Controllers[SW2_IDX_PROCON  ].deviceActivity;

      sw2Con_commitControllerData();

      tempControllerData = game_controller_data();

      // Finished updating, reset values to allow further updating
      sw2Controllers[SW2_IDX_JOYCON_L].reportUpdated =
      sw2Controllers[SW2_IDX_JOYCON_R].reportUpdated =
      sw2Controllers[SW2_IDX_PROCON  ].reportUpdated = false;

      sw2Controllers[SW2_IDX_JOYCON_L].deviceActivity = 
      sw2Controllers[SW2_IDX_JOYCON_R].deviceActivity = 
      sw2Controllers[SW2_IDX_PROCON  ].deviceActivity = false;
    }

    sw2Con_applyControllerState(&sw2Controllers[controllerIndex]);
  }
}

bool sw2Con_has_activity()
{
  return deviceActivity;
}

void sw2Con_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (sw2Con_is_hardware(dev_addr))
  {
    if (false == sw2ConTicking)
    {
      sw2ConTicking = true;
    }
  }
}

void sw2Con_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (sw2Con_is_hardware(dev_addr))
  {
    #ifdef SW2_DBG
      debugPort->println("[Tick] Instance = " + String(instance) + ", Led Offset = " + String(sw2_playerLedOffset) + ", numOfTicks = " + String(numOfTicks));
    #endif

    if (sw2Con_is_joyCon(dev_addr))
    {
      if (HOLDING_LEFT == holdState)
      {
        if (true == sw2Controllers[SW2_IDX_JOYCON_L].present)
        {
          sw2Controllers[SW2_IDX_JOYCON_L].playerLedData = sw2_playerLed[holdState][sw2_playerLedOffset];
          sw2Controllers[SW2_IDX_JOYCON_R].playerLedData = SW2_PLAYER_LED_DEFAULT;
        } else {
          sw2Controllers[SW2_IDX_JOYCON_R].playerLedData = sw2_playerLed[holdState][sw2_playerLedOffset];
        }
      } else 
      if (HOLDING_RIGHT == holdState)
      {
        if (true == sw2Controllers[SW2_IDX_JOYCON_R].present)
        {
          sw2Controllers[SW2_IDX_JOYCON_L].playerLedData = SW2_PLAYER_LED_DEFAULT;
          sw2Controllers[SW2_IDX_JOYCON_R].playerLedData = sw2_playerLed[holdState][sw2_playerLedOffset];
        } else {
          sw2Controllers[SW2_IDX_JOYCON_L].playerLedData = sw2_playerLed[holdState][sw2_playerLedOffset];
        }
      }

      sw2Controllers[SW2_IDX_JOYCON_L].initState.playerLedSet = false;
      sw2Controllers[SW2_IDX_JOYCON_R].initState.playerLedSet = false;
    } else 
    if (sw2Con_is_proCon(dev_addr))
    {
      sw2Controllers[SW2_IDX_PROCON].playerLedData = sw2_playerLed[1 - holdState][sw2_playerLedOffset];
      sw2Controllers[SW2_IDX_PROCON].initState.playerLedSet = false;
    }    

    #define SW2_PLAYER_LED_SLOWDOWN_FACTOR  2
    if (0 == numOfTicks % SW2_PLAYER_LED_SLOWDOWN_FACTOR)
    {
      if (++sw2_playerLedOffset > 3)
      {
        sw2_playerLedOffset = 0;
      }
    }

    if (++numOfTicks > 252)
    {
      numOfTicks = 0;
    }
  }
}

void sw2Con_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{

  if (sw2Con_is_hardware(dev_addr))
  {
    rumbleStartTime = millis();

    uint8_t vibrationAction = SW2_VIBRATION_PROJOY_OFF;

    if (HOLDING_LEFT == holdState)
    {
      vibrationAction = SW2_TICK_VIBRATION_LEFT;
    } else if (HOLDING_RIGHT == holdState)
    {
      vibrationAction = SW2_TICK_VIBRATION_RIGHT;
    }

    if (sw2Con_is_joyCon(dev_addr))
    { 
      if (HOLDING_LEFT == holdState)
      {        
        if (true == sw2Controllers[SW2_IDX_JOYCON_L].present)
        {
          sw2Con_simpleVibrate(&sw2Controllers[SW2_IDX_JOYCON_L], vibrationAction);
        } else {
          sw2Con_simpleVibrate(&sw2Controllers[SW2_IDX_JOYCON_R], vibrationAction);
        }
      } else 
      if (HOLDING_RIGHT == holdState)
      {
        if (true == sw2Controllers[SW2_IDX_JOYCON_R].present)
        {
          sw2Con_simpleVibrate(&sw2Controllers[SW2_IDX_JOYCON_R], vibrationAction);
        } else {
          sw2Con_simpleVibrate(&sw2Controllers[SW2_IDX_JOYCON_L], vibrationAction);
        }
      }
    } else {
      sw2Con_simpleVibrate(&sw2Controllers[SW2_IDX_PROCON], vibrationAction);
    }

    #ifdef SW2_DBG 
      debugPort->println("[Tick] Rumble " + String((holdState == 0)?"Right":"Left"));
    #endif
  }
}

// Unused API func
void sw2Con_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance) {}
void sw2Con_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}
void sw2Con_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}
void sw2Con_housekeepingRoutine() {}