#include "class/hid/hid.h"
#include "mydebug.h"

#include <Arduino.h>

#include "controller_sony_ds4.h"
#include "usb_hid.h"
#include "supported_controllers.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
#endif

extern game_controller_data immediateControllerData;

controller_api_interface sony_ds4_api = { &ds4_mount,
                                          &ds4_umount,
                                          &ds4_hid_set_report_complete,
                                          &ds4_hid_get_report_complete,
                                          &ds4_hid_report_received,
                                          &ds4_has_activity,
                                          &ds4_setupTick,
                                          &ds4_visualTick,
                                          &ds4_tactileTick,
                                          &ds4_executeTick,
                                          &ds4_housekeepingRoutine
                                          };

const device_id_entry supportedDevices[NUM_OF_DS4_DEVICES] = {DS4_VID_PID_ARRAY};

// Device
static uint8_t registeredDaddr = 0;
static uint8_t registeredGamepadInstance = 0;
static uint8_t registeredCommandInstance = 1;
static bool deviceMounted = false;
static bool deviceActivity = false;

// Tick
static int numOfTicks = 0;
static int rgbPhase = 2;
int phaseIncrementSign[3][3] = {{-1,  1,  0},
                                { 0, -1,  1},
                                { 1,  0, -1}};
sony_ds4_output_report_set_controller_state_t ds4_output_report = {0};
static unsigned long rumbleStartTime = 0;
ds4_light_bar_color lightBarData = DS4_LIGHT_BAR_RAINBOW_START;

bool ds4ConnectedToDongle = false;
uint8_t ds4DongleControllerConnectedPreviously = DS4_DONGLE_CONTROLLER_NOT_CONNECTED;
unsigned long ds4LastActivityTime = 0;
bool ds4ShutdownRequestSent = true;

////////////////////////////////////////////////////
// check if device is Sony DualShock 4
bool ds4_is_hardware(uint8_t dev_addr)
{
  bool result = false;
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  for (int i = 0; i < NUM_OF_DS4_DEVICES; i++)
  {
    if (vid == supportedDevices[i].vid && pid == supportedDevices[i].pid)
    {
      result = true;
    }
  }

  return result;
}

bool ds4_is_registeredGamepadInstance(uint8_t dev_addr, uint8_t instance)
{
  return (  ds4_is_hardware(dev_addr)       &&
            dev_addr == registeredDaddr     &&
            instance == registeredGamepadInstance  &&
            deviceMounted                   );
}

void ds4_defaultLedRumbleState()
{
  //set LED color
  ds4_output_report.set_led = 1;
  ds4_output_report.lightbar_red    = lightBarData.red / DS4_LIGHT_BAR_FRACTION;
  ds4_output_report.lightbar_green  = lightBarData.green / DS4_LIGHT_BAR_FRACTION;
  ds4_output_report.lightbar_blue   = lightBarData.blue / DS4_LIGHT_BAR_FRACTION;

  //stop rumble
  ds4_output_report.set_rumble = 1;
  ds4_output_report.motor_right = 0;
  ds4_output_report.motor_left = 0;
}

void process_sony_ds4(uint8_t const* report, uint16_t len, uint8_t dev_addr, uint8_t instance)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
        
  uint8_t const report_id = report[0];
  report++;

  if (DS4_INPUT_REPORT == report_id)
  {
    sony_ds4_report_t ds4_input_report;
    memcpy(&ds4_input_report, report, sizeof(ds4_input_report));

    if  ( ds4_input_report.dpad < 8 ||
          ds4_input_report.square   ||
          ds4_input_report.cross    ||
          ds4_input_report.circle   ||
          ds4_input_report.triangle ||
          ds4_input_report.l1       ||
          ds4_input_report.l2       ||
          ds4_input_report.r1       ||
          ds4_input_report.r2       ||
          ds4_input_report.share    ||
          ds4_input_report.option   ||
          ds4_input_report.l3       ||
          ds4_input_report.r3       ||
          ds4_input_report.ps       ||
          ds4_input_report.tpad
        )
    {
      deviceActivity = true;
      immediateControllerData.hasActivity = true;
      immediateControllerData.controllerType = CONTROLLER_DUALSHOCK4;
      ds4LastActivityTime = millis();
      ds4ShutdownRequestSent = false;
    } else {
      deviceActivity = false;

      lightBarData = DS4_LIGHT_BAR_DEFAULT;

      numOfTicks = 0;
      rgbPhase = 0;
      ds4_defaultLedRumbleState();
    }

    // DS4 wireless dongle special logic
    if (DS4_SONY_VID == vid && DS4_SONY_ADAPTER_PID == pid)
    {
      // If a controller is connected
      if (DS4_DONGLE_CONTROLLER_CONNECTED == ds4_input_report.dongle_not_connected)
      {
        ds4ConnectedToDongle = true;

        // It just connected, wasn't connected - Reset idle countdown when dongle
        if (DS4_DONGLE_CONTROLLER_NOT_CONNECTED == ds4DongleControllerConnectedPreviously)
        {
          ds4LastActivityTime = millis();
          deviceActivity = true;
          ds4ShutdownRequestSent = false;
#ifdef DS4_DBG
          debugPort->println("[DS4] Controller connected to dongle");
#endif          
        }
      } else {
        //not connected at all
        ds4ConnectedToDongle = false;
        deviceActivity = false;
        ds4ShutdownRequestSent = true;

        // Just disconnected
        if (DS4_DONGLE_CONTROLLER_CONNECTED == ds4DongleControllerConnectedPreviously)
        {
#ifdef DS4_DBG
          debugPort->println("[DS4] Controller disconnected from dongle");
#endif     
        }
      }
    }
    // End of DS4 wireless dongle special logic

/*
    // Idle power down routine
    if (millis() - ds4LastActivityTime > DS4_IDLE_SHUTDOWN_TIME)
    {
      //DS4 wireless dongle
      if (DS4_SONY_VID == vid && DS4_SONY_ADAPTER_PID == pid)
      {
        // If a controller is connected AND a shutdown request has not been sent
        if( DS4_DONGLE_CONTROLLER_CONNECTED == ds4_input_report.dongle_not_connected &&
            false == ds4ShutdownRequestSent)
        {
          
          //send report           
          static sony_ds4_feature_report_t shutDownReport = {0};
          //shutDownReport.report_id = DS4_DONGLE_SHUTDOWN;
          shutDownReport.data = 0x02;
          
          bool success = tuh_hid_set_report(dev_addr, instance, DS4_DONGLE_SHUTDOWN, HID_REPORT_TYPE_FEATURE, &shutDownReport, sizeof(shutDownReport));

          ds4ShutdownRequestSent = true;

#ifdef DS4_DBG
          if (true == success)          
          {
            debugPort->println("[DS4] Dongle shutdown command sent successfully");
          } else {
            debugPort->println("[DS4] Dongle shutdown command failed to sent");
          }
#endif    
        }  
      }
    }
*/

    /////////////////////////////
    // Button Mapping
    /////////////////////////////
    //direcitonal controls
    immediateControllerData.dpad_up     |=  (0 == ds4_input_report.dpad);
    immediateControllerData.dpad_down   |=  (4 == ds4_input_report.dpad);
    immediateControllerData.dpad_left   |=  (6 == ds4_input_report.dpad);
    immediateControllerData.dpad_right  |=  (2 == ds4_input_report.dpad);

    //face buttons
    immediateControllerData.apad_north  |= ds4_input_report.triangle;
    immediateControllerData.apad_south  |= ds4_input_report.cross;
    immediateControllerData.apad_east   |= ds4_input_report.circle;
    immediateControllerData.apad_west   |= ds4_input_report.square;

    //select & start OR share & option
    immediateControllerData.button_start  |= ds4_input_report.option;
    immediateControllerData.button_select |= ds4_input_report.share;
    immediateControllerData.button_home   |= ds4_input_report.ps;

    // shoulder buttons
    immediateControllerData.button_l1 |= ds4_input_report.l1;
    immediateControllerData.button_r1 |= ds4_input_report.r1;

    immediateControllerData.button_l2 |= ds4_input_report.l2;
    immediateControllerData.button_r2 |= ds4_input_report.r2;

    //analog stick buttons
    immediateControllerData.button_l3 |= ds4_input_report.l3;
    immediateControllerData.button_r3 |= ds4_input_report.r3;

    //modern buttons on PlayStation controlelrs
    immediateControllerData.button_touchPadButton |= ds4_input_report.tpad;

    // store dongle connection state
    ds4DongleControllerConnectedPreviously = ds4_input_report.dongle_not_connected;
  }  

  tuh_hid_send_report(registeredDaddr, registeredGamepadInstance, DS4_OUTPUT_REPORT_SET_CONTROLLER_STATE, &ds4_output_report, sizeof(ds4_output_report));
}

////////////////////////////////////////////////////
//API functions

bool ds4_mount(uint8_t dev_addr, uint8_t instance)
{
  // Sony DualShock 4 [CUH-ZCT2x] Init
  if ( ds4_is_hardware(dev_addr) )
  {
    if (!deviceMounted)
    {
      registeredDaddr = dev_addr;

      if (instance == 0)
      {
        registeredGamepadInstance = instance;
      } else {
        registeredCommandInstance = instance;
      }
      deviceMounted = true;
      deviceActivity = false;

      ds4_defaultLedRumbleState();

      #ifdef DS4_DBG
        debugPort->println("[Controller] DS4 mounted!");
      #endif
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      #ifdef DS4_DBG
        debugPort->println("[DS4]Couldn't receive report");
      #endif
    } else {
      #ifdef DS4_DBG
        debugPort->println("[DS4]Can receive report!");
      #endif
    }

    return true;
  }

  return false;
}

void ds4_umount(uint8_t dev_addr, uint8_t instance)
{
  if (ds4_is_registeredGamepadInstance(dev_addr, instance))
  {
    deviceMounted = false;
    deviceActivity = false;
    registeredDaddr = 0;
    registeredGamepadInstance = 0;
    ds4ShutdownRequestSent = true;

    #ifdef DS4_DBG
      debugPort->println("[Controller] DS4 removed");
    #endif
  }
}

void ds4_hid_report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (ds4_is_registeredGamepadInstance(dev_addr, instance))
  {
    process_sony_ds4(report, len, dev_addr, instance);
  }
}

bool ds4_has_activity()
{
  return deviceActivity;
}

void ds4_setupTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
}

void ds4_visualTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (ds4_is_registeredGamepadInstance(dev_addr, instance))
  {
    if (0 == numOfTicks && 0 == rgbPhase)
    {
      lightBarData = DS4_LIGHT_BAR_RAINBOW_START;
    }

    lightBarData.red   += DS4_RGB_STEP * phaseIncrementSign[rgbPhase][0];
    lightBarData.green += DS4_RGB_STEP * phaseIncrementSign[rgbPhase][1];
    lightBarData.blue  += DS4_RGB_STEP * phaseIncrementSign[rgbPhase][2];

    if (lightBarData.red < DS4_LIGHT_BAR_RGB_MIN)
    {
      lightBarData.red = DS4_LIGHT_BAR_RGB_MIN;

    } else if (lightBarData.red > DS4_LIGHT_BAR_RGB_MAX)
    {
      lightBarData.red = DS4_LIGHT_BAR_RGB_MAX;
    }

    if (lightBarData.green < DS4_LIGHT_BAR_RGB_MIN)
    {
      lightBarData.green = DS4_LIGHT_BAR_RGB_MIN;

    } else if (lightBarData.green > DS4_LIGHT_BAR_RGB_MAX)
    {
      lightBarData.green = DS4_LIGHT_BAR_RGB_MAX;
    }

    if (lightBarData.blue < DS4_LIGHT_BAR_RGB_MIN)
    {
      lightBarData.blue = DS4_LIGHT_BAR_RGB_MIN;

    } else if (lightBarData.blue > DS4_LIGHT_BAR_RGB_MAX)
    {
      lightBarData.blue = DS4_LIGHT_BAR_RGB_MAX;
    }

    #ifdef DS4_DBG
      debugPort->println("[DS4] R: " + String(lightBarData.red) + " G: " + String(lightBarData.green) + " B: " + String(lightBarData.blue));
    #endif

    ds4_output_report.lightbar_red    = lightBarData.red / DS4_LIGHT_BAR_FRACTION;
    ds4_output_report.lightbar_green  = lightBarData.green / DS4_LIGHT_BAR_FRACTION;
    ds4_output_report.lightbar_blue   = lightBarData.blue / DS4_LIGHT_BAR_FRACTION;

    numOfTicks += DS4_RGB_STEP;

    if (numOfTicks > 255)
    {
      numOfTicks = 0;

      if (++rgbPhase > 2)
      {
        rgbPhase = 0;
      }
    }
  }
}

void ds4_tactileTick(holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  rumbleStartTime = millis();

  ds4_output_report.set_rumble = 1;

  if (HOLDING_RIGHT == holdState)
  {
    ds4_output_report.motor_right = 200;
    ds4_output_report.motor_left = 0;
  } else {
    ds4_output_report.motor_right = 0;
    ds4_output_report.motor_left  = 255;
  }

  #ifdef DS4_DBG
    debugPort->println("[DS4] Rumble!");
  #endif


}

void ds4_executeTick (holding_left_right holdState, uint8_t dev_addr, uint8_t instance)
{
  if (millis() - rumbleStartTime > 75 || false == deviceActivity)
  {
    #ifdef DS4_DBG
      debugPort->println("[DS4] Rumble Stop!");
    #endif
    //ds4_output_report.set_rumble = 0;
    ds4_output_report.motor_right = 0;
    ds4_output_report.motor_left = 0;
  }
}


void ds4_hid_set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
#ifdef DS4_DBG
    debugPort->printf(
        "[DS4] SET_REPORT completed: ID=%02X, type=%02X, length=%u\n",
        report_id, report_type, len
    );
#endif
}

void ds4_housekeepingRoutine() {
  uint16_t vid, pid;
  tuh_vid_pid_get(registeredDaddr, &vid, &pid);

  // Idle power down routine
  if (millis() - ds4LastActivityTime > DS4_IDLE_SHUTDOWN_TIME)
  {
    //DS4 wireless dongle
    if (DS4_SONY_VID == vid && DS4_SONY_ADAPTER_PID == pid)
    {
      // If a controller is connected AND a shutdown request has not been sent
      if( true == ds4ConnectedToDongle &&
          false == ds4ShutdownRequestSent)
      {
        //send report
        
        static sony_ds4_feature_report_t shutDownReport = {0};
        //shutDownReport.report_id = DS4_DONGLE_SHUTDOWN;
        shutDownReport.data = 0x02;
        /*
        bool success = tuh_hid_set_report ( registeredDaddr,          \
                                            registeredGamepadInstance,  \
                                            DS4_DONGLE_SHUTDOWN,        \
                                            HID_REPORT_TYPE_FEATURE,    \
                                            &shutDownReport,            \
                                            sizeof(shutDownReport));
        */

        ds4ShutdownRequestSent = true;

#ifdef DS4_DBG
        //bool success = true; // dummy yes
        
        if (true == success)          
        {
          debugPort->printf("[DS4] Dongle shutdown command sent successfully\n");
        } else {
          debugPort->printf("[DS4] Dongle shutdown command failed to sent\n");
        }
#endif    
      }  
    }
  }
}

////////////////////////////////////////////////////
// Not implemented api functions
void ds4_hid_get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}