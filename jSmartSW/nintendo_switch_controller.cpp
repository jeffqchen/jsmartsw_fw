#include <sys/_stdint.h>
#include "mydebug.h"

#include "tusb.h"
#include <Arduino.h>

#include "nintendo_switch_controller.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

const uint8_t defaultPlayerLed = 0x0f;
const uint8_t defaultRumbleData[SWITCH_RUMBLE_DATA_SIZE] = SWITCH_RUMBLE_DATA_DEFAULT;
const uint8_t defaultHomeLedData  [SWITCH_HOME_LED_DATA_SIZE] = SWITCH_HOME_LED_DATA_DEFAULT;

////////////////////
// command report related functions
int rotateReportPacketNumber()
{
  static uint8_t proPacketNum = 0;

  if (++proPacketNum > 0xF0)
  {
    proPacketNum = 0;
  }

  return proPacketNum;
}

bool switchController_sendSubCmd(switchController_instance * currentInstance, const uint8_t* subCmdbuf, int subCmdLen)
{
  bool result = false;

  if ((millis() - currentInstance->initState.lastReportSendTime > SWITCH_SEND_REPORT_INTERVAL) || 
      (tuh_hid_send_ready(currentInstance->dev_addr, currentInstance->dev_instance))      )
  {
    int subCmdSize = sizeof(nintendo_switch_subCmd_report_t) + subCmdLen - 1;
    uint8_t subCmdData[subCmdSize]= {0};
    nintendo_switch_subCmd_report_t* subCmdReport = (nintendo_switch_subCmd_report_t*)subCmdData;

    subCmdReport->packetNum = rotateReportPacketNumber();
    if (SWITCH_NO_DATA == currentInstance->rumbleData)
    {
      memcpy(subCmdReport->rumbleData, defaultRumbleData, SWITCH_RUMBLE_DATA_SIZE);
    } else {
      memcpy(subCmdReport->rumbleData, currentInstance->rumbleData, SWITCH_RUMBLE_DATA_SIZE);
    }
    memcpy(&subCmdReport->subCmdId, subCmdbuf, subCmdLen);

    result = tuh_hid_send_report(currentInstance->dev_addr, currentInstance->dev_instance, SWITCH_OUTPUT_RUMBLE_AND_SUBCMD, subCmdReport, subCmdSize);
    
    if (true == result)
    {
      currentInstance->initState.lastReportSendTime = millis();
    } else {
      #ifdef SWITCHLIB_DBG
        debugPort->println("[SwConLib] Send command error");
      #endif
    }
  }

  return result;
}

//various subcommand functions
bool switchController_updateHomeLed(switchController_instance * currentInstance)
{
  /*
  #ifdef SWITCHLIB_DBG
    debugPort->println("[SwConLib] Lighting up home button");
  #endif
  */

  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_HOME_LED] = {0};

  subCmdData[0] = SWITCH_SUBCMD_HOME_LED;

  if (SWITCH_NO_DATA == currentInstance->homeLedData)
  {
    memcpy(&subCmdData[1], defaultHomeLedData, SWITCH_SUBCMD_DATA_SIZE_HOME_LED - 1);
  } else {
    memcpy(&subCmdData[1], currentInstance->homeLedData, SWITCH_SUBCMD_DATA_SIZE_HOME_LED - 1);
  }

  return switchController_sendSubCmd(currentInstance, subCmdData, SWITCH_SUBCMD_DATA_SIZE_HOME_LED);
}

bool switchController_enableRumble(switchController_instance * currentInstance)
{
  /*
  #ifdef SWITCHLIB_DBG
    debugPort->println("[SwConLib] Enabling Rumble");
  #endif
  */

  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_ENABLE_VIBRATION]= {0};

  subCmdData[0] = SWITCH_SUBCMD_ENABLE_VIBRATION;
  subCmdData[1] = 0x01;

  return switchController_sendSubCmd(currentInstance, subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_ENABLE_VIBRATION);
}

bool switchController_setFullReportMode(switchController_instance * currentInstance)
{
  /*
  #ifdef SWITCHLIB_DBG
    debugPort->println("[SwConLib] Setting full report mode");
  #endif
  */
  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_REPORT_MODE]= {0};

  subCmdData[0] = SWITCH_SUBCMD_SET_REPORT_MODE;
  subCmdData[1] = 0x30;

  return switchController_sendSubCmd(currentInstance, subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_REPORT_MODE);
}

bool switchController_setPlayerLedNum(switchController_instance * currentInstance)
{
  /*
  #ifdef SWITCHLIB_DBG
    debugPort->println("[SwConLib] Setting Player LED");
  #endif
  */
  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_PLAYER_LEDS]= {0};

  subCmdData[0] = SWITCH_SUBCMD_SET_PLAYER_LEDS;
  subCmdData[1] = currentInstance->playerLedData;

  return switchController_sendSubCmd(currentInstance, subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_PLAYER_LEDS);
}

bool switchController_queryDeviceInfo(switchController_instance * currentInstance)
{
  /*
  #ifdef SWITCHLIB_DBG
    debugPort->println("[SwConLib] Query device info");
  #endif
  */

  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_QUERY_DEV_INFO]= {0};
  subCmdData[0] = SWITCH_SUBCMD_QUERY_DEV_INFO;

  return switchController_sendSubCmd(currentInstance, subCmdData, SWITCH_SUBCMD_DATA_SIZE_QUERY_DEV_INFO);
}

bool switchController_sleepHci(switchController_instance * currentInstance)
{
  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_HCI_STATE]= {0};

  subCmdData[0] = SWITCH_SUBCMD_SET_HCI_STATE;
  subCmdData[1] = 0x00;

  return switchController_sendSubCmd(currentInstance, subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_HCI_STATE);
}

bool switchController_setImu(switchController_instance * currentInstance)
{
  /*
  #ifdef SWITCHLIB_DBG
    debugPort->println("[SwConLib] Enabling IMU");
  #endif
  */
  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_SET_IMU]= {0};

  subCmdData[0] = SWITCH_SUBCMD_SET_IMU;
  subCmdData[1] = 0x01;

  return switchController_sendSubCmd(currentInstance, subCmdData, SWITCH_SUBCMD_DATA_SIZE_SET_IMU);
}

//The subCmd to get new report data from the Switch Pro controller
void switchController_renewReport(switchController_instance * currentInstance)
{
  /*
  #ifdef SWITCHLIB_DBG
    debugPort->println("[SwConLib] Renewing report");
  #endif
  */

  uint8_t subCmdData[SWITCH_SUBCMD_DATA_SIZE_STATE]= {0};
  subCmdData[0] = SWITCH_SUBCMD_STATE;

  while(!switchController_sendSubCmd(currentInstance, subCmdData, SWITCH_SUBCMD_DATA_SIZE_STATE)) ;
}

void switchController_applyControllerSettings(switchController_instance * currentInstance)
{
  #ifdef SWITCHLIB_DBG
    String controllerTypeDbgText;
    switch (currentInstance->controllerType)
    {
      case SWITCH_ID_JOYCON_LEFT:
        controllerTypeDbgText = "JoyCon Left ";
        break;
      case SWITCH_ID_JOYCON_RIGHT:
        controllerTypeDbgText = "JoyCon Right";
        break;
      case SWITCH_ID_PROCON:
        controllerTypeDbgText = "Pro Controller";
        break;
    }
  #endif

  if (false == currentInstance->initState.imuEnabled)
  {
    #ifdef SWITCHLIB_DBG
      debugPort->println("[SwConLib | " + controllerTypeDbgText + "] INIT - Enabling IMU");
    #endif

    currentInstance->initState.imuEnabled = switchController_setImu(currentInstance);

  } else if (false == currentInstance->initState.fullReportMode)
  {
    #ifdef SWITCHLIB_DBG
      debugPort->println("[SwConLib | " + controllerTypeDbgText + "] INIT - Setting full report mode");
    #endif

    currentInstance->initState.fullReportMode = switchController_setFullReportMode(currentInstance);

  } else if (false == currentInstance->initState.rumbleEnabled)
  {
    #ifdef SWITCHLIB_DBG
      debugPort->println("[SwConLib | " + controllerTypeDbgText + "] INIT - Enabling Rumble");
    #endif

    currentInstance->initState.rumbleEnabled = switchController_enableRumble(currentInstance);

  } else if (false == currentInstance->initState.homeLedSet)
  {
    #ifdef SWITCHLIB_DBG
      debugPort->println("[SwConLib | " + controllerTypeDbgText + "] Setting home LED effect");
    #endif

    currentInstance->initState.homeLedSet = switchController_updateHomeLed(currentInstance);

  } else if (false == currentInstance->initState.playerLedSet)
  {

    #ifdef SWITCHLIB_DBG
      debugPort->println("[SwConLib | " + controllerTypeDbgText + "] Setting Player LED");
    #endif

    currentInstance->initState.playerLedSet = switchController_setPlayerLedNum(currentInstance);

  } else if (false == currentInstance->initState.devInfoAcquired)
  {
    #ifdef SWITCHLIB_DBG
      debugPort->println("[SwConLib | " + controllerTypeDbgText + "] Querying device info");
    #endif

    currentInstance->initState.devInfoAcquired = switchController_queryDeviceInfo(currentInstance);
  } else {
    switchController_renewReport(currentInstance); // will shut down without renewing packets
  }
}

/// USB HID Command
void switchController_sendHidCommand(uint8_t hidCmd, uint8_t dev_addr, uint8_t dev_instance)
{
  nintendo_switch_output_report_80_t output_report = {0};
  output_report.cmd = hidCmd;
  while(tuh_hid_send_report( dev_addr, dev_instance, SWITCH_OUTPUT_USB_CMD, &output_report, sizeof(output_report) ) ) ;
}