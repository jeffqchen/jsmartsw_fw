#include "common/tusb_types.h"
#include "mydebug.h"

#include "tusb.h"

#include "controller_sega_saturn_usb.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

controller_api_interface saturn_usb_api = { &saturn_usb_mount,
                                            &saturn_usb_umount,
                                            &saturn_usb_hid_set_report_complete,
                                            &saturn_usb_hid_get_report_complete,
                                            &saturn_usb_hid_report_received,
                                            &saturn_usb_has_activity,
                                            &saturn_usb_setupTick,
                                            &saturn_usb_visualTick,
                                            &saturn_usb_tactileTick,
                                            &saturn_usb_executeTick
                                          };

static uint8_t registeredDaddr = 0;
static uint8_t registeredInstance = 0;
static bool deviceMounted = false;
static bool deviceActivity = false;

////////////////////////////////////////////////////
bool saturn_usb_is_hardware(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0x04b4 && pid == 0x010a)     //DS3
         );
}

bool saturn_usb_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  saturn_usb_is_hardware(dev_addr)       &&
            dev_addr == registeredDaddr     &&
            instance == registeredInstance  &&
            deviceMounted                   );
}

void process_saturn_usb(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  saturn_usb_report_t saturn_usb_input_report;
  memcpy(&saturn_usb_input_report, report, sizeof(saturn_usb_input_report));

  if  ( saturn_usb_input_report.dpad_leftRight  != SATURN_USB_DPAD_NEUTRAL ||
        saturn_usb_input_report.dpad_upDown     != SATURN_USB_DPAD_NEUTRAL ||
        saturn_usb_input_report.button_a      ||
        saturn_usb_input_report.button_b      ||
        saturn_usb_input_report.button_c      ||
        saturn_usb_input_report.button_x      ||
        saturn_usb_input_report.button_y      ||
        saturn_usb_input_report.button_z      ||
        saturn_usb_input_report.button_l      ||
        saturn_usb_input_report.button_r      ||
        saturn_usb_input_report.button_start
      )
  {
    deviceActivity = true;
    immediateControllerData.hasActivity = true;
    immediateControllerData.controllerType = CONTROLLER_SATURN_USB;
  } else {
    deviceActivity = false;
  }
  /////////////////////////////
  // Button Mapping
  /////////////////////////////
  //direcitonal controls
  immediateControllerData.dpad_up     |=  (0x00 == saturn_usb_input_report.dpad_upDown);
  immediateControllerData.dpad_down   |=  (0xff == saturn_usb_input_report.dpad_upDown);
  immediateControllerData.dpad_left   |=  (0x00 == saturn_usb_input_report.dpad_leftRight);
  immediateControllerData.dpad_right  |=  (0xff == saturn_usb_input_report.dpad_leftRight);

  //face buttons
  immediateControllerData.apad_north  |= saturn_usb_input_report.button_y;
  immediateControllerData.apad_south  |= saturn_usb_input_report.button_a;
  immediateControllerData.apad_east   |= saturn_usb_input_report.button_b;
  immediateControllerData.apad_west   |= saturn_usb_input_report.button_x;

  //select & start OR share & option
  immediateControllerData.button_start  |= saturn_usb_input_report.button_start;
  //immediateControllerData.button_select |= ds4_input_report.share;
  immediateControllerData.button_home   |= saturn_usb_input_report.button_start;

  // shoulder buttons
  immediateControllerData.button_l1 |= saturn_usb_input_report.button_l;
  immediateControllerData.button_r1 |= saturn_usb_input_report.button_r;

  //immediateControllerData.button_l2 |= ;
  //immediateControllerData.button_r2 |= ;

  //analog stick buttons
  immediateControllerData.button_l3 |= saturn_usb_input_report.button_c;
  //immediateControllerData.button_r3 |= ds4_input_report.r3;

  //modern buttons on PlayStation controlelrs
  immediateControllerData.button_touchPadButton |= saturn_usb_input_report.button_z;
}

////////////////////////////////////////////////////
//API functions

void saturn_usb_mount(uint8_t dev_addr, uint8_t instance)
{
  // Saturn USB Init
  if ( saturn_usb_is_hardware(dev_addr) )
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance = instance;

      deviceMounted = true;
      deviceActivity = false;

      #ifdef SATURNUSB_DBG
        debugPort->println("[Controller] Saturn USB mounted!");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef SATURNUSB_DBG
        debugPort->println("[SaturnUSB] Couldn't receive report");
      #endif
    } else {
      #ifdef SATURNUSB_DBG
        debugPort->println("[SaturnUSB] Can receive report!");
      #endif
    }
  }
}

void saturn_usb_umount(uint8_t dev_addr, uint8_t instance)
{
  if (saturn_usb_is_registeredInstance(dev_addr, instance))
  {
    deviceMounted = false;
    deviceActivity = false;
    registeredDaddr = 0;
    registeredInstance = 0;

    #ifdef SATURNUSB_DBG
      debugPort->println("[Controller] Saturn USB removed");
    #endif
  }
}

void saturn_usb_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (saturn_usb_is_registeredInstance(dev_addr, instance))
  {
    process_saturn_usb(report, len, dev_addr, instance);
  }
}

bool saturn_usb_has_activity()
{
  return deviceActivity;
}

////////////////////////////////////////////////////
// Not implemented api functions

void saturn_usb_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void saturn_usb_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void saturn_usb_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void saturn_usb_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void saturn_usb_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void saturn_usb_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}
