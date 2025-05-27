#include <sys/_stdint.h>
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
#include "controller_nintendo_joycon.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

controller_api_interface switchJoyCon_api = { &switchJoyCon_mount,
                                              &switchJoyCon_umount,
                                              &switchJoyCon_hid_set_report_complete,
                                              &switchJoyCon_hid_get_report_complete,
                                              &switchJoyCon_hid_report_received,
                                              &switchJoyCon_has_activity,
                                              &switchJoyCon_setupTick,
                                              &switchJoyCon_visualTick,
                                              &switchJoyCon_tactileTick,
                                              &switchJoyCon_executeTick
                                            };


static uint8_t grip_devAddr = 255;
static switchController_instance joyCons[NUM_OF_CONTROLLERS] = {switchController_instance()};
static bool gripIsPresent = false;
static bool gripIsComplete = false;

static nintendo_switch_input_report_common_data mergedJoyConReport = {0};
static bool deviceActivity = false;

static unsigned long joyCon_lastResetTime = 0;

static const uint8_t defaultHomeLedData  [SWITCH_HOME_LED_DATA_SIZE] = SWITCH_HOME_LED_DATA_DEFAULT;
static const uint8_t fastBlinkHomeLedData[SWITCH_HOME_LED_DATA_SIZE] = SWITCH_HOME_LED_DATA_FAST;

static const uint8_t defaultRumbleData[SWITCH_RUMBLE_DATA_SIZE] = SWITCH_RUMBLE_DATA_DEFAULT;
static const uint8_t joyCon_rumbleArray[2][SWITCH_RUMBLE_DATA_SIZE] = {SWITCH_RUMBLE_DATA_VIBRATE_RIGHT, SWITCH_RUMBLE_DATA_VIBRATE_LEFT};

static const uint8_t defaultPlayerLed = 0x0f;
static const uint8_t playerLedInCompleteGrip = 0x55;
static const uint8_t joyCon_playerLed[2][4] = { {0x7, 0xB, 0xD, 0xE},
                                                {0xE, 0xD, 0xB, 0x7}  };

static unsigned long rumbleStartTime = 0;
static int joyCon_ledOffset = 0;
static int numOfTicks = 0;
static bool joyConTicking = false;

bool switchJoyCon_is_hardware(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return (vid == 0x057e && pid == 0x200e);  //Swith Joy-Con Controller
         
}

bool switchJoyCon_is_registeredInstance(uint8_t dev_addr)
{
  return (  switchJoyCon_is_hardware(dev_addr)  &&
            dev_addr == grip_devAddr            &&
            true == gripIsPresent               );
}

bool isGripComplete()
{

  if ((true == joyCons[JOYCON_IDX_LEFT ].present) &&
      (true == joyCons[JOYCON_IDX_RIGHT].present) )
  {
    return true;
  }

  return false;
}

static void joyCon_clearLeftData()
{
  #ifdef SWITCHJOYCON_DBG
    debugPort->println("[JoyCon] Clear Left JoyCon Data");
  #endif

  mergedJoyConReport.dpad_down  = 0;
  mergedJoyConReport.dpad_up    = 0;
  mergedJoyConReport.dpad_right = 0;
  mergedJoyConReport.dpad_left  = 0;

  mergedJoyConReport.lx = 2000;
  mergedJoyConReport.ly = 2000;
  mergedJoyConReport.button_l3 = 0;

  mergedJoyConReport.button_minus = 0;
  mergedJoyConReport.button_cap   = 0;

  mergedJoyConReport.button_l   = 0;
  mergedJoyConReport.button_zl  = 0; 
}

static void joyCon_clearRightData()
{
  #ifdef SWITCHJOYCON_DBG
    debugPort->println("[JoyCon] Clear Right JoyCon Data");
  #endif

  mergedJoyConReport.button_a = 0;
  mergedJoyConReport.button_b = 0;
  mergedJoyConReport.button_x = 0;
  mergedJoyConReport.button_y = 0;

  mergedJoyConReport.rx = 2000;
  mergedJoyConReport.ry = 2000;
  mergedJoyConReport.button_r3 = 0;

  mergedJoyConReport.button_plus = 0;
  mergedJoyConReport.button_home = 0;

  mergedJoyConReport.button_r   = 0;
  mergedJoyConReport.button_zr  = 0;
}

static void joyCon_mergeReport(int joyConIndex, nintendo_switch_input_report_common_data* newReport)
{
  if (JOYCON_IDX_LEFT == joyConIndex)
  {
    /*
    #ifdef CONTROLLER_REPORT_DBG
      debugPort->println("[JoyCon] Merging Left JoyCon Data");
    #endif
    */

    mergedJoyConReport.dpad_down    = newReport->dpad_down;
    mergedJoyConReport.dpad_up      = newReport->dpad_up;
    mergedJoyConReport.dpad_right   = newReport->dpad_right;
    mergedJoyConReport.dpad_left    = newReport->dpad_left;

    mergedJoyConReport.lx           = newReport->lx;
    mergedJoyConReport.ly           = newReport->ly;
    mergedJoyConReport.button_l3    = newReport->button_l3;

    mergedJoyConReport.button_minus = newReport->button_minus;
    mergedJoyConReport.button_cap   = newReport->button_cap;

    mergedJoyConReport.button_l     = newReport->button_l;
    mergedJoyConReport.button_zl    = newReport->button_zl;
  } else 
  if (JOYCON_IDX_RIGHT == joyConIndex)
  {
    /*
    #ifdef CONTROLLER_REPORT_DBG
      debugPort->println("[JoyCon] Merging Right JoyCon Data");
    #endif
    */

    mergedJoyConReport.button_a     = newReport->button_a;
    mergedJoyConReport.button_b     = newReport->button_b;
    mergedJoyConReport.button_x     = newReport->button_x;
    mergedJoyConReport.button_y     = newReport->button_y;

    mergedJoyConReport.rx           = newReport->rx;
    mergedJoyConReport.ry           = newReport->ry;
    mergedJoyConReport.button_r3    = newReport->button_r3;

    mergedJoyConReport.button_plus  = newReport->button_plus;
    mergedJoyConReport.button_home  = newReport->button_home;

    mergedJoyConReport.button_r     = newReport->button_r;
    mergedJoyConReport.button_zr    = newReport->button_zr;
  }

  if (false == gripIsComplete)
  {
    if (false == joyCons[JOYCON_IDX_LEFT].present)
    {
      joyCon_clearLeftData(); 
      joyCons[JOYCON_IDX_LEFT].reportUpdated = true;
    } else 
    if (false == joyCons[JOYCON_IDX_RIGHT].present)
    {
      joyCon_clearRightData();
      joyCons[JOYCON_IDX_RIGHT].reportUpdated = true;
    }
  }
}

void resetPlayerLed()
{
  if (true == gripIsComplete)
  {
    joyCons[JOYCON_IDX_LEFT ].playerLedData = 
    joyCons[JOYCON_IDX_RIGHT].playerLedData = defaultPlayerLed;
  } else {
    joyCons[JOYCON_IDX_LEFT ].playerLedData = 
    joyCons[JOYCON_IDX_RIGHT].playerLedData = playerLedInCompleteGrip;
  }

  joyCons[JOYCON_IDX_PRO].playerLedData = defaultPlayerLed;

  joyCons[JOYCON_IDX_LEFT ].initState.playerLedSet = 
  joyCons[JOYCON_IDX_RIGHT].initState.playerLedSet = 
  joyCons[JOYCON_IDX_PRO  ].initState.playerLedSet = false; //when idle, send packets with default player LED to keep alive
}

void joyCon_processMergedInputReport(uint8_t instance)
{
  if (instance == joyCons[JOYCON_IDX_RIGHT].dev_instance)
  {
    immediateControllerData.instance = joyCons[JOYCON_IDX_LEFT].dev_instance;
  }
  
  if (mergedJoyConReport.ly > JOYCON_ANALOG_UP_RIGHT_ACTIVE_VALUE ||
      mergedJoyConReport.ly < JOYCON_ANALOG_DOWN_LEFT_ACTIVE_VALUE||
      mergedJoyConReport.lx < JOYCON_ANALOG_DOWN_LEFT_ACTIVE_VALUE||
      mergedJoyConReport.lx > JOYCON_ANALOG_UP_RIGHT_ACTIVE_VALUE ||
      mergedJoyConReport.dpad_down     ||
      mergedJoyConReport.dpad_up       ||
      mergedJoyConReport.dpad_right    ||
      mergedJoyConReport.dpad_left     ||

      mergedJoyConReport.button_y      ||
      mergedJoyConReport.button_x      ||
      mergedJoyConReport.button_b      ||
      mergedJoyConReport.button_a      ||

      mergedJoyConReport.button_l      ||
      mergedJoyConReport.button_zl     ||
      mergedJoyConReport.button_r      ||
      mergedJoyConReport.button_zr     ||

      mergedJoyConReport.button_minus  ||
      mergedJoyConReport.button_plus   ||

      mergedJoyConReport.button_home   ||
      mergedJoyConReport.button_cap    ||

      mergedJoyConReport.button_l3     ||
      mergedJoyConReport.button_r3     )
  {
    deviceActivity = true;
    immediateControllerData.hasActivity = true;
    immediateControllerData.controllerType = CONTROLLER_SWITCHJOYCON;
  } else {
    deviceActivity = false;
    
    if (true == joyConTicking)
    {      
      joyCons[JOYCON_IDX_RIGHT].homeLedData = defaultHomeLedData;    
      joyCons[JOYCON_IDX_RIGHT].initState.homeLedSet = false;

      joyCons[JOYCON_IDX_LEFT].rumbleData = joyCons[JOYCON_IDX_RIGHT].rumbleData = defaultRumbleData;
      resetPlayerLed();

      joyCon_ledOffset = 0;      
      joyConTicking = false;
    }
  }

  /////////////////////////////
  // Button Mapping
  /////////////////////////////
  //directional controls
  immediateControllerData.dpad_up     |=  (mergedJoyConReport.dpad_up    || mergedJoyConReport.ly > JOYCON_ANALOG_UP_RIGHT_ACTIVE_VALUE);
  immediateControllerData.dpad_down   |=  (mergedJoyConReport.dpad_down  || mergedJoyConReport.ly < JOYCON_ANALOG_DOWN_LEFT_ACTIVE_VALUE);
  immediateControllerData.dpad_left   |=  (mergedJoyConReport.dpad_left  || mergedJoyConReport.lx < JOYCON_ANALOG_DOWN_LEFT_ACTIVE_VALUE);
  immediateControllerData.dpad_right  |=  (mergedJoyConReport.dpad_right || mergedJoyConReport.lx > JOYCON_ANALOG_UP_RIGHT_ACTIVE_VALUE);


  //face buttons
  immediateControllerData.apad_north  |= mergedJoyConReport.button_x;
  immediateControllerData.apad_south  |= mergedJoyConReport.button_a;  //A is mapped to cross
  immediateControllerData.apad_east   |= mergedJoyConReport.button_b;  //B is mapped to circle
  immediateControllerData.apad_west   |= mergedJoyConReport.button_y;

  //select & start OR share & option
  immediateControllerData.button_start  |= mergedJoyConReport.button_home;
  immediateControllerData.button_select |= mergedJoyConReport.button_cap;
  immediateControllerData.button_home   |= mergedJoyConReport.button_home;

  // shoulder buttons
  immediateControllerData.button_l1 |= mergedJoyConReport.button_minus || mergedJoyConReport.button_l;
  immediateControllerData.button_r1 |= mergedJoyConReport.button_plus  || mergedJoyConReport.button_r;

  immediateControllerData.button_l2 |= mergedJoyConReport.button_zl;
  immediateControllerData.button_r2 |= mergedJoyConReport.button_zr;

  //analog stick buttons
  immediateControllerData.button_l3 |= mergedJoyConReport.button_l3;
  immediateControllerData.button_r3 |= mergedJoyConReport.button_r3;
}

static void joyCon_reportProcessor(uint8_t dev_addr, uint8_t dev_instance, uint8_t const* report, uint16_t len)
{
  #ifdef SWITCHJOYCON_DBG
    String controllerTypeDbgText = "Unkown";
  #endif

  int joyConIndex = -1;
  if (dev_instance == joyCons[JOYCON_IDX_LEFT].dev_instance)
  {
    joyConIndex = JOYCON_IDX_LEFT;

    #ifdef SWITCHJOYCON_DBG
      controllerTypeDbgText = "JoyCon Left";
    #endif
  } else 
  if (dev_instance == joyCons[JOYCON_IDX_RIGHT].dev_instance)
  {
    joyConIndex = JOYCON_IDX_RIGHT;

    #ifdef SWITCHJOYCON_DBG
      controllerTypeDbgText = "JoyCon Right";
    #endif
  } else 
  if (dev_instance == joyCons[JOYCON_IDX_PRO].dev_instance)
  {
    joyConIndex = JOYCON_IDX_PRO;

    #ifdef SWITCHJOYCON_DBG
      controllerTypeDbgText = "Pro Controller";
    #endif
  }

  if (SWITCH_RESPONSE == report[0])
  {
    if (SWITCH_USB_CONNECTION_STATUS == report[1])
    {
      // JoyCon Added
      if (SWITCH_JOYCON_ADD == report[2])
      {
        nintendo_switch_respond_report_conn_status_t connResponse;
        memcpy(&connResponse, report, sizeof(nintendo_switch_respond_report_conn_status_t));

        #ifdef SWITCHJOYCON_DBG
          debugPort->print("[JoyCon] Add [" + String(((SWITCH_ID_JOYCON_LEFT == connResponse.identity.controllerType) ? "Left" : "Right")) + "], ");

          debugPort->print("MAC");
          for (int i = 0; i < MAC_ADDR_NUM_OF_BYTES; i++)
          {
            debugPort->print(":");
            if (connResponse.identity.macAddrLE[MAC_ADDR_NUM_OF_BYTES - i - 1] < 0x10)
            {
              debugPort->print("0");
            }
            debugPort->print(String(connResponse.identity.macAddrLE[MAC_ADDR_NUM_OF_BYTES - i - 1], HEX));
          }
          debugPort->print("\n");
        #endif
        
        if (SWITCH_ID_JOYCON_LEFT == connResponse.identity.controllerType)
        {
          joyConIndex = JOYCON_IDX_LEFT;
          joyCons[joyConIndex].controllerType = SWITCH_ID_JOYCON_LEFT;          
        } else 
        if (SWITCH_ID_JOYCON_RIGHT == connResponse.identity.controllerType){
          joyConIndex = JOYCON_IDX_RIGHT;
          joyCons[joyConIndex].controllerType = SWITCH_ID_JOYCON_RIGHT;          
        }

        joyCon_clearLeftData();
        joyCon_clearRightData();

        joyCons[joyConIndex].dev_addr      = dev_addr;
        joyCons[joyConIndex].dev_instance = dev_instance;
        joyCons[joyConIndex].present      = true;
        joyCons[joyConIndex].initState    = switch_controller_init_states();
        if (SWITCH_ID_JOYCON_LEFT == connResponse.identity.controllerType)
        {
          joyCons[joyConIndex].initState.homeLedSet = true; //left JoyCon doesn't have home LED, no need to initialize
        }
        joyCons[joyConIndex].playerLedData  = defaultPlayerLed;
        joyCons[joyConIndex].rumbleData     = defaultRumbleData;
        memcpy(joyCons[joyConIndex].macAddrLE, connResponse.identity.macAddrLE, MAC_ADDR_NUM_OF_BYTES);
        
        gripIsComplete = isGripComplete();
        resetPlayerLed();

        switchController_sendHidCommand(SWITCH_USB_CMD_HANDSHAKE, dev_addr, dev_instance);
      }

      // JoyCon Removed
      if (SWITCH_JOYCON_REMOVE == report[2]) 
      {
        if (joyConIndex != -1)
        {
          #ifdef SWITCHJOYCON_DBG
            debugPort->println("[JoyCon] " + controllerTypeDbgText + " was removed");
            debugPort->println("[JoyCon] JoyCon grip incomplete");
          #endif
          
          //reset joycon entity data structure
          joyCons[joyConIndex] = switchController_instance();
          
          gripIsComplete = false;
          resetPlayerLed();
        }
      }
    } else 
    if (SWITCH_USB_CMD_HANDSHAKE == report[1])
    {
      if (false == joyCons[joyConIndex].initState.baudRateSet) // baudrate unset
      {
        #ifdef SWITCHJOYCON_DBG
          debugPort->println("[" + controllerTypeDbgText + "] 1st handshake successful");
        #endif

        switchController_sendHidCommand(SWITCH_USB_CMD_BAUDRATE_3M, dev_addr, dev_instance);
      } else {        
        #ifdef SWITCHJOYCON_DBG
          debugPort->println("[" + controllerTypeDbgText + "] 2nd handshake successful");
        #endif
        //switchController_sendHidCommand(SWITCH_USB_CMD_NO_TIMEOUT, dev_addr, dev_instance);
        switchController_renewReport(&(joyCons[joyConIndex]));
      }
    } else 
    if (SWITCH_USB_CMD_BAUDRATE_3M == report[1])
    {
      #ifdef SWITCHJOYCON_DBG
        debugPort->println("[" + controllerTypeDbgText + "] Baudrate set successfully");
      #endif

      joyCons[joyConIndex].initState.baudRateSet = true;

      switchController_sendHidCommand(SWITCH_USB_CMD_HANDSHAKE, dev_addr, dev_instance);
    } else 
    if (SWITCH_USB_CMD_RESET == report[1])
    {
      #ifdef SWITCHJOYCON_DBG
        debugPort->println("[" + controllerTypeDbgText + "] Resetting...");
      #endif
    }
  }
  ///// End of Handshake code /////

  // Process 0x21 controller input data
  if (SWITCH_INPUT_DATA == report[0])
  {
    nintendo_switch_input_report_common_data tempReport;
    memcpy(&tempReport, &(report[1]), sizeof(nintendo_switch_input_report_common_data));

    //update power info
    if (joyCons[joyConIndex].chargeLevel != (tempReport.powerState >> 1))
    {
      joyCons[joyConIndex].chargeLevel = (tempReport.powerState >> 1);

      #ifdef SWITCHJOYCON_DBG
        String batteryLevelText = "";
        switch (joyCons[joyConIndex].chargeLevel)
        {
          case BATTERY_LEVEL_CRITICAL:
            batteryLevelText = "Critical";
            break;
          case BATTERY_LEVEL_LOW:
            batteryLevelText = "Low";
            break;
          case BATTERY_LEVEL_MEDIUM:
            batteryLevelText = "Medium";
            break;
          case BATTERY_LEVEL_FULL:
            batteryLevelText = "Full";
            break;
        }
        debugPort->println("[" + controllerTypeDbgText + "] Charging level changed to " + batteryLevelText);
      #endif     
    }
    if (joyCons[joyConIndex].isCharging != ((0x1 == (tempReport.powerState & 0x1)) ? true : false))
    {
      joyCons[joyConIndex].isCharging = ((0x1 == (tempReport.powerState & 0x1)) ? true : false);

      #ifdef SWITCHJOYCON_DBG
        debugPort->println("[" + controllerTypeDbgText + "]" + String(((0x1 == (tempReport.powerState & 0x1)) ? " Started charging" : "Stopped charging")));
      #endif
    }

    joyCon_mergeReport(joyConIndex, &tempReport);

    switchController_applyControllerSettings(&(joyCons[joyConIndex]));

    if (SWITCH_SUBCMD_QUERY_DEV_INFO_RESPONSE == tempReport.responseType)
    {
      #ifdef SWITCHJOYCON_DBG
        String controllerTypeFromResponse = "";
        switch(tempReport.devInfo.controllerType)
        {
          case SWITCH_ID_JOYCON_LEFT:
            controllerTypeFromResponse = "JoyCon  Left";
            break;
          case SWITCH_ID_JOYCON_RIGHT:
            controllerTypeFromResponse = "JoyCon Right";
            break;
          case SWITCH_ID_PROCON:
            controllerTypeFromResponse = "Pro Controller";
            break;
        }

        String batteryLevelText = "";
        switch (joyCons[joyConIndex].chargeLevel)
        {
          case BATTERY_LEVEL_CRITICAL:
            batteryLevelText = "Critical";
            break;
          case BATTERY_LEVEL_LOW:
            batteryLevelText = "Low";
            break;
          case BATTERY_LEVEL_MEDIUM:
            batteryLevelText = "Medium";
            break;
          case BATTERY_LEVEL_FULL:
            batteryLevelText = "Full";
            break;
        }

        debugPort->print("[" + controllerTypeFromResponse + "] FW Ver: " + String(tempReport.devInfo.fw_ver_major) + "." + String(tempReport.devInfo.fw_ver_minor) + ", MAC");
        for (int i = 0; i < MAC_ADDR_NUM_OF_BYTES; i++)
        {
          debugPort->print(":");
          if (tempReport.devInfo.macAddrBE[i] < 0x10)
          {
            debugPort->print("0");
          }
          debugPort->print(String(tempReport.devInfo.macAddrBE[i], HEX));
        }
        debugPort->print(", Colors in SPI: " + String((0x1 == tempReport.devInfo.colorsInSPI)?"Yes":"No") + ", " + String(((0x1 == joyCons[joyConIndex].isCharging)?"Battery charging":"Battery not Charging")) + " - Charge level: " + batteryLevelText + "\n");
      #endif
    }
  }

  // Force reset controller when state is wrong
  if (SWITCH_ERROR_RESPONSE == report[0])
  {
    if (millis() - joyCon_lastResetTime > SWITCH_USB_RESET_WAIT)
    {
      #ifdef SWITCHJOYCON_DBG
        debugPort->println("[JoyCon] Genuine controller stuck in wrong more. Resetting...");
      #endif

      nintendo_switch_output_report_80_t output_report = {0};
      output_report.cmd = SWITCH_USB_CMD_RESET;
      tuh_hid_send_report(dev_addr, dev_instance, SWITCH_OUTPUT_USB_CMD, &output_report, sizeof(output_report));

      joyCon_lastResetTime = millis();
    }
  }
}

////////////////////////////
// API functions
////////////////////////////

void switchJoyCon_mount(uint8_t dev_addr, uint8_t instance)
{
  if (switchJoyCon_is_hardware(dev_addr))
  {
    nintendo_switch_output_report_80_t output_report = {0};
    output_report.cmd = SWITCH_USB_CMD_RESET;
    tuh_hid_send_report(dev_addr, instance, SWITCH_OUTPUT_USB_CMD, &output_report, sizeof(output_report));

    gripIsPresent = true;
    grip_devAddr = dev_addr;
    deviceActivity = false;

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef SWITCHJOYCON_DBG
        debugPort->println("[JoyCon] Couldn't receive report");
      #endif
    } else {
      #ifdef SWITCHJOYCON_DBG
        debugPort->println("[JoyCon] Can receive report!");
      #endif
    }
  }
}

void switchJoyCon_umount(uint8_t dev_addr, uint8_t instance)
{
  if (switchJoyCon_is_registeredInstance(dev_addr))
  {
    #ifdef SWITCHJOYCON_DBG
      debugPort->println("[JoyCon]<<UNMOUNT>> daddr: " + String(dev_addr, HEX) + ", instance: " + String(instance, HEX));
    #endif

    gripIsPresent = false;
    gripIsComplete = false;
    grip_devAddr = 255;
    deviceActivity = false;
    joyCons[JOYCON_IDX_LEFT] = joyCons[JOYCON_IDX_RIGHT] = joyCons[JOYCON_IDX_PRO] = switchController_instance();
  }
}

void switchJoyCon_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (switchJoyCon_is_registeredInstance(dev_addr))
  {
    joyCon_reportProcessor(dev_addr, instance, report, len);

    joyCon_processMergedInputReport(instance);
  }
}

bool switchJoyCon_has_activity()
{
  return deviceActivity;
}

void switchJoyCon_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (switchJoyCon_is_hardware(dev_addr))
  {
    if (false == joyConTicking)
    {
      #ifdef SWITCHJOYCON_DBG
        debugPort->println("[Tick] Controller Home LED set to fast");
      #endif

      if (true == joyCons[JOYCON_IDX_RIGHT].present)
      {
        joyCons[JOYCON_IDX_RIGHT].homeLedData = fastBlinkHomeLedData;
        joyCons[JOYCON_IDX_RIGHT].initState.homeLedSet = false;
      }
      joyConTicking = true;
    }
  }
}

void switchJoyCon_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (switchJoyCon_is_hardware(dev_addr))
  {
    #ifdef SWITCHJOYCON_DBG
      debugPort->println("[Tick] Instance = " + String(instance) + ", Led Offset = " + String(joyCon_ledOffset) + ", numOfTicks = " + String(numOfTicks));
    #endif

    if (HOLDING_LEFT == holdState)
    {
      joyCons[JOYCON_IDX_LEFT ].playerLedData = joyCon_playerLed[holdState][joyCon_ledOffset];
      joyCons[JOYCON_IDX_RIGHT].playerLedData = defaultPlayerLed;
    } else 
    if (HOLDING_RIGHT == holdState)
    {
      if (true == joyCons[JOYCON_IDX_RIGHT].present)
      {
        joyCons[JOYCON_IDX_LEFT ].playerLedData = defaultPlayerLed;
        joyCons[JOYCON_IDX_RIGHT].playerLedData = joyCon_playerLed[holdState][joyCon_ledOffset];
      } else {
        joyCons[JOYCON_IDX_LEFT ].playerLedData = joyCon_playerLed[holdState][joyCon_ledOffset];
      }
    }

    joyCons[JOYCON_IDX_LEFT ].initState.playerLedSet = false;
    joyCons[JOYCON_IDX_RIGHT].initState.playerLedSet = false;

    #define PLAYER_LED_SLOWDOWN_FACTOR  2
    if (0 == numOfTicks % PLAYER_LED_SLOWDOWN_FACTOR)
    {
      if (++joyCon_ledOffset > 3)
      {
        joyCon_ledOffset = 0;
      }
    }

    if (++numOfTicks > 252)
    {
      numOfTicks = 0;
    }
  }
}

void switchJoyCon_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (switchJoyCon_is_hardware(dev_addr))
  {
    if (HOLDING_LEFT == holdState)
    {
      rumbleStartTime = millis();
      joyCons[JOYCON_IDX_LEFT ].rumbleData  = &(joyCon_rumbleArray[holdState][0]);
      joyCons[JOYCON_IDX_RIGHT].rumbleData  = defaultRumbleData;
    } else 
    if (HOLDING_RIGHT == holdState)
    {
      rumbleStartTime = millis();
      if (true == joyCons[JOYCON_IDX_RIGHT].present)
      {
        joyCons[JOYCON_IDX_LEFT ].rumbleData = defaultRumbleData;
        joyCons[JOYCON_IDX_RIGHT].rumbleData = &(joyCon_rumbleArray[holdState][0]);
      } else {
        joyCons[JOYCON_IDX_LEFT ].rumbleData = &(joyCon_rumbleArray[HOLDING_LEFT][0]);
      }
    }

    #ifdef SWITCHJOYCON_DBG
      debugPort->println("[Tick] Rumble " + String((holdState == 0)?"Right":"Left"));
    #endif
  }
}

void switchJoyCon_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (switchJoyCon_is_hardware(dev_addr))
  {
    //stop rumble when vibration duration is reached
    if( millis() - rumbleStartTime > 50       || 
        false == deviceActivity               )
    {
      joyCons[JOYCON_IDX_LEFT ].rumbleData = 
      joyCons[JOYCON_IDX_RIGHT].rumbleData = defaultRumbleData;
    }
  }
}

/////////////////////////////////////////////
// Unused API
/////////////////////////////////////////////

void switchJoyCon_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void switchJoyCon_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}