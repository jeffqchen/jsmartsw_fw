#include "class/hid/hid.h"
#include "mydebug.h"

#include "tusb.h"
#include <Arduino.h>

#include "controller_mouse.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData, storedControllerData;
//game_controller_data immediateControllerData;

controller_api_interface mouse_api = {&mouse_mount,
                                      &mouse_umount,
                                      &mouse_hid_set_report_complete,
                                      &mouse_hid_get_report_complete,
                                      &mouse_hid_report_received,
                                      &mouse_has_activity,
                                      &mouse_setupTick,
                                      &mouse_visualTick,
                                      &mouse_tactileTick,
                                      &mouse_executeTick
                                    };

static uint8_t registeredDaddr = 0;
static uint8_t registeredInstance = 0;
static bool deviceMounted = false;
static bool mouse_button_activity = false;
static bool mouse_axis_activity = false;
static unsigned long lastActivityTime = 0;
static mouse_input_report_t previous_mouse_input_report = {0};

////////////////////////////////////////////////////
bool mouse_is_hardware(uint8_t dev_addr, uint8_t instance)
{
  bool result = false;

  if (2 == tuh_hid_interface_protocol(dev_addr, instance))
  {
    result = true;
  }
  return result;
}

bool mouse_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  mouse_is_hardware(dev_addr, instance) &&
            dev_addr == registeredDaddr           &&
            instance == registeredInstance        &&
            deviceMounted                         );
}

void process_mouse(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  static bool bothMouseButtonPressed = false;

  mouse_input_report_t mouse_input_report;
  memcpy(&mouse_input_report, report, sizeof(mouse_input_report_t));

  if (previous_mouse_input_report.button != mouse_input_report.button)
  {
    if (mouse_input_report.button == MOUSE_BUTTON_NONE)
    {
      lastActivityTime = millis();

      if (previous_mouse_input_report.button != MOUSE_BUTTON_NONE)
      {
        mouse_button_activity = true;
        immediateControllerData.hasActivity = true;
        immediateControllerData.controllerType = CONTROLLER_MOUSE;

        // translate buttons
        if (true == bothMouseButtonPressed)
        {
          immediateControllerData.button_start|= true;
          bothMouseButtonPressed = false;

        } else if (previous_mouse_input_report.button == MOUSE_BUTTON_R) {
          immediateControllerData.apad_east   |= true;

        } else if (previous_mouse_input_report.button == MOUSE_BUTTON_L) {
          immediateControllerData.apad_south  |= true;
        }

        // other buttons
        if (previous_mouse_input_report.button == MOUSE_BUTTON_5)
        {
            immediateControllerData.button_r1   |= true;

        } else if (previous_mouse_input_report.button == MOUSE_BUTTON_4)
        {
            immediateControllerData.button_l1   |= true;
        }

        if (previous_mouse_input_report.button == MOUSE_BUTTON_MIDDLE)
        {
            immediateControllerData.button_l3   |= true;
        }
      }
    } else {
      if ((MOUSE_BUTTON_L | MOUSE_BUTTON_R) == mouse_input_report.button)
      {
        bothMouseButtonPressed = true;
      }
    }
  }

  if (mouse_input_report.wheel == MOUSE_WHEEL_UP)
  {
    immediateControllerData.button_l2     |= true;

  } else if (mouse_input_report.wheel == MOUSE_WHEEL_DOWN)
  {
    immediateControllerData.button_r2     |= true;
  }

  //////////////////////////////////////////////////////////////////
  // Axis

  if (mouse_input_report.axis_x != MOUSE_AXIS_NEUTRAL ||
      mouse_input_report.axis_y != MOUSE_AXIS_NEUTRAL   )
  {
    mouse_axis_activity = true;
    immediateControllerData.hasActivity = true;
    immediateControllerData.controllerType = CONTROLLER_MOUSE;
  } else {
    mouse_axis_activity = false;
    immediateControllerData.hasActivity = false;
  }

  //translate axis
  if (mouse_input_report.axis_y < MOUSE_AXIS_MIDDLE  &&
      mouse_input_report.axis_y > MOUSE_AXIS_DEADZONE_DOWN)
  {
    immediateControllerData.dpad_down |= true;

  } else if(mouse_input_report.axis_y > MOUSE_AXIS_MIDDLE &&
            mouse_input_report.axis_y < MOUSE_AXIS_DEADZONE_UP)
  {
    immediateControllerData.dpad_up |= true;

  } else if(mouse_input_report.axis_x < MOUSE_AXIS_MIDDLE &&
            mouse_input_report.axis_x > MOUSE_AXIS_DEADZONE_RIGHT)
  {
    immediateControllerData.dpad_right |= true;

  } else if(mouse_input_report.axis_x > MOUSE_AXIS_MIDDLE &&
            mouse_input_report.axis_x < MOUSE_AXIS_DEADZONE_LEFT)
  {
    immediateControllerData.dpad_left |= true;
  }

  //take down the last time of report coming in
  if (  mouse_input_report.axis_x != MOUSE_AXIS_NEUTRAL ||
        mouse_input_report.axis_y != MOUSE_AXIS_NEUTRAL ||
        mouse_input_report.wheel  != MOUSE_BUTTON_NONE    )
  {
    lastActivityTime = millis();
  }

  memcpy(&previous_mouse_input_report, &mouse_input_report, sizeof(mouse_input_report_t));
}

////////////////////////////////////////////////////
//API functions

void mouse_mount(uint8_t dev_addr, uint8_t instance)
{
  if (mouse_is_hardware(dev_addr, instance))
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance = instance;
      deviceMounted = true;

      mouse_button_activity = false;
      mouse_axis_activity = false;

      #ifdef MOUSE_DBG
        debugPort->println("[Mouse] Mouse mounted!");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef MOUSE_DBG
        debugPort->println("[Mouse] Couldn't receive report");
      #endif
    } else {
      #ifdef MOUSE_DBG
        debugPort->println("[Mouse] Can receive report!");
      #endif
    }
  }
}

void mouse_umount(uint8_t dev_addr, uint8_t instance)
{
  if (mouse_is_registeredInstance(dev_addr, instance))
  {
    deviceMounted = false;
    mouse_button_activity = false;
    mouse_axis_activity = false;
    registeredDaddr = 0;
    registeredInstance = 0;

    #ifdef MOUSE_DBG
      debugPort->println("[Mouse] Mouse removed");
    #endif
  }
}

void mouse_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  /*
  #ifdef MOUSE_DBG
    debugPort->println("[Mouse] Daddr: " + String(dev_addr) + ", Instance: " + String(instance) + ", Protocl: " + String(tuh_hid_interface_protocol(dev_addr, instance)));
  #endif
  */

  if (mouse_is_registeredInstance(dev_addr, instance))
  {
    process_mouse(report, len, dev_addr, instance);
  }
}

void mouse_timeout()
{
  if (mouse_is_registeredInstance(storedControllerData.dev_addr, storedControllerData.instance) )
  {
    if (millis() - lastActivityTime > MOUSE_TIMEOUT_THRESHOLD)
    {
      //clearControllerData(&storedControllerData);
      storedControllerData = game_controller_data();
      mouse_axis_activity = false;
      mouse_button_activity = false;
    }
  }
}

bool mouse_has_activity()
{
  return mouse_axis_activity || mouse_button_activity;
}

////////////////////////////////////////////////////
// Not implemented api functions

void mouse_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void mouse_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void mouse_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void mouse_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void mouse_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void mouse_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}
