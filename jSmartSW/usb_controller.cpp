#include "mydebug.h"

#include <Arduino.h>
#include "tusb.h"

#include "usb_controller.h"
#include "mypico.h"

#include "supported_controllers.h"
#include "controller_mouse.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern controller_api_interface * controllerEntity[];

game_controller_data immediateControllerData, storedControllerData;
unsigned long lastReportUpdateTime = 0;

// Rumble ticking variables
unsigned int controllerHoldTick = 0 + RUMBLE_TICK_ADJUST_VALUE;

// start one sharp rumble and end it very shortly after
void controller_holdLeftRightRumbleTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  //////////////////////////////////////////////////////////////////////////
  // Setup tick action variables and states
  for (int i = 0; i < NUM_OF_API; i++)
  {
    controllerEntity[i]->setupTick(holdState, dev_addr, instance);
  }
  
  //////////////////////////////////////////////////////////////////////////
  //Visual logic, happens during the entire time during HOLD

  for (int i = 0; i < NUM_OF_API; i++)
  {
    controllerEntity[i]->visualTick(holdState, dev_addr, instance);
  }

  //////////////////////////////////////////////////////////////////////////
  // Tacktile rumble when tick is reset to 0
  if (0 == controllerHoldTick)
  {    
    #ifdef CONTROLLER_DBG
      debugPort->println("[Tick] Trigger!");
    #endif

    for (int i = 0; i < NUM_OF_API; i++)
    {
      controllerEntity[i]->tactilelTick(holdState, dev_addr, instance);
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Actually send package to HID
  for (int i = 0; i < NUM_OF_API; i++)
  {
    controllerEntity[i]->executeTick(holdState, dev_addr, instance);
  }

  //////////////////////////////////////////////////////////////////////////
  // reset tick counter every TICK_EVERY_TIMES times
  if (++controllerHoldTick > TICK_EVERY_TIMES - 1)
  {
    controllerHoldTick = 0;
  }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

/////////////////////////////////////////////////////////
// For devices that require set/get reports (DualShock 3)

// Set for applying settings
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
  for (int i = 0; i < NUM_OF_API; i++)
  {
    controllerEntity[i]->hid_set_report_complete(dev_addr, instance, report_id, report_type, len);
  }
}

// Get for receving response after settings are applied and figure out what to do next
void tuh_hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
  for (int i = 0; i < NUM_OF_API; i++)
  {
    controllerEntity[i]->hid_get_report_complete(dev_addr, instance, report_id, report_type, len);
  }
}
/////////////////////////////////////////////////////////

// Invoked when device with hid interface is mounted
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  
  #ifdef CONTROLLER_DBG
    debugPort->println("[HID] Vid: 0x" + String(vid, HEX) + ", Pid: 0x" + String(pid, HEX));
  #endif
  
  for (int i = 0; i < NUM_OF_API; i++)
  {
    controllerEntity[i]->mount(dev_addr, instance);
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  for (int i = 0; i < NUM_OF_API; i++)
  {
    controllerEntity[i]->umount(dev_addr, instance);
  }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  #ifdef CONTROLLER_REPORT_DBG
    // Display content of received HID reports
    String debugString;
    uint16_t displayBytes = len;

    #ifdef REPORT_FILTERING
      if (NO_SWITCH_INPUT_REPORT
          NO_DS3_REPORT
         )  
    #endif 
    #ifdef NO_REPORT_FILTERING
      if (true)
    #endif
    {
      for (int i = 0; i < displayBytes; i++)
      {
        if (0 == i % 16)
        {
          debugString += "[";
          if (i < 0x10 )
          {
            debugString += "0";
          }
          debugString += String(i) + "] ";
        }

        if (report[i] < 0x10)
        {
          debugString += "0";
        }
        debugString += String(report[i], HEX) + " ";

        if (0 == (i + 1) % 64)
        {
          debugString += "[" + String(i) + "]";
          if (i + 1 < displayBytes)
          {
            debugString += "\n";
          }
        }
      }
      debugPort->println(debugString);
    }    
  #endif

  immediateControllerData = game_controller_data();

  immediateControllerData.dev_addr = dev_addr;
  immediateControllerData.instance = instance;

  for (int i = 0; i < NUM_OF_API; i++)
  {
    controllerEntity[i]->hid_report_received(dev_addr, instance, report, len);
  }

  // if report from the same controller OR other controller has activity, update stored data report
  if (( storedControllerData.dev_addr == immediateControllerData.dev_addr && 
        storedControllerData.instance == immediateControllerData.instance  ) ||
        immediateControllerData.hasActivity == true                           )
  {
    storedControllerData = immediateControllerData;
    lastReportUpdateTime = millis();
  }

  // continue to request to receive report
  tuh_hid_receive_report(dev_addr, instance);
}

///////////////////////////////////////////////////
bool controller_isThereActivity()
{
  bool result = false;

  for (int i = 0; i < NUM_OF_API; i++)
  {
    result |= controllerEntity[i]->hasActivity();
  }

  return result;
}

game_controller_data controller_currentReading()
{
  mouse_timeout();
    
  return storedControllerData;
}

void controller_tickReset()
{
  controllerHoldTick = 0 + RUMBLE_TICK_ADJUST_VALUE;
}