////////////////////////////////////////////////////////////////////
// Special Thanks
// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/
// https://gist.github.com/ToadKing/b883a8ccfa26adcc6ba9905e75aeb4f2
// https://github.com/MTCKC/ProconXInput/
//
////////////////////////////////////////////////////////////////////

#include "mydebug.h"

#include "tusb.h"
#include <Arduino.h>

#include "mypico.h"
#include "nintendo_switch_controller.h"
#include "controller_nintendo_switch_pro.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

static uint8_t registeredDaddr = 0;
static uint8_t registeredInstance = 0;
static bool deviceMounted = false;
static bool deviceActivity = false;

//static bool switchPro_baudRateSet = false;
static bool switchPro_handshakeSuccessful = false;

static switch_pro_init_states switchProStates;

static unsigned long switchProCon_lastPacketSentTime = 0;
static unsigned long switchProCon_lastResetTiming = 0;

static bool switchProConIsActually8bitdo = false;

static const uint8_t* switchProCon_homeLedData;
static const uint8_t defaultHomeLedData  [SWITCH_HOME_LED_DATA_SIZE] = SWITCH_HOME_LED_DATA_DEFAULT;
static const uint8_t fastBlinkHomeLedData[SWITCH_HOME_LED_DATA_SIZE] = SWITCH_HOME_LED_DATA_FAST;

static const uint8_t* switchProCon_rumbleData;
static const uint8_t defaultRumbleData[SWITCH_RUMBLE_DATA_SIZE] = SWITCH_RUMBLE_DATA_DEFAULT;
static const uint8_t switchProCon_rumbleArray[2][SWITCH_RUMBLE_DATA_SIZE] = {SWITCH_RUMBLE_DATA_VIBRATE_RIGHT, SWITCH_RUMBLE_DATA_VIBRATE_LEFT};

static const uint8_t defaultPlayerLed = 0x0f;
static const uint8_t switchProCon_playerLed[2][4] = { {0xE, 0xD, 0xB, 0x7},
                                                      {0x7, 0xB, 0xD, 0xE}  };
static const uint8_t* switchProCon_ledData = &defaultPlayerLed;

static unsigned long rumbleStartTime = 0;
static int switchProCon_ledOffset = 0;
static int numOfTicks = 0;
static bool switchProTicked = false;

controller_api_interface switchProCon_api =  {  &switchProCon_mount,
                                                &switchProCon_umount,
                                                &switchProCon_hid_set_report_complete,
                                                &switchProCon_hid_get_report_complete,
                                                &switchProCon_hid_report_received,
                                                &switchProCon_has_activity,
                                                &switchProCon_setupTick,
                                                &switchProCon_visualTick,
                                                &switchProCon_tactileTick,
                                                &switchProCon_executeTick
                                              };

////////////////////////////////////////////////////
//Switch Pro Check
bool switchProCon_is_hardware(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return (    (vid == 0x057e && pid == 0x2009)     //Swith Pro Controller
         );
}

bool switchProCon_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  switchProCon_is_hardware(dev_addr)       &&
            dev_addr == registeredDaddr     &&
            instance == registeredInstance  &&
            deviceMounted                   );
}

////////////////////
// command report related functions
int rotateSwitchProReportPacketNumber()
{
  static uint8_t proPacketNum = 0;

  if (++proPacketNum > 0xF)
  {
    proPacketNum = 0;
  }

  return proPacketNum;
}

bool switchProCon_sendSubCmd(const uint8_t* subCmdbuf, int subCmdLen, const uint8_t* rumbleDataBuf)
{
  bool result = false;
  if ((millis() - switchProCon_lastPacketSentTime > SWITCH_SEND_REPORT_INTERVAL) || (tuh_hid_send_ready(registeredDaddr, registeredInstance)))
  {
    int subCmdSize = sizeof(nintendo_switch_subCmd_report_t) + subCmdLen - 1;
    uint8_t subCmdData[subCmdSize]= {0};
    nintendo_switch_subCmd_report_t* subCmdReport = (nintendo_switch_subCmd_report_t*)subCmdData;

    subCmdReport->packetNum = rotateSwitchProReportPacketNumber();
    if (SWITCH_NO_DATA == rumbleDataBuf)
    {
      memcpy(subCmdReport->rumbleData, defaultRumbleData, SWITCH_RUMBLE_DATA_SIZE);
    } else {
      memcpy(subCmdReport->rumbleData, rumbleDataBuf, SWITCH_RUMBLE_DATA_SIZE);
    }
    memcpy(&subCmdReport->subCmdId, subCmdbuf, subCmdLen);

    result = tuh_hid_send_report(registeredDaddr, registeredInstance, SWITCH_OUTPUT_RUMBLE_AND_SUBCMD, subCmdReport, subCmdSize);

    switchProCon_lastPacketSentTime = millis();
  }

  return result;
}

//various subcommand functions
bool switchProCon_updateHomeLed(const uint8_t* homeLedData)
{
  /*
  #ifdef SWITCHPRO_DBG
    debugPort->println("[SwPro] Lighting up home button");
  #endif
  */

  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_HOME_LED] = {0};

  subCmdData[0] = SWITCH_SUBCMD_HOME_LED;

  if (0 == homeLedData)
  {
    memcpy(&subCmdData[1], defaultHomeLedData, SWITCH_SUBCMD_DATA_SIZE_HOME_LED - 1);
  } else {
    memcpy(&subCmdData[1], homeLedData, SWITCH_SUBCMD_DATA_SIZE_HOME_LED - 1);
  }

  return switchProCon_sendSubCmd(subCmdData, SWITCH_SUBCMD_DATA_SIZE_HOME_LED, switchProCon_rumbleData);
}

bool switchProCon_enableRumble()
{
  /*
  #ifdef SWITCHPRO_DBG
    debugPort->println("[SwPro] Enabling Rumble");
  #endif
  */

  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_ENABLE_VIBRATION]= {0};

  subCmdData[0] = SWITCH_SUBCMD_ENABLE_VIBRATION;
  subCmdData[1] = 0x01;

  return switchProCon_sendSubCmd(subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_ENABLE_VIBRATION, SWITCH_NO_DATA);
}

bool switchProCon_setFullReportMode()
{
  /*
  #ifdef SWITCHPRO_DBG
    debugPort->println("[SwPro] Setting full report mode");
  #endif
  */
  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_REPORT_MODE]= {0};

  subCmdData[0] = SWITCH_SUBCMD_SET_REPORT_MODE;
  subCmdData[1] = 0x30;

  return switchProCon_sendSubCmd(subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_REPORT_MODE, 0);
}

bool switchProCon_setPlayerLedNum(const uint8_t* ledData, const uint8_t* rumbleData)
{
  /*
  #ifdef SWITCHPRO_DBG
    debugPort->println("[SwPro] Setting Player LED");
  #endif
  */
  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_PLAYER_LEDS]= {0};

  subCmdData[0] = SWITCH_SUBCMD_SET_PLAYER_LEDS;
  if (SWITCH_NO_DATA == ledData)
  {
    subCmdData[1] = 0x0F;
  } else {
    subCmdData[1] = *ledData;
  }

  return switchProCon_sendSubCmd(subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_PLAYER_LEDS, rumbleData);
}

bool switchProCon_queryDeviceInfo()
{
  #ifdef SWITCHPRO_DBG
    debugPort->println("[SwPro] Query device info");
  #endif

  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_QUERY_DEV_INFO]= {0};
  subCmdData[0] = SWITCH_SUBCMD_QUERY_DEV_INFO;

  return switchProCon_sendSubCmd(subCmdData, SWITCH_SUBCMD_DATA_SIZE_QUERY_DEV_INFO, 0);
}

bool switchProCon_sleepHci()
{
  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_HCI_STATE]= {0};

  subCmdData[0] = SWITCH_SUBCMD_SET_HCI_STATE;
  subCmdData[1] = 0x00;

  return switchProCon_sendSubCmd(subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_HCI_STATE, 0);
}

bool switchProCon_setImu()
{
  /*
  #ifdef SWITCHPRO_DBG
    debugPort->println("[SwPro] Enabling IMU");
  #endif
  */
  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_IMU]= {0};

  subCmdData[0] = SWITCH_SUBCMD_SET_IMU;
  subCmdData[1] = 0x01;

  return switchProCon_sendSubCmd(subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_IMU, 0);
}

//The subCmd to get new report data from the Switch Pro controller
void switchProCon_renewReport()
{
  /*
  #ifdef SWITCHPRO_DBG
    debugPort->println("[SwPro] Renewing report");
  #endif
  */

  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_STATE]= {0};
  subCmdData[0] = SWITCH_SUBCMD_STATE;

  while(!switchProCon_sendSubCmd(subCmdData, SWITCH_SUBCMD_DATA_SIZE_STATE, switchProCon_rumbleData)) ;
}

/////////////////////////////

void switchProConHandshake(const uint8_t* report, uint16_t len)
{
  uint8_t report_id = report[0];

  nintendo_switch_output_report_80_t output_report = {0};

  // USB Input Response
  if (0x81 == report[0])
  {
    if (1 == report[1] &&
        3 == report[3]  ) //Baudrate set to 3Mbit, continue to handshake the 2nd time
    {
      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] === Handshaking Stage 1 Begin ===");
      #endif

      if (false == switchProStates.switchPro_baudRateSet)
      {
        #ifdef SWITCHPRO_DBG
          debugPort->println("[SwPro]<<S1>> Setting baudrate");
        #endif

        output_report.cmd = SWITCH_USB_CMD_BAUDRATE_3M;

        while(tuh_hid_send_report( registeredDaddr, registeredInstance, SWITCH_OUTPUT_USB_CMD, &output_report, sizeof(output_report) ) ) ;
        //delay(25);
        switchProStates.switchPro_baudRateSet = true;
      } else {
        switchProCon_renewReport();
      }

      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro]<<S1>> Controller is genuine Nintendo Switch Pro controller");
      #endif

      //8bitdo controiller doesn't enter this part, leaving the flag set for it
      switchProConIsActually8bitdo = false;

      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] === Handshake Stage 1 complete ===");
      #endif
    }

    if (3 == report[1])
    {
      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] Baudrate set successfully. Handshake again");
      #endif

      output_report.cmd = SWITCH_USB_CMD_HANDSHAKE;
      while(tuh_hid_send_report( registeredDaddr, registeredInstance, SWITCH_OUTPUT_USB_CMD, &output_report, sizeof(output_report) ) ) ;
    }

    if (2 == report[1])
    {
      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] === Handshaking Stage 2 ===");
      #endif

      if (false == switchProConIsActually8bitdo)
      { // Genuine Nintendo Swith Pro contoller
        #ifdef SWITCHPRO_DBG
          debugPort->println("[SwPro]<<S2>> Start polling real Nintendo controller");
        #endif

        switchProCon_renewReport(); //can't complete without sending one packet
      } else {  // deal with 8bitdo M30
        #ifdef SWITCHPRO_DBG
          debugPort->println("[SwPro]<<S2>> Start polling 8bitdo M30 controller in wired mode");
        #endif

        nintendo_switch_output_report_80_t output_report = {0};
        output_report.cmd = SWITCH_USB_CMD_NO_TIMEOUT;
        while( tuh_hid_send_report(registeredDaddr, registeredInstance, SWITCH_OUTPUT_USB_CMD, &output_report, sizeof(output_report) ) ) ;
        //delay(25);
      }

      switchPro_handshakeSuccessful = true;

      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro]<<S2>> Mounted");
        debugPort->println("[SwPro] === Handshaking Stage 2 complete ===");
      #endif
    }
  } else if (0x00 == report_id)
  {
    switchProConIsActually8bitdo = false; // only genuine Nintendo Switch Pro controller sends report 0x00 in confusion

    if (millis() - switchProCon_lastResetTiming > SWITCH_USB_RESET_WAIT)
    {
      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] Genuine controller stuck in wrong more. Resetting...");
      #endif

      nintendo_switch_output_report_80_t output_report = {0};
      output_report.cmd = SWITCH_USB_CMD_RESET;
      tuh_hid_send_report(registeredDaddr, registeredInstance, SWITCH_OUTPUT_USB_CMD, &output_report, sizeof(output_report));

      switchProCon_lastResetTiming = millis();
    }
  } else {
    #ifdef SWITCHPRO_DBG
      debugPort->println("[SwPro] Unhandled report id: 0x" + String(report_id, HEX));
    #endif
  }
}

void switchProCon_applyControllerSettings()
{
  if (false == switchProStates.imuEnabled)
  {
    #ifdef SWITCHPRO_DBG
      debugPort->println("[SwPro] INIT - Enabling IMU");
    #endif

    switchProStates.imuEnabled = switchProCon_setImu();

  } else if (false == switchProStates.fullReportMode)
  {
    #ifdef SWITCHPRO_DBG
      debugPort->println("[SwPro] INIT - Setting full report mode");
    #endif

    switchProStates.fullReportMode = switchProCon_setFullReportMode();

  } else if (false == switchProStates.rumbleEnabled)
  {
    #ifdef SWITCHPRO_DBG
      debugPort->println("[SwPro] INIT - Enabling Rumble");
    #endif

    switchProStates.rumbleEnabled = switchProCon_enableRumble();

  } else if (false == switchProStates.homeLedSet)
  {
    #ifdef SWITCHPRO_DBG
      debugPort->println("[SwPro] Setting home LED effect");
    #endif

    switchProStates.homeLedSet = switchProCon_updateHomeLed(switchProCon_homeLedData);

  } else if (false == switchProStates.playerLedSet)
  {

    #ifdef SWITCHPRO_DBG
      debugPort->println("[SwPro] Setting Player LED");
    #endif

    switchProStates.playerLedSet = switchProCon_setPlayerLedNum(switchProCon_ledData, switchProCon_rumbleData);

  } else {
    switchProCon_renewReport(); // will shut down without renewing packets
  }
}

/////////////////////////////

void process_switchProCon_input_report(nintendo_switch_input_report_common_data* commonInputData)
{
  nintendo_switch_input_report_common_data switchProCon_report;
  memcpy(&switchProCon_report, commonInputData, sizeof(switchProCon_report));

  if (switchProCon_report.ly > PRO_CON_ANALOG_UP_RIGHT_ACTIVE_VALUE ||
      switchProCon_report.ly < PRO_CON_ANALOG_DOWN_LEFT_ACTIVE_VALUE||
      switchProCon_report.lx < PRO_CON_ANALOG_DOWN_LEFT_ACTIVE_VALUE||
      switchProCon_report.lx > PRO_CON_ANALOG_UP_RIGHT_ACTIVE_VALUE ||
      switchProCon_report.dpad_down     ||
      switchProCon_report.dpad_up       ||
      switchProCon_report.dpad_right    ||
      switchProCon_report.dpad_left     ||
      switchProCon_report.button_y      ||
      switchProCon_report.button_x      ||
      switchProCon_report.button_b      ||
      switchProCon_report.button_a      ||
      switchProCon_report.button_l      ||
      switchProCon_report.button_zl     ||
      switchProCon_report.button_r      ||
      switchProCon_report.button_zr     ||
      switchProCon_report.button_minus  ||
      switchProCon_report.button_plus   ||
      switchProCon_report.button_home   ||
      switchProCon_report.button_cap    ||
      switchProCon_report.button_l3     ||
      switchProCon_report.button_r3     )
  {
    deviceActivity = true;
    immediateControllerData.hasActivity = true;
    immediateControllerData.controllerType = CONTROLLER_SWITCHPRO;
  } else {
    deviceActivity = false;
    switchProCon_homeLedData = defaultHomeLedData;
    switchProCon_ledData = &defaultPlayerLed;
    switchProCon_rumbleData = defaultRumbleData;
    switchProCon_ledOffset = 0;    

    if (true == switchProTicked)
    {
      switchProStates.homeLedSet = false;
      switchProStates.playerLedSet = false; //when idle, send packets with default player LED to keep alive

      switchProTicked = false;
    }
  }

  /////////////////////////////
  // Button Mapping
  /////////////////////////////
  //directional controls
  immediateControllerData.dpad_up     |=  (switchProCon_report.dpad_up    || switchProCon_report.ly > PRO_CON_ANALOG_UP_RIGHT_ACTIVE_VALUE);
  immediateControllerData.dpad_down   |=  (switchProCon_report.dpad_down  || switchProCon_report.ly < PRO_CON_ANALOG_DOWN_LEFT_ACTIVE_VALUE);
  immediateControllerData.dpad_left   |=  (switchProCon_report.dpad_left  || switchProCon_report.lx < PRO_CON_ANALOG_DOWN_LEFT_ACTIVE_VALUE);
  immediateControllerData.dpad_right  |=  (switchProCon_report.dpad_right || switchProCon_report.lx > PRO_CON_ANALOG_UP_RIGHT_ACTIVE_VALUE);


  //face buttons
  immediateControllerData.apad_north  |= switchProCon_report.button_x;
  immediateControllerData.apad_south  |= switchProCon_report.button_a;  //A is mapped to cross
  immediateControllerData.apad_east   |= switchProCon_report.button_b;  //B is mapped to circle
  immediateControllerData.apad_west   |= switchProCon_report.button_y;

  //select & start OR share & option
  immediateControllerData.button_start  |= switchProCon_report.button_home;
  immediateControllerData.button_select |= switchProCon_report.button_cap;
  immediateControllerData.button_home   |= switchProCon_report.button_home;

  // shoulder buttons
  immediateControllerData.button_l1 |= switchProCon_report.button_minus || switchProCon_report.button_l;
  immediateControllerData.button_r1 |= switchProCon_report.button_plus  || switchProCon_report.button_r;

  immediateControllerData.button_l2 |= switchProCon_report.button_zl;
  immediateControllerData.button_r2 |= switchProCon_report.button_zr;

  //analog stick buttons
  immediateControllerData.button_l3 |= switchProCon_report.button_l3;
  immediateControllerData.button_r3 |= switchProCon_report.button_r3;

  switchProCon_applyControllerSettings();
}

void process_8bitdo_m30_input_report(nintendo_switch_input_report_common_data* commonInputData)
{
  nintendo_switch_input_report_common_data m30_8bitdo_report;
  memcpy(&m30_8bitdo_report, commonInputData, sizeof(m30_8bitdo_report));

  if (M30_8BITDO_DIRECTION_UP     ||
      M30_8BITDO_DIRECTION_DOWN   ||
      M30_8BITDO_DIRECTION_LEFT   ||
      M30_8BITDO_DIRECTION_RIGHT  ||

      M30_8BITDO_BUTTON_A         ||
      M30_8BITDO_BUTTON_B         ||
      M30_8BITDO_BUTTON_C         ||

      M30_8BITDO_BUTTON_X         ||
      M30_8BITDO_BUTTON_Y         ||
      M30_8BITDO_BUTTON_Z         ||

      M30_8BITDO_BUTTON_L         ||
      M30_8BITDO_BUTTON_R         ||

      M30_8BITDO_BUTTON_MINUS     ||
      M30_8BITDO_BUTTON_START     ||
      M30_8BITDO_BUTTON_HEART     ||
      M30_8BITDO_BUTTON_STAR        )
  {
    deviceActivity = true;
    immediateControllerData.hasActivity = true;
    immediateControllerData.controllerType = CONTROLLER_8BITDO_M30;
  } else {
    deviceActivity = false;
  }

  /////////////////////////////
  // Button Mapping
  /////////////////////////////
  //directional controls
  //Dpad data not present on 8bitdo
  immediateControllerData.dpad_up     |=  M30_8BITDO_DIRECTION_UP;
  immediateControllerData.dpad_down   |=  M30_8BITDO_DIRECTION_DOWN;
  immediateControllerData.dpad_left   |=  M30_8BITDO_DIRECTION_LEFT;
  immediateControllerData.dpad_right  |=  M30_8BITDO_DIRECTION_RIGHT;

  //face buttons
  immediateControllerData.apad_north  |= M30_8BITDO_BUTTON_Y; //M30_8BITDO_BUTTON_X;
  immediateControllerData.apad_south  |= M30_8BITDO_BUTTON_A;  //A is mapped to cross
  immediateControllerData.apad_east   |= M30_8BITDO_BUTTON_B;  //B is mapped to circle
  immediateControllerData.apad_west   |= M30_8BITDO_BUTTON_X; //M30_8BITDO_BUTTON_Y;

  //select & start OR share & option
  immediateControllerData.button_start  |= M30_8BITDO_BUTTON_START;
  immediateControllerData.button_select |= M30_8BITDO_BUTTON_MINUS;
  immediateControllerData.button_home   |= M30_8BITDO_BUTTON_HEART;

  // shoulder buttons
  immediateControllerData.button_l1 |= M30_8BITDO_BUTTON_L;//M30_8BITDO_BUTTON_X;
  immediateControllerData.button_r1 |= M30_8BITDO_BUTTON_R;//M30_8BITDO_BUTTON_Y;

  //immediateControllerData.button_l2 |= M30_8BITDO_BUTTON_L;
  //immediateControllerData.button_r2 |= M30_8BITDO_BUTTON_R;

  //analog stick buttons
  immediateControllerData.button_l3 |= M30_8BITDO_BUTTON_C;

  immediateControllerData.button_touchPadButton |= M30_8BITDO_BUTTON_STAR;
}

void process_nintendo_switchProCon(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  uint8_t const report_id = report[0];

  if (0x30 != report_id && 0x21 != report_id)
  { // Switch Pro controller replied with handshake stage reports
    switchProConHandshake(report, len);
  } else if (0x21 == report_id) // Genuine Nintendo Switch Pro Controller in partial report mode
  {
    nintendo_switch_input_report_21_t switchProCon_report_21;
    memcpy(&switchProCon_report_21, report + 1, sizeof(switchProCon_report_21));
    nintendo_switch_input_report_common_data* commonData_21;
    commonData_21 = &switchProCon_report_21.commonData;

    //only genuine Switch Pro controller sends report 0x21
    switchProConIsActually8bitdo = false;

    process_switchProCon_input_report(commonData_21);

  } else {  // report id is 0x30
    //report++;
    nintendo_switch_input_report_30_t switchProCon_report_30;
    memcpy(&switchProCon_report_30, report + 1, sizeof(switchProCon_report_30));

    nintendo_switch_input_report_common_data* commonData_30;
    commonData_30 = &switchProCon_report_30.commonData;

    
    #ifdef SWITCHPRO_DBG
      debugPort->println("[SwPro] conn_info: " + String(commonData_30->conn_info, HEX));
    #endif
    

    if (/*switchProCon_report.imu_data[0].accel_x != 0xfd00 &&
        switchProCon_report.imu_data[0].accel_y != 0x29fd   */
        commonData_30->conn_info != 0)
    { //Genuine Nintendo Switch Pro controller
      switchProConIsActually8bitdo = false;

      process_switchProCon_input_report(commonData_30);
    } else {
      //another evidence that this is an 8bitdo M30 controller
      switchProConIsActually8bitdo = true;

      process_8bitdo_m30_input_report(commonData_30);
    }
  }
}

////////////////////////////////////////////////////
//API functions

void switchProCon_mount(uint8_t dev_addr, uint8_t instance)
{
  //Nintendo Pro Controller
  if ( switchProCon_is_hardware(dev_addr) )
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance = instance;

      deviceMounted = true;
      deviceActivity = false;
      
      switchProStates = switch_pro_init_states();

      switchProConIsActually8bitdo = true;

      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] Switch Pro OR 8bitdo M30 recognized!");
      #endif
    } else {
      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] ...Problem??");
      #endif
      deviceMounted = false;
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] Couldn't receive report");
      #endif
    } else {
      #ifdef SWITCHPRO_DBG
        debugPort->println("[SwPro] Can receive report!");
      #endif

      //probe the controller
      nintendo_switch_output_report_80_t output_report = {0};
      output_report.cmd = SWITCH_USB_CMD_HANDSHAKE;
      while( tuh_hid_send_report(registeredDaddr, registeredInstance, SWITCH_OUTPUT_USB_CMD, &output_report, sizeof(output_report) ) ) ;
    }
  }
}

void switchProCon_umount(uint8_t dev_addr, uint8_t instance)
{
  if (switchProCon_is_registeredInstance(dev_addr, instance))
  {
    deviceMounted = false;
    deviceActivity = false;
    registeredDaddr = 0;
    registeredInstance = 0;

    switchPro_handshakeSuccessful = false;
    switchProStates = switch_pro_init_states();

    #ifdef SWITCHPRO_DBG
      if (false == switchProConIsActually8bitdo)
      {
        debugPort->println("[Controller] Switch Pro controller removed");
      } else {
        debugPort->println("[Controller] 8bitdo M30 removed");
      }
    #endif
  }
}

void switchProCon_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (switchProCon_is_registeredInstance(dev_addr, instance))
  {
    process_nintendo_switchProCon(report, len, dev_addr, instance);
  }
}

bool switchProCon_has_activity()
{
  return deviceActivity;
}

void switchProCon_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if ( switchProCon_is_hardware(dev_addr) )
  {
    if (false == switchProTicked)
    {
      #ifdef SWITCHPRO_DBG
        debugPort->println("[Tick] Switch Pro home LED set to fast");
      #endif

      switchProCon_homeLedData = fastBlinkHomeLedData;
      switchProStates.homeLedSet = false;
      switchProTicked = true;
    }
  }
}

void switchProCon_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if ( switchProCon_is_hardware(dev_addr) )
  {
    if (switchProCon_is_registeredInstance(dev_addr, instance))
    {
      switchProCon_ledData = &switchProCon_playerLed[holdState][switchProCon_ledOffset];

      if (0 == numOfTicks % 3)
      {
        if (++switchProCon_ledOffset > 3)
        {
          switchProCon_ledOffset = 0;
        }
      }

      if (++numOfTicks > 255)
      {
        numOfTicks = 0;
      }
    }
  }
}

void switchProCon_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if ( switchProCon_is_hardware(dev_addr) )
  {
    rumbleStartTime = millis();
    switchProCon_rumbleData = &(switchProCon_rumbleArray[holdState][0]);

    #ifdef SWITCHPRO_DBG
      debugPort->println("[Tick] Rumble " + String((holdState == 0)?"Right":"Left"));
    #endif
  }
}

void switchProCon_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if ( switchProCon_is_hardware(dev_addr) )
  {
    //set data to update
    switchProStates.playerLedSet = false;

    //stop rumble when vibration duration is reached
    if( millis() - rumbleStartTime > 50       || 
        true == switchProConIsActually8bitdo  || 
        false == deviceActivity               )
    {
      switchProCon_rumbleData = defaultRumbleData;
    }
  }
}

////////////////////////////////////////////////////
// Not implemented api functions

void switchProCon_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void switchProCon_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}
