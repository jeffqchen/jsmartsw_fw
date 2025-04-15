#include "mydebug.h"

#include "tusb.h"

#include "mypico.h"
#include "controller_md2x.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

controller_api_interface md2x_api = { &md2x_mount,
                                      &md2x_umount,
                                      &md2x_hid_set_report_complete,
                                      &md2x_hid_get_report_complete,
                                      &md2x_hid_report_received,
                                      &md2x_has_activity,
                                      &md2x_setupTick,
                                      &md2x_visualTick,
                                      &md2x_tactileTick,
                                      &md2x_executeTick
                                    };

static uint8_t registeredDaddr = 0;
static md2x_instance registeredInstance[2] = {{0, false},
                                              {1, false}};
static bool deviceActivity = false;

////////////////////////////////////////////////////
bool md2x_is_hardware(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return (vid == 0x1b4f && pid == 0x9206);
}

bool md2x_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  md2x_is_hardware(dev_addr)  &&
            dev_addr == registeredDaddr &&
            instance == registeredInstance[instance].instance &&
            registeredInstance[instance].mounted             );
}


void process_md2x(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  md2x_report_t md2x_input_report;
  memcpy(&md2x_input_report, report, sizeof(md2x_input_report));

  #ifdef CONTROLLER_DBG
    debugPort->println("[MD2X] Instance: " + String(instance));
  #endif

  if  ( md2x_input_report.dpad_leftRight  != 0 ||
        md2x_input_report.dpad_upDown     != 0 ||
        md2x_input_report.button_a      ||
        md2x_input_report.button_b      ||
        md2x_input_report.button_c      ||
        md2x_input_report.button_x      ||
        md2x_input_report.button_y      ||
        md2x_input_report.button_z      ||
        md2x_input_report.button_mode   ||
        md2x_input_report.button_start
      )
  {
    deviceActivity = true;
    immediateControllerData.hasActivity = true;
    immediateControllerData.controllerType = CONTROLLER_MD2X;
  } else {
    deviceActivity = false;
  }
  /////////////////////////////
  // Button Mapping
  /////////////////////////////
  //direcitonal controls
  immediateControllerData.dpad_up     |=  (0xff == md2x_input_report.dpad_upDown);
  immediateControllerData.dpad_down   |=  (0x01 == md2x_input_report.dpad_upDown);
  immediateControllerData.dpad_left   |=  (0xff == md2x_input_report.dpad_leftRight);
  immediateControllerData.dpad_right  |=  (0x01 == md2x_input_report.dpad_leftRight);

  //face buttons
  immediateControllerData.apad_south  |= md2x_input_report.button_a;
  immediateControllerData.apad_east   |= md2x_input_report.button_b;
  immediateControllerData.apad_west   |= md2x_input_report.button_x;
  immediateControllerData.apad_north  |= md2x_input_report.button_y;

  //select & start OR share & option
  immediateControllerData.button_start  |= md2x_input_report.button_start;
  immediateControllerData.button_select |= md2x_input_report.button_mode;
  immediateControllerData.button_home   |= md2x_input_report.button_start;

  // shoulder buttons
  //immediateControllerData.button_l1 |= ;
  //immediateControllerData.button_r1 |= ;

  //analog stick buttons
  immediateControllerData.button_l3 |= md2x_input_report.button_c;

  //modern buttons on PlayStation controlelrs
  immediateControllerData.button_touchPadButton |= md2x_input_report.button_z;
}

////////////////////////////////////////////////////
//API functions

void md2x_mount(uint8_t dev_addr, uint8_t instance)
{
  // MD2x Init
  if ( md2x_is_hardware(dev_addr) )
  {
    #ifdef CONTROLLER_DBG
      debugPort->println("[MD2X] Instance: " + String(instance));
    #endif

    if (!registeredInstance[instance].mounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance[instance].instance = instance;
      registeredInstance[instance].mounted = true;
      deviceActivity = false;

      #ifdef CONTROLLER_DBG
        debugPort->println("[Controller] MD2X instance [" + String(instance) + "] mounted!");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef CONTROLLER_DBG
        debugPort->println("[MD2X] Couldn't receive report");
      #endif
    } else {
      #ifdef CONTROLLER_DBG
        debugPort->println("[MD2X] Can receive report!");
      #endif
    }
  }
}

void md2x_umount(uint8_t dev_addr, uint8_t instance)
{
  if  (md2x_is_registeredInstance(dev_addr, instance))
  {
    registeredInstance[instance].mounted = false;
    deviceActivity = false;

    #ifdef CONTROLLER_DBG
      debugPort->println("[Controller] MD2X instance [" + String(instance) + "] removed");
    #endif
  }
}

void md2x_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (md2x_is_registeredInstance(dev_addr, instance))
  {
    process_md2x(report, len, dev_addr, instance);
  }
}

bool md2x_has_activity()
{
  return deviceActivity;
}

////////////////////////////////////////////////////
// Not implemented api functions

void md2x_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void md2x_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void md2x_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void md2x_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void md2x_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void md2x_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}
