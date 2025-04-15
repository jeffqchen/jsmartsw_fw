#include "mydebug.h"

#include "tusb.h"

#include <Arduino.h>

#include "controller_sony_ds3.h"
#include "usb_controller.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

// API
controller_api_interface sony_ds3_api = { &ds3_mount,
                                          &ds3_umount,
                                          &ds3_hid_set_report_complete,
                                          &ds3_hid_get_report_complete,
                                          &ds3_hid_report_received,
                                          &ds3_has_activity,
                                          &ds3_setupTick,
                                          &ds3_visualTick,
                                          &ds3_tactileTick,
                                          &ds3_executeTick
                                          };

// Device
static uint8_t registeredDaddr = 0;
static uint8_t registeredInstance = 0;
static bool deviceMounted = false;
static bool deviceActivity = false;

// Tick
static sony_ds3_output_report_01_t ds3_output_report;
static const sony_ds3_output_report_01_t ds3OutputReportTemplate = DS3_OUTPUT_REPORT_TEMPLATE;
static unsigned long rumbleStartTime = 0;

static        ds3_led_mask ds3LedMask[2][DS3_NUM_LEDS];
static const  ds3_led_mask defaultLedMask[2][DS3_NUM_LEDS] = { {{0xf0, 1}, {0x80, 1}, {0x40, 1}, {0x0,  1}},
                                                               {{0x0,  1}, {0x40, 1}, {0x80, 1}, {0xf0, 1}} };

////////////////////////////////////////////////////
// DS3 check
bool ds3_is_hardware(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return (SONY_DS3_VID == vid && SONY_DS3_PID == pid);
}

bool ds3_is_registeredInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  ds3_is_hardware(dev_addr)       &&
            dev_addr == registeredDaddr     &&
            instance == registeredInstance  &&
            deviceMounted                   );
}

void ds3_defaultLedRumbleState()
{
  const sony_ds3_output_report_01_t ds3OutputReportTemplate = DS3_OUTPUT_REPORT_TEMPLATE;
  memcpy (&ds3_output_report, &ds3OutputReportTemplate, sizeof(sony_ds3_output_report_01_t));

  //turn off motor
  ds3_output_report.small_motor_duration = 0;
  ds3_output_report.small_motor_strength = 0;
  ds3_output_report.large_motor_duration = 0;
  ds3_output_report.large_motor_strength = 0;

  ds3_output_report.led_bitmap =  DS3_LED_1_ON  |
                                  DS3_LED_2_ON  |
                                  DS3_LED_3_ON  |
                                  DS3_LED_4_ON;
  //reset LED effect
  memcpy(ds3LedMask, defaultLedMask, sizeof(ds3LedMask));
}

void process_sony_ds3(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  uint8_t const report_id = report[0];
  report++;

  if (report_id == 1)
  {
    sony_ds3_input_report_t ds3_input_report;
    memcpy(&ds3_input_report, report, sizeof(ds3_input_report));

    if  ( ds3_input_report.dpad_up    ||
          ds3_input_report.dpad_down  ||
          ds3_input_report.dpad_left  ||
          ds3_input_report.dpad_right ||
          ds3_input_report.square     ||
          ds3_input_report.cross      ||
          ds3_input_report.circle     ||
          ds3_input_report.triangle   ||
          ds3_input_report.l1         ||
          ds3_input_report.l2         ||
          ds3_input_report.r1         ||
          ds3_input_report.r2         ||
          ds3_input_report.start      ||
          ds3_input_report.select     ||
          ds3_input_report.l3         ||
          ds3_input_report.r3         ||
          ds3_input_report.ps
        )
    {
      deviceActivity = true;
      immediateControllerData.hasActivity = true;
      immediateControllerData.controllerType = CONTROLLER_DUALSHOCK3;
    } else {
      deviceActivity = false;
      ds3_defaultLedRumbleState();
    }

    /////////////////////////////
    // Button Mapping
    /////////////////////////////
    //direcitonal controls
    immediateControllerData.dpad_up     |=  ds3_input_report.dpad_up;
    immediateControllerData.dpad_down   |=  ds3_input_report.dpad_down;
    immediateControllerData.dpad_left   |=  ds3_input_report.dpad_left;
    immediateControllerData.dpad_right  |=  ds3_input_report.dpad_right;

    //face buttons
    immediateControllerData.apad_north  |= ds3_input_report.triangle;
    immediateControllerData.apad_south  |= ds3_input_report.cross;
    immediateControllerData.apad_east   |= ds3_input_report.circle;
    immediateControllerData.apad_west   |= ds3_input_report.square;

    //select & start OR share & option
    immediateControllerData.button_start  |= ds3_input_report.start;
    immediateControllerData.button_select |= ds3_input_report.select;
    immediateControllerData.button_home   |= ds3_input_report.ps;

    // shoulder buttons
    immediateControllerData.button_l1 |= ds3_input_report.l1;
    immediateControllerData.button_r1 |= ds3_input_report.r1;

    immediateControllerData.button_l2 |= ds3_input_report.l2;
    immediateControllerData.button_r2 |= ds3_input_report.r2;

    //analog stick buttons
    immediateControllerData.button_l3 |= ds3_input_report.l3;
    immediateControllerData.button_r3 |= ds3_input_report.r3;
  }

  tuh_hid_send_report(registeredDaddr, registeredInstance, DS3_OUTPUT_REPORT_ID, &ds3_output_report, sizeof(sony_ds3_output_report_01_t));
}

////////////////////////////////////////////////////
//API functions

void ds3_mount(uint8_t dev_addr, uint8_t instance)
{
  // Sony DualShock3 Init
  if ( ds3_is_hardware(dev_addr) )
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;
      registeredInstance = instance;
      deviceMounted = true;
      deviceActivity = false;

      while (!tuh_hid_receive_ready(dev_addr, instance))
      {
        #ifdef DS3_DBG
          debugPort->println("[DS3]Waiting for DS3");
        #endif
      }

      //Stage1 DS3 setup
      u_char getBuf[17];
      tuh_hid_get_report(dev_addr, instance, 0xf2, HID_REPORT_TYPE_FEATURE , getBuf, 17);

      #ifdef DS3_DBG
        debugPort->println("[DS3]Stage1: Set USB mode");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef DS3_DBG
        debugPort->println("[DS3]Couldn't receive report");
      #endif
    } else {
      #ifdef DS3_DBG
        debugPort->println("[DS3]Can receive report!");
      #endif
    }
  }
}

void ds3_umount(uint8_t dev_addr, uint8_t instance)
{
  if (ds3_is_registeredInstance(dev_addr, instance))
  {
    deviceMounted = false;
    deviceActivity = false;
    registeredDaddr = 0;
    registeredInstance = 0;

    #ifdef DS3_DBG
      debugPort->println("[Controller] DS3 removed");
    #endif
  }
}

void ds3_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
  //Stage3 DS3 setup
  if  (ds3_is_registeredInstance(dev_addr, instance))
  {
    if (0 == len)
    {
      #ifdef DS3_DBG
        debugPort->println("[DS3]Stage3: Possible set report error!");
      #endif
    } else {
      #ifdef DS3_DBG
        debugPort->println("[DS3]Stage3: Set report completed");
      #endif
    }
  }
}

void ds3_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
  //Stage2 DS3 setup
  if  (ds3_is_registeredInstance(dev_addr, instance))
  {
    if (0 == len)
    {
      #ifdef DS3_DBG
        debugPort->println("[DS3]Stage2: Get report error!");
      #endif
    } else {
      if (0xf2 == report_id)
      {
        #ifdef DS3_DBG
          debugPort->println("[DS3]Stage2: Official method enabled. Enabling unofficial method");
        #endif

        u_char getBuf[8];
        tuh_hid_get_report(dev_addr, instance, 0xf5, HID_REPORT_TYPE_FEATURE , getBuf, 8);
      }

      if (0xf5 == report_id)
      {
        #ifdef DS3_DBG
          debugPort->println("[DS3]Stage2: Unofficial method enabled. Get report completed. Start getting USB report");
        #endif

        u_char enableHidReport[4] = {0x42, 0x0c, 0x0, 0x0};
        tuh_hid_set_report(dev_addr, instance, 0xf4, HID_REPORT_TYPE_FEATURE , enableHidReport, 4);
      }
    }
  }
}

void ds3_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (ds3_is_registeredInstance(dev_addr, instance))
  {
    process_sony_ds3(report, len, dev_addr, instance);
  }
}

bool ds3_has_activity()
{
  return deviceActivity;
}

void ds3_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  memcpy (&ds3_output_report, &ds3OutputReportTemplate, sizeof(sony_ds3_output_report_01_t));
  ds3_output_report.led_bitmap = DS3_LED_1_ON|DS3_LED_2_ON|DS3_LED_3_ON|DS3_LED_4_ON;
}

void ds3_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  for (int i = 0; i < DS3_NUM_LEDS; i++)
  {
    if (ds3LedMask[holdState][i].maskValue + ds3LedMask[holdState][i].stepSign * DS3_LED_ANIMATION_STEP < DS3_LED_DUTY_MIN + DS3_LED_ANIMATION_STEP)
    {
      ds3LedMask[holdState][i].maskValue = DS3_LED_DUTY_MIN;
      ds3LedMask[holdState][i].stepSign *= -1;
    } else if (ds3LedMask[holdState][i].maskValue + ds3LedMask[holdState][i].stepSign * DS3_LED_ANIMATION_STEP > DS3_LED_DUTY_MAX - DS3_LED_ANIMATION_STEP)
    {
      ds3LedMask[holdState][i].maskValue = DS3_LED_DUTY_MAX;
      ds3LedMask[holdState][i].stepSign *= -1;
    } else {
      ds3LedMask[holdState][i].maskValue += ds3LedMask[holdState][i].stepSign * DS3_LED_ANIMATION_STEP;
    }

    ds3_output_report.led_parameter[i].time_enabled = DS3_LED_TIME_ENABLED;
    ds3_output_report.led_parameter[i].duty_length = DS3_LED_DUTY_LENGTH;
    ds3_output_report.led_parameter[i].enabled = 1;
    ds3_output_report.led_parameter[i].duty_off = ds3LedMask[holdState][i].maskValue;
    ds3_output_report.led_parameter[i].duty_on = DS3_LED_DUTY_MAX - ds3LedMask[holdState][i].maskValue;
  }
}

void ds3_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  #ifdef DS3_DBG
    debugPort->println("[DS3] Rumble!");
  #endif

  rumbleStartTime = millis();

  if (HOLDING_RIGHT == holdState)
  {
    ds3_output_report.small_motor_duration = 0xff;
    ds3_output_report.small_motor_strength = 0xff;
    ds3_output_report.large_motor_duration = 0x0;
    ds3_output_report.large_motor_strength = 0x0;
  }

  if (HOLDING_LEFT == holdState)
  {
    ds3_output_report.small_motor_duration = 0x0;
    ds3_output_report.small_motor_strength = 0x0;
    ds3_output_report.large_motor_duration = 0xff;
    ds3_output_report.large_motor_strength = 0xff;
  }
}

void ds3_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (millis() - rumbleStartTime > 100 || false == deviceActivity)
  {
    #ifdef DS3_DBG
      debugPort->println("[DS3] Rumble Stop!");
    #endif

    ds3_output_report.small_motor_duration = 0;
    ds3_output_report.small_motor_strength = 0;
    ds3_output_report.large_motor_duration = 0;
    ds3_output_report.large_motor_strength = 0;
  }
}
