#include "common/tusb_types.h"
#include "mydebug.h"

//#include "tusb.h"

#include "controller_8bitdo_retro_sfc.h"
#include "usb_hid.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

controller_api_interface retro_sfc_8bitdo_api = { &retro_sfc_8bitdo_mount,
                                                  &retro_sfc_8bitdo_umount,
                                                  &retro_sfc_8bitdo_hid_set_report_complete,
                                                  &retro_sfc_8bitdo_hid_get_report_complete,
                                                  &retro_sfc_8bitdo_hid_report_received,
                                                  &retro_sfc_8bitdo_has_activity,
                                                  &retro_sfc_8bitdo_setupTick,
                                                  &retro_sfc_8bitdo_visualTick,
                                                  &retro_sfc_8bitdo_tactileTick,
                                                  &retro_sfc_8bitdo_executeTick,
                                                  &retro_sfc_8bitdo_housekeepingRoutine
                                                };

static uint8_t registeredDaddr = 0;
static uint8_t registeredInstance = 0;
static bool deviceMounted = false;
static bool deviceActivity = false;

////////////////////////////////////////////////////
bool retro_sfc_8bitdo_is_hardware(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return (  (vid == 0x2dc8 && pid == 0x3101)    // 8bitdo Retro Receiver for SFC/SNES
         );
}

bool retro_sfc_8bitdo_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  retro_sfc_8bitdo_is_hardware(dev_addr)  &&
            dev_addr == registeredDaddr       &&
            instance == registeredInstance    &&
            deviceMounted                   );
}

void process_retro_sfc_8bitdo(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  retro_sfc_8bitdo_report_t retro_sfc_8bitdo_input_report;
  memcpy(&retro_sfc_8bitdo_input_report, report, sizeof(retro_sfc_8bitdo_input_report));

  if  ( retro_sfc_8bitdo_input_report.x_axis  != RETRO_SFC_8BITDO_AXIS_NEUTRAL ||
        retro_sfc_8bitdo_input_report.y_axis  != RETRO_SFC_8BITDO_AXIS_NEUTRAL ||
        retro_sfc_8bitdo_input_report.button_a      ||
        retro_sfc_8bitdo_input_report.button_b      ||
        retro_sfc_8bitdo_input_report.button_x      ||
        retro_sfc_8bitdo_input_report.button_y      ||
        retro_sfc_8bitdo_input_report.button_l      ||
        retro_sfc_8bitdo_input_report.button_r      ||
        retro_sfc_8bitdo_input_report.button_start  ||
        retro_sfc_8bitdo_input_report.button_select
      )
  {
    deviceActivity = true;
    immediateControllerData.hasActivity = true;
    immediateControllerData.controllerType = CONTROLLER_8BITDO_RETRO_SFC;
  } else {
    deviceActivity = false;
  }
  /////////////////////////////
  // Button Mapping
  /////////////////////////////
  //direcitonal controls
  immediateControllerData.dpad_up     |=  (RETRO_SFC_8BITDO_AXIS_MIN == retro_sfc_8bitdo_input_report.y_axis);
  immediateControllerData.dpad_down   |=  (RETRO_SFC_8BITDO_AXIS_MAX == retro_sfc_8bitdo_input_report.y_axis);
  immediateControllerData.dpad_left   |=  (RETRO_SFC_8BITDO_AXIS_MIN == retro_sfc_8bitdo_input_report.x_axis);
  immediateControllerData.dpad_right  |=  (RETRO_SFC_8BITDO_AXIS_MAX == retro_sfc_8bitdo_input_report.x_axis);

  //face buttons
  immediateControllerData.apad_north  |= retro_sfc_8bitdo_input_report.button_x;
  immediateControllerData.apad_south  |= retro_sfc_8bitdo_input_report.button_a;
  immediateControllerData.apad_east   |= retro_sfc_8bitdo_input_report.button_b;
  immediateControllerData.apad_west   |= retro_sfc_8bitdo_input_report.button_y;

  //select & start OR share & option
  immediateControllerData.button_start  |= retro_sfc_8bitdo_input_report.button_start;
  //immediateControllerData.button_select |= ds4_input_report.share;
  immediateControllerData.button_home   |= retro_sfc_8bitdo_input_report.button_select;

  // shoulder buttons
  immediateControllerData.button_l1 |= retro_sfc_8bitdo_input_report.button_l;
  immediateControllerData.button_r1 |= retro_sfc_8bitdo_input_report.button_r;

  immediateControllerData.button_l2 |= retro_sfc_8bitdo_input_report.button_select;
}

////////////////////////////////////////////////////
//API functions

bool retro_sfc_8bitdo_mount(uint8_t dev_addr, uint8_t instance)
{
  // 8Bitdo Retro Receiver SFC Init
  if ( retro_sfc_8bitdo_is_hardware(dev_addr) )
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance = instance;

      deviceMounted = true;
      deviceActivity = false;

      #ifdef SFC8BITDO_DBG
        debugPort->println("[Controller] 8BitDo Retro Receiver for SFC mounted!");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef SFC8BITDO_DBG
        debugPort->println("[8BitDo SFC] Couldn't receive report");
      #endif
    } else {
      #ifdef SFC8BITDO_DBG
        debugPort->println("[8BitDo SFC] Can receive report!");
      #endif
    }

    return true;
  }

  return false;
}

void retro_sfc_8bitdo_umount(uint8_t dev_addr, uint8_t instance)
{
  if (retro_sfc_8bitdo_is_registeredInstance(dev_addr, instance))
  {
    deviceMounted = false;
    deviceActivity = false;
    registeredDaddr = 0;
    registeredInstance = 0;

    #ifdef SFC8BITDO_DBG
      debugPort->println("[Controller] 8BitDo Retro Receiver for SFC removed");
    #endif
  }
}

void retro_sfc_8bitdo_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (retro_sfc_8bitdo_is_registeredInstance(dev_addr, instance))
  {
    process_retro_sfc_8bitdo(report, len, dev_addr, instance);
  }
}

bool retro_sfc_8bitdo_has_activity()
{
  return deviceActivity;
}

////////////////////////////////////////////////////
// Not implemented api functions

void retro_sfc_8bitdo_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}
void retro_sfc_8bitdo_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}
void retro_sfc_8bitdo_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance) {}
void retro_sfc_8bitdo_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance) {}
void retro_sfc_8bitdo_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance) {}
void retro_sfc_8bitdo_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance) {}
void retro_sfc_8bitdo_housekeepingRoutine () {}