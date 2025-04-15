//////////////////////////////////////////////////////////////////////////////////////////////////////
// Referenced:
// https://controllers.fandom.com/wiki/Sony_DualSense#HID_Report_0x02_Output_USB
//////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mydebug.h"

#include "tusb.h"
#include <Arduino.h>

#include "controller_sony_dualsense.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

uint8_t registeredDaddr = 0;
uint8_t registeredInstance = 0;
bool deviceMounted = false;
bool deviceActivity = false;

controller_api_interface sony_dualSense_api = { &dualSense_mount,
                                                &dualSense_umount,
                                                &dualSense_hid_set_report_complete,
                                                &dualSense_hid_get_report_complete,
                                                &dualSense_hid_report_received,
                                                &dualSense_has_activity,
                                                &dualSense_setupTick,
                                                &dualSense_visualTick,
                                                &dualSense_tactileTick,
                                                &dualSense_executeTick
                                              };

// Tick
static int numOfTicks = 0;
static sony_dualSense_output_report_02_t dualSense_output_report = {0};

static int dualSense_ledOffset = 0;
//static const int dualSense_playerLed[2][5] = {{1,  3,  7,  15, 31}, {16, 24, 28, 30, 31}};  // Increasing LED effect
static const int dualSense_playerLed[2][5] = {{0x1e, 0x1d, 0x1b, 0x17, 0xf}, {0xf,  0x17,  0x1b,  0x1d, 0x1e}}; //Flowing black LED effect

static unsigned long rumbleStartTime = 0;

////////////////////////////////////////////////////
//DualSense Check
bool dualSense_is_hardware(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0x054c && pid == 0x0ce6)     //DualSense!
         );
}

bool dualSense_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  dualSense_is_hardware(dev_addr)       &&
            dev_addr == registeredDaddr     &&
            instance == registeredInstance  &&
            deviceMounted                   );
}

void dualSense_defaultLedRumbleState()
{
  //set tpad led
  dualSense_output_report.toggle_tpad_led = 1;
  dualSense_output_report.led_tpad_color_red = 29;
  dualSense_output_report.led_tpad_color_green = 50;
  dualSense_output_report.led_tpad_color_blue = 100;
  dualSense_output_report.led_pulse_option = 0x1;

  //set player number
  dualSense_output_report.toggle_player_led = 1;
  dualSense_output_report.player_led_fade = 1;
  dualSense_output_report.led_player_num = 4;

  //flash mic led when ticking
  dualSense_output_report.toggle_mic_led = 1;

  //stop rumble
  dualSense_output_report.motor_right = 0;
  dualSense_output_report.motor_left = 0;

  // Stiffen triggers to prevent misfiring
  dualSense_output_report.set_right_trigger_motor = dualSense_output_report.set_left_trigger_motor  = 1;
  dualSense_output_report.right_trigger_mode = dualSense_output_report.left_trigger_mode  = 0x21;

  const uint8_t triggerData[10] = { 0x09, 0x09,
                                    0x03, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00  };
  memcpy(&dualSense_output_report.right_trigger_forces, triggerData, 10);
  memcpy(&dualSense_output_report.left_trigger_forces, triggerData, 10);

  dualSense_ledOffset = 0;
  numOfTicks = 0;
}

void process_sony_dualSense (uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  uint8_t const report_id = report[0];
  report++;

  if (report_id == 1)
  {
    sony_dualSense_input_report_t dualSense_input_report;

    memcpy(&dualSense_input_report, report, sizeof(dualSense_input_report));

    if  ( dualSense_input_report.dpad < 8 ||
          dualSense_input_report.square   ||
          dualSense_input_report.cross    ||
          dualSense_input_report.circle   ||
          dualSense_input_report.triangle ||
          dualSense_input_report.l1       ||
          dualSense_input_report.l2       ||
          dualSense_input_report.r1       ||
          dualSense_input_report.r2       ||
          dualSense_input_report.share    ||
          dualSense_input_report.option   ||
          dualSense_input_report.l3       ||
          dualSense_input_report.r3       ||
          dualSense_input_report.ps       ||
          dualSense_input_report.tpad
        )
    {
      deviceActivity = true;
      immediateControllerData.hasActivity = true;
      immediateControllerData.controllerType = CONTROLLER_DUALSENSE;
    } else {
      deviceActivity = false;
      dualSense_defaultLedRumbleState();
    }

    /////////////////////////////
    // Button Mapping
    /////////////////////////////
    //direcitonal controls
    immediateControllerData.dpad_up     |=  (0 == dualSense_input_report.dpad);
    immediateControllerData.dpad_down   |=  (4 == dualSense_input_report.dpad);
    immediateControllerData.dpad_left   |=  (6 == dualSense_input_report.dpad);
    immediateControllerData.dpad_right  |=  (2 == dualSense_input_report.dpad);


    //face buttons
    immediateControllerData.apad_north  |= dualSense_input_report.triangle;
    immediateControllerData.apad_south  |= dualSense_input_report.cross;
    immediateControllerData.apad_east   |= dualSense_input_report.circle;
    immediateControllerData.apad_west   |= dualSense_input_report.square;

    //select & start OR share & option
    immediateControllerData.button_start  |= dualSense_input_report.option;
    immediateControllerData.button_select |= dualSense_input_report.share;
    immediateControllerData.button_home   |= dualSense_input_report.ps;

    // shoulder buttons
    immediateControllerData.button_l1 |= dualSense_input_report.l1;
    immediateControllerData.button_r1 |= dualSense_input_report.r1;

    immediateControllerData.button_l2 |= dualSense_input_report.l2;
    immediateControllerData.button_r2 |= dualSense_input_report.r2;

    //analog stick buttons
    immediateControllerData.button_l3 |= dualSense_input_report.l3;
    immediateControllerData.button_r3 |= dualSense_input_report.r3;

    //modern buttons on PlayStation controlelrs
    immediateControllerData.button_touchPadButton |= dualSense_input_report.tpad;
  }

  //while(!tuh_hid_receive_ready(dev_addr, instance));
  tuh_hid_send_report(registeredDaddr, registeredInstance, DUALSENSE_OUTPUT_REPORT_2, &dualSense_output_report, sizeof(dualSense_output_report));
}

////////////////////////////////////////////////////
//API functions

void dualSense_mount(uint8_t dev_addr, uint8_t instance)
{
  //Sony DualSense
  if ( dualSense_is_hardware(dev_addr) )
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance = instance;

      dualSense_defaultLedRumbleState();

      deviceMounted = true;
      deviceActivity = false;

      #ifdef DS5_DBG
        debugPort->println("[Controller] DualSense mounted!");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef DS5_DBG
        debugPort->println("[DuSe]Couldn't receive report");
      #endif
    } else {
      #ifdef DS5_DBG
        debugPort->println("[DuSe]Can receive report!");
      #endif
    }
  }
}

void dualSense_umount(uint8_t dev_addr, uint8_t instance)
{
  if (dualSense_is_registeredInstance(dev_addr, instance))
  {
    deviceMounted = false;
    deviceActivity = false;
    registeredDaddr = 0;
    registeredInstance = 0;

    #ifdef DS5_DBG
      debugPort->println("[Controller] DualSense removed");
    #endif
  }
}

void dualSense_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (dualSense_is_registeredInstance(dev_addr, instance))
  {
    process_sony_dualSense(report, len, dev_addr, instance);
  }
}

bool dualSense_has_activity()
{
  return deviceActivity;
}

void dualSense_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  dualSense_output_report.toggle_player_led = 1;
  dualSense_output_report.set_rumble_graceful = 1;
  dualSense_output_report.set_rumble_continued = 1;
}

void dualSense_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (dualSense_is_registeredInstance(dev_addr, instance))
  {
    dualSense_output_report.led_player_num = dualSense_playerLed[holdState][dualSense_ledOffset];

    if (0 == numOfTicks % 4)
    {
      if (++dualSense_ledOffset > 4)
      {
        dualSense_ledOffset = 0;
      }
    }

    if (++numOfTicks > 255)
    {
      numOfTicks = 0;
    }
  }
}

void dualSense_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  rumbleStartTime = millis();

  if (HOLDING_RIGHT == holdState)
  {
    dualSense_output_report.motor_right = 0x7;
    dualSense_output_report.motor_left = 0;
  }

  if (HOLDING_LEFT == holdState)
  {
    dualSense_output_report.motor_right = 0;
    dualSense_output_report.motor_left = 0x7;
  }

  dualSense_output_report.mic_led = 1;
}

void dualSense_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (millis() - rumbleStartTime > 50 || false == deviceActivity)
  {
    dualSense_output_report.motor_right = 0;
    dualSense_output_report.motor_left = 0;
    dualSense_output_report.mic_led = 0;
  }
}

////////////////////////////////////////////////////
// Not implemented api functions

void dualSense_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}

void dualSense_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{}
