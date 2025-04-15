#include "SerialUART.h"
#include "mydebug.h"

#include "gsw_hardware.h"

#include "rt4k.h"

#include "mypico.h"
#include "rgbled.h"
#include "usb_host.h"

#ifdef DEBUG
  SerialPIO Serial3(14, 15);
  SerialPIO* debugPort = NULL;

  #ifdef RT4K_DBG   // RT4K debug purpose text
    char throbber[]={'|', '/', '-', '\\'};
    unsigned int throbberIndex = 0;
  #endif
#endif

static SerialUART* hd15Uart;

static unsigned int uartPortInUse = RT4K_UART_PORT_TYPE_HD15;
static unsigned int inputToKeepAlive = 0;
static unsigned long lastTinkKeepAliveTime = 0;

static unsigned long rt4kPowerTransitionMoment = 0;
static bool rt4kPowerStateTransition = false;
static bool rt4kPowerUpSvsUpdateSent = true;
static unsigned int rt4kPowerState = RT4K_UNKNOWN;

static int currentTabNum = 0;

static String quickTabPages[NUM_QUICK_TABS] =  {RT4K_IR_REMOTE_INPUT,
                                                RT4K_IR_REMOTE_OUTPUT,
                                                RT4K_IR_REMOTE_SCALER,
                                                RT4K_IR_REMOTE_SFX,
                                                RT4K_IR_REMOTE_ADC,
                                                RT4K_IR_REMOTE_PROF };

/////////////// FUNCTION PROTOTYPES ////////////////
  static void rt4kPowerRoutine();    //internal power routine
  static void clearRt4kPowerFlags();
//////////// END OF FUNCTION PROTOTYPES ////////////

void rt4k_uartInit()
{
  #ifdef DEBUG
    Serial3.begin(115200, SERIAL_8N1);
    
    debugPort = &Serial3;

    debugPort->println("\n=[ jSmartSW ]=");
    debugPort->flush();

    if ( watchdog_enable_caused_reboot())
    {
      debugPort->println("[JSW] Last restarted by Watchdog_Enable");
      debugPort->flush();
    }

    if (watchdog_caused_reboot())
    {
      debugPort->println("[JSW] Last restarted by Watchdog");
      debugPort->flush();
    }

    debugPort->println("[JSW] Initializing...");
    debugPort->flush();
  #endif

  //HD15 serial
  Serial1.begin(RT4K_HD15_UART_BAUD_RATE, RT4K_HD15_UART_CONFIG);
  Serial1.println("ver");
  hd15Uart = &Serial1;

  rt4k_notifyUsbcDisonnected();
}

void rt4k_inputChange(unsigned int newInput)
{
  #ifdef RT4K_DBG
    debugPort->println("[RT4K] Sending change command to RT4K");
  #endif

  //Send input change message to RT4K
  if (RT4K_UART_PORT_TYPE_HD15 == uartPortInUse)
  {
    hd15Uart->println(String(RT4K_UART_COMMAND_NEW_INPUT) + String(newInput));
    hd15Uart->flush();
  } else if (RT4K_UART_PORT_TYPE_USBC == uartPortInUse)
  {
    usbHostFtdi_sendToUsbSerial(String(RT4K_UART_COMMAND_NEW_INPUT) + String(newInput));
  }
  inputToKeepAlive = newInput;
  lastTinkKeepAliveTime = millis();
}

void routine_rt4kCommunication()
{
  rt4kPowerRoutine();

  /////////////////////////////
  //send rt4k keepalive message
  if (millis() - lastTinkKeepAliveTime > TINK_KEEPALIVE_INTERVAL) 
  {
    #ifdef RT4K_DBG
      debugPort->println("[RT4K] Current Input: " + ((inputToKeepAlive!=0)?String(inputToKeepAlive):"No Input!") + " " + throbber[throbberIndex] + " Switch operation mode: " + String(pico_getGswMode() ? "AUTO" : "MANUAL"));
    #endif

    if(inputToKeepAlive > NO_INPUT)
    {
      #ifdef RT4K_DBG
        debugPort->println("[RT4K] Keep Alive Input: " + String(inputToKeepAlive) + " " + throbber[throbberIndex]);
      #endif

      //Send keepalive message to RT4K
      if (RT4K_UART_PORT_TYPE_HD15 == uartPortInUse)
      {
        hd15Uart->println(String(RT4K_UART_COMMAND_KEEPALIVE) + String(inputToKeepAlive));
      } else if (RT4K_UART_PORT_TYPE_USBC == uartPortInUse)
      {//using USB CDC communication
        usbHostFtdi_sendToUsbSerial(String(RT4K_UART_COMMAND_KEEPALIVE) + String(inputToKeepAlive));
      }
    }

    #ifdef RT4K_DBG
      throbberIndex ++;
      throbberIndex %= 4;
    #endif

    lastTinkKeepAliveTime = millis();
  }
  //end of send rt4k keepalive message
}

////////////////////////////////
// USB-C Notifications
////////////////////////////////

void rt4k_notifyUsbcConnected()
{
  uartPortInUse = RT4K_UART_PORT_TYPE_USBC;
  rt4kPowerState = RT4K_UNKNOWN;
  pico_notifyControlInterfaceUsbc();
}

void rt4k_notifyUsbcDisonnected()
{
  uartPortInUse = RT4K_UART_PORT_TYPE_HD15;
  rt4kPowerState = RT4K_UNKNOWN;
  pico_notifyControlInterfaceHd15();
}

bool rt4k_attemptingPowerAction()
{
  return rt4kPowerStateTransition;
}

static void rt4kPowerRoutine()
{
  // power transtion action in effect  
  if (true == rt4kPowerStateTransition)
  {
    //rgbled_notifyRt4kPowerTransitionAttempt(true);  //flash rt4k power led while attempting power up

    // during time of attempt
    if (millis() - rt4kPowerTransitionMoment < ((RT4K_PRO_BOOT_TIME > RT4K_CE_BOOT_TIME) ? RT4K_PRO_BOOT_TIME : RT4K_CE_BOOT_TIME))
    {
      #ifdef RT4K_DBG
        debugPort->println("Power Transition!");
        debugPort->flush();
      #endif

      //power state is on and message is not sent yet
      if (RT4K_ON == rt4kPowerState)
      {
        #ifdef RT4K_DBG
          debugPort->println("Power ON received!");
          debugPort->flush();
        #endif
        rt4kPowerStateTransition = false; // interrupt power transition state

        if (false == rt4kPowerUpSvsUpdateSent)
        {
          if (inputToKeepAlive > NO_INPUT)
          {
            rt4k_inputChange(inputToKeepAlive);
            #ifdef RT4K_DBG
              debugPort->println("Sent SVS!");
              debugPort->flush();
            #endif
          }
        }
        rt4kPowerUpSvsUpdateSent = true;  //mark SVS request as sent        
      }

      if (RT4K_OFF == rt4kPowerState)
      {
        rt4kPowerStateTransition = false; // interrupt power transition state
        #ifdef RT4K_DBG
          debugPort->println("Power OFF received!");
          debugPort->flush();
        #endif
      }
    } else { // reached the end of power up attempt deadline
      if(false == rt4kPowerUpSvsUpdateSent)  //message hasn't been sent
      {
        #ifdef RT4K_DBG
          debugPort->println("Power action timed out!");
          debugPort->flush();
        #endif
        //send SVS profile request IF valid input is registered
        if (inputToKeepAlive > NO_INPUT)
        {
          rt4k_inputChange(inputToKeepAlive);
          #ifdef RT4K_DBG
            debugPort->println("Sent SVS!");
            debugPort->flush();
          #endif
        }
        rt4kPowerUpSvsUpdateSent = true;  //mark SVS request as sent
      }
      rt4kPowerStateTransition = false; // end power transition state
    }
  } else {  // No power transition action
  }
}


////////////////////////////////
// RT4K command macros
////////////////////////////////
void rt4k_cmdPowerToggle()
{
  //send remote power button action first
  if (RT4K_UART_PORT_TYPE_HD15 == uartPortInUse)
  {
    hd15Uart->println(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_BUTTON_PWR));
    rt4k_remoteWakeFromSleep();
  } else if (RT4K_UART_PORT_TYPE_USBC == uartPortInUse)
  {//using USB CDC communication
    rt4kPowerStateTransition = true;

    switch (rt4kPowerState)
    {
      case RT4K_ON:
        usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_BUTTON_PWR)); //send the power command on the remote to turn it off
        rt4kPowerState = RT4K_OFF;
        rt4kPowerStateTransition = false;
        break;
      case RT4K_OFF:
        rt4k_remoteWakeFromSleep();
        break;
      default:
        usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_BUTTON_PWR)); //send the power command on the remote to turn it off
        rt4k_remoteWakeFromSleep();
    }
  } 
}

void rt4k_cmdRes4k()
{
  if (RT4K_UART_PORT_TYPE_HD15 == uartPortInUse)
  {
    hd15Uart->println(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_4K));
    delay(200);
    hd15Uart->println(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_RIGHT));
    delay(200);
    hd15Uart->println(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_OK));
  } else if (RT4K_UART_PORT_TYPE_USBC == uartPortInUse)
  {//using USB CDC communication
    usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_4K));
    delay(200);
    usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_RIGHT));
    delay(200);
    usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_OK));
  }
}

void rt4k_cmdRes1080p()
{
  if (RT4K_UART_PORT_TYPE_HD15 == uartPortInUse)
  {
    hd15Uart->println(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_1080P));
    delay(200);
    hd15Uart->println(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_RIGHT));
    delay(200);
    hd15Uart->println(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_OK));
  } else if (RT4K_UART_PORT_TYPE_USBC == uartPortInUse)
  {//using USB CDC communication
    usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_1080P));
    delay(200);
    usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_RIGHT));
    delay(200);
    usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + String(RT4K_IR_REMOTE_OK));
  }
}

void rt4k_cmdInputHD15()
{
  //waiting for Mike to implement input command
}

void rt4k_cmdInputSCART()
{
  //waiting for Mike to implement input command
}

void rt4k_remoteButtonSingle(String buttonString)
{
  if (RT4K_UART_PORT_TYPE_HD15 == uartPortInUse)
  {
    hd15Uart->println(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + buttonString);
  } else if (RT4K_UART_PORT_TYPE_USBC == uartPortInUse)
  {//using USB CDC communication
    usbHostFtdi_sendToUsbSerial(String(RT4K_UART_REMOTE_COMMAND_PREFIX) + buttonString);
  }
}

static void clearRt4kPowerFlags()
{
  rt4kPowerTransitionMoment = millis();  //take down time of pwr on command sent
  rt4kPowerStateTransition = true;     //enter rt4k power up attempting state
  rt4kPowerUpSvsUpdateSent = false; //set SVS input update to "Not sent yet"
  rt4kPowerState = RT4K_UNKNOWN;
}

void rt4k_remoteWakeFromSleep()
{
  if (RT4K_UART_PORT_TYPE_HD15 == uartPortInUse)
  {
    hd15Uart->println(String(RT4K_UART_COMMAND_POWER_ON_FROM_SLEEP));
  } else if (RT4K_UART_PORT_TYPE_USBC == uartPortInUse)
  {//using USB CDC communication
    if (RT4K_ON != rt4kPowerState)
    {
      usbHostFtdi_sendToUsbSerial(String(RT4K_UART_COMMAND_POWER_ON_FROM_SLEEP));
    }
  }
  clearRt4kPowerFlags();
}

void cycleQuickTabs(int increment)
{
  //static int currentTabNum = 0;

  currentTabNum += increment;

  if (currentTabNum > NUM_QUICK_TABS - 1)
  {
    currentTabNum = 0;
  }

  if (currentTabNum < 0)
  {
    currentTabNum = 5;
  }
  
  rt4k_remoteButtonSingle(quickTabPages[currentTabNum]);
}

void rt4k_cycleQuickTabsIncrease()
{
  cycleQuickTabs(1);
}

void rt4k_cycleQuickTabsDecrease()
{
  cycleQuickTabs(-1);
}

void rt4k_setPowerState(unsigned int newState)
{
  rt4kPowerState = newState;
}

unsigned int rt4k_getPowerState()
{
  return rt4kPowerState;
}

unsigned int rt4k_getConnection()
{
  return uartPortInUse;
}