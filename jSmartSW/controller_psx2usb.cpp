#include "mydebug.h"

#include "tusb.h"

#include "mypico.h"
#include "controller_psx2usb.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

controller_api_interface psx2usb_api = {  &psx2usb_mount,
                                          &psx2usb_umount,
                                          &psx2usb_hid_set_report_complete,
                                          &psx2usb_hid_get_report_complete,
                                          &psx2usb_hid_report_received,
                                          &psx2usb_has_activity,
                                          &psx2usb_setupTick,
                                          &psx2usb_visualTick,
                                          &psx2usb_tactileTick,
                                          &psx2usb_executeTick
                                        };

static uint8_t registeredDaddr = 0;
static uint8_t registeredInstance = 0;
static bool deviceMounted = false;
static bool deviceActivity = false;

////////////////////////////////////////////////////
bool psx2usb_is_hardware(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0x1b4f && pid == 0x9204)
         );
}

bool psx2usb_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  psx2usb_is_hardware(dev_addr)       &&
            dev_addr == registeredDaddr     &&
            instance == registeredInstance  &&
            deviceMounted                   );
}

void process_psx2usb(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  uint8_t const report_id = report[0];
  report++;

  if (0x03 == report_id)
  {
    psx2usb_report_03_t psx2usb_input_report;
    memcpy(&psx2usb_input_report, report, sizeof(psx2usb_input_report));

    if  ( psx2usb_input_report.dpad < 8 ||
          psx2usb_input_report.square   ||
          psx2usb_input_report.cross    ||
          psx2usb_input_report.circle   ||
          psx2usb_input_report.triangle ||
          psx2usb_input_report.l1       ||
          psx2usb_input_report.l2       ||
          psx2usb_input_report.r1       ||
          psx2usb_input_report.r2       ||
          psx2usb_input_report.start    ||
          psx2usb_input_report.select
        )
    {
      deviceActivity = true;
      immediateControllerData.hasActivity = true;
      immediateControllerData.controllerType = CONTROLLER_PSX2USB;
    } else {
      deviceActivity = false;
    }
    /////////////////////////////
    // Button Mapping
    /////////////////////////////
    //direcitonal controls
    immediateControllerData.dpad_up     |=  (0 == psx2usb_input_report.dpad);
    immediateControllerData.dpad_down   |=  (4 == psx2usb_input_report.dpad);
    immediateControllerData.dpad_left   |=  (6 == psx2usb_input_report.dpad);
    immediateControllerData.dpad_right  |=  (2 == psx2usb_input_report.dpad);

    //face buttons
    immediateControllerData.apad_north  |= psx2usb_input_report.triangle;
    immediateControllerData.apad_south  |= psx2usb_input_report.cross;
    immediateControllerData.apad_east   |= psx2usb_input_report.circle;
    immediateControllerData.apad_west   |= psx2usb_input_report.square;

    //select & start OR share & option
    immediateControllerData.button_start  |= psx2usb_input_report.start;
    immediateControllerData.button_select |= psx2usb_input_report.select;
    immediateControllerData.button_home   |= psx2usb_input_report.start;

    // shoulder buttons
    immediateControllerData.button_l1 |= psx2usb_input_report.l1;
    immediateControllerData.button_r1 |= psx2usb_input_report.r1;

    immediateControllerData.button_l2 |= psx2usb_input_report.l2;
    immediateControllerData.button_r2 |= psx2usb_input_report.r2;
  }
}

////////////////////////////////////////////////////
//API functions

void psx2usb_mount(uint8_t dev_addr, uint8_t instance)
{
  // psx2usb USB Init
  if ( psx2usb_is_hardware(dev_addr) )
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance = instance;

      deviceMounted = true;
      deviceActivity = false;

      #ifdef CONTROLLER_DBG
        debugPort->println("[Controller] psx2usb USB mounted!");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef CONTROLLER_DBG
        debugPort->println("[PSX2USB] Couldn't receive report");
      #endif
    } else {
      #ifdef CONTROLLER_DBG
        debugPort->println("[PSX2USB] Can receive report!");
      #endif
    }
  }
}

void psx2usb_umount(uint8_t dev_addr, uint8_t instance)
{
  if (psx2usb_is_registeredInstance(dev_addr, instance))
  {
    deviceMounted = false;
    deviceActivity = false;
    registeredDaddr = 0;
    registeredInstance = 0;

    #ifdef CONTROLLER_DBG
      debugPort->println("[Controller] PSX2USB removed");
    #endif
  }
}

void psx2usb_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (psx2usb_is_registeredInstance(dev_addr, instance))
  {
    process_psx2usb(report, len, dev_addr, instance);
  }
}

bool psx2usb_has_activity()
{
  return deviceActivity;
}

////////////////////////////////////////////////////
// Not implemented api functions

void psx2usb_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void psx2usb_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void psx2usb_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void psx2usb_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void psx2usb_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void psx2usb_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}
