#include "mydebug.h"

#include "tusb.h"

#include "mypico.h"
#include "controller_keyboard.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

keyboard_data immediateKeyboardData;

controller_api_interface keyboard_api = { &keyboard_mount,
                                          &keyboard_umount,
                                          &keyboard_hid_set_report_complete,
                                          &keyboard_hid_get_report_complete,
                                          &keyboard_hid_report_received,
                                          &keyboard_has_activity,
                                          &keyboard_setupTick,
                                          &keyboard_visualTick,
                                          &keyboard_tactileTick,
                                          &keyboard_executeTick
                                        };

static uint8_t registeredDaddr = 0;
static uint8_t registeredInstance = 0;
static bool deviceMounted = false;
static bool deviceActivity = false;

/*
static mouse_report_t previous_mouse_input_report = {0};
static bool mouse_button_activity = false;
static bool mouse_axis_activity = false;
static unsigned long mouseLastDebounceTime = 0;
*/
////////////////////////////////////////////////////
bool keyboard_is_hardware(uint8_t dev_addr, uint8_t instance)
{
  bool result = false;

  if (1 == tuh_hid_interface_protocol(dev_addr, instance))
  {
    result = true;
  }

  return result;
}

bool keyboard_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  keyboard_is_hardware(dev_addr, instance)  &&
            dev_addr == registeredDaddr               &&
            instance == registeredInstance            &&
            deviceMounted                             );
}

void process_keyboard(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  keyboard_report_t keyboard_input_report = {0};
  memcpy(&keyboard_input_report, report, sizeof(keyboard_input_report));

  if  ( keyboard_input_report.modifier  ||
        keyboard_input_report.keycode[0] )
  {
    deviceActivity = true;
    immediateKeyboardData.hasActivity = true;
  } else {
    deviceActivity = false;
     immediateKeyboardData.hasActivity = false;
  }

  immediateKeyboardData.modifier = keyboard_input_report.modifier;
  immediateKeyboardData.firstKey = keyboard_input_report.keycode[0];
}

////////////////////////////////////////////////////
//API functions

void keyboard_mount(uint8_t dev_addr, uint8_t instance)
{
  if ( keyboard_is_hardware(dev_addr, instance) )
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance = instance;

      deviceMounted = true;
      deviceActivity = false;

      #ifdef KEYBOARD_DBG
        debugPort->println("[Keyboard] Keyboard mounted!");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef KEYBOARD_DBG
        debugPort->println("[Keyboard] Couldn't receive report");
      #endif
    } else {
      #ifdef KEYBOARD_DBG
        debugPort->println("[Keyboard] Can receive report!");
      #endif
    }
  }
}

void keyboard_umount(uint8_t dev_addr, uint8_t instance)
{
  if (keyboard_is_registeredInstance(dev_addr, instance))
  {
    deviceMounted = false;
    deviceActivity = false;
    registeredDaddr = 0;
    registeredInstance = 0;

    #ifdef KEYBOARD_DBG
      debugPort->println("[Keyboard] Keyboard removed");
    #endif
  }
}

void keyboard_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  /*
  #ifdef KEYBOARD_DBG
    debugPort->println("[Keyboard] Daddr: " + String(dev_addr) + ", Instance: " + String(instance) + ", Protocl: " + String(tuh_hid_interface_protocol(dev_addr, instance)));
  #endif
  */

  if (keyboard_is_registeredInstance(dev_addr, instance))
  {
    process_keyboard(report, len, dev_addr, instance);
  }
}

bool keyboard_has_activity()
{
  return deviceActivity;
}

////////////////////////////////////////////////////
// Not implemented api functions

void keyboard_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void keyboard_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void keyboard_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void keyboard_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void keyboard_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void keyboard_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

////////////////////////////////////////////////////

keyboard_data keyboard_currentReading()
{
  return immediateKeyboardData;
}
