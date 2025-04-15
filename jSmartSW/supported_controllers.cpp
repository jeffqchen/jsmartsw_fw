#include "supported_controllers.h"
#include "usb_controller.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

////////////////////////////////////////////////////////////////
//Controller entities

extern controller_api_interface keyboard_api;
extern controller_api_interface mouse_api;

extern controller_api_interface sony_ds3_api;
extern controller_api_interface sony_ds4_api;
extern controller_api_interface sony_dualSense_api;
extern controller_api_interface switchProCon_api;
extern controller_api_interface saturn_usb_api;
extern controller_api_interface psx2usb_api;
extern controller_api_interface md2x_api;

//empty entry required for controller entity array to exist
controller_api_interface * controllerEntity[NUM_OF_API];
controller_api_interface empty_api = {&empty_mount,
                                      &empty_umount,
                                      &empty_hid_set_report_complete,
                                      &empty_hid_get_report_complete,
                                      &empty_hid_report_received,
                                      &empty_has_activity,
                                      &empty_setupTick,
                                      &empty_visualTick,
                                      &empty_tactileTick,
                                      &empty_executeTick
                                    };

void controller_init_deviceRegister()
{
  /*
  #ifdef CONTROLLER_DBG
    debugPort->println("[Controller] Initializing API");
  #endif
  */
  controllerEntity[API_ID_EMPTY]      = &empty_api;

  controllerEntity[API_ID_KEYBOARD]   = &keyboard_api;
  controllerEntity[API_ID_MOUSE]      = &mouse_api;

  controllerEntity[API_ID_DUALSHOCK3] = &sony_ds3_api;
  controllerEntity[API_ID_DUALSHOCK4] = &sony_ds4_api;
  controllerEntity[API_ID_DUALSENSE]  = &sony_dualSense_api;
  controllerEntity[API_ID_SWITCHPRO]  = &switchProCon_api;
  controllerEntity[API_ID_SATURN_USB] = &saturn_usb_api;
  controllerEntity[API_ID_PSX2USB]    = &psx2usb_api;
  controllerEntity[API_ID_MD2X]       = &md2x_api;
}

////////////////////////////////////////////////////
//API functions

void empty_mount(uint8_t dev_addr, uint8_t instance)
{}

void empty_umount(uint8_t dev_addr, uint8_t instance)
{}

void empty_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void empty_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void empty_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{}

bool empty_has_activity()
{
  return false;
}

void empty_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void empty_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void empty_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}

void empty_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{}