#include "mydebug.h"

#include "Adafruit_TinyUSB.h"

#include "usb_host.h"
#include "mypico.h"
#include "rt4k.h"

#ifdef DEBUG
  extern HardwareSerial* debugPort;
  int msgCounter = 0;
#endif

Adafruit_USBH_Host USBHost;
Adafruit_USBH_CDC SerialHost;

String msgFromUsbSerial;

unsigned long lastActivityTime = millis();
bool usbActivity = false;

/////////////// FUNCTION PROTOTYPES ////////////////
  static void sniffRt4kState();
  static bool isCdcDeviceRt4k(uint8_t);
  static void usbActivitySet();
  
  #ifdef DEBUG
    static void echoFromUsbSerialToDebugSerial();
  #endif
//////////// END OF FUNCTION PROTOTYPES ////////////

void usbHost_init()
{ 
  // Init USB Host on native controller roothub port0
  USBHost.begin(USB_NATIVE_HOST_ROOT_PORT);
}

void routine_usbHostFtdi()
{
  USBHost.task();
  sniffRt4kState();

  if (millis() - lastActivityTime > 500)
  {
    usbActivity = false;
  }
}

void usbHostFtdi_sendToUsbSerial(String newMessage)
{
  newMessage += "\n";

  if (SerialHost && SerialHost.connected()) 
  {
    SerialHost.write(newMessage.c_str(), newMessage.length());
    SerialHost.flush();
  }
}

bool usb_isThereActivity()
{
  return usbActivity;
}

void usb_stopUsbHost()
{
  tuh_rhport_reset_bus(USB_NATIVE_HOST_ROOT_PORT, tuh_rhport_is_active(USB_NATIVE_HOST_ROOT_PORT));
  delay(50);
  tuh_deinit(USB_NATIVE_HOST_ROOT_PORT);
}

void usb_restartUsbHost()
{
  delay(100);
  tuh_init(USB_NATIVE_HOST_ROOT_PORT);
}

// Internal Functions
static void sniffRt4kState()
{
  if (SerialHost.available())
  {
  //set RT4K to ON state when detected
    if (true == SerialHost.findUntil(RT4K_UART_MSG_POWER_ON, "\n"))//(0 == strcmp(newMsg.c_str(), RT4K_UART_MSG_POWER_ON))
    {
      rt4k_setPowerState(RT4K_ON);
      usbActivitySet();
    }
    //set RT4K to OFF state when detected
    if (true == SerialHost.findUntil(RT4K_UART_MSG_POWER_OFF, "\n"))//(0 == strcmp(newMsg.c_str(), RT4K_UART_MSG_POWER_OFF))
    {
      rt4k_setPowerState(RT4K_OFF);
      usbActivitySet();
    }
  }
}

#ifdef DEBUG
  static void echoFromUsbSerialToDebugSerial() 
  { // SerialHost -> Serial
    if (SerialHost.connected() && SerialHost.available()) {
      msgFromUsbSerial = SerialHost.readString();

      // echo to the other serial port
      debugPort->println(msgFromUsbSerial);
      debugPort->flush();
    }
  }
#endif

static bool isCdcDeviceRt4k(uint8_t idx)
{
  tuh_itf_info_t info;
  uint16_t vid, pid;

  tuh_cdc_itf_get_info(idx, &info);  
  tuh_vid_pid_get(info.daddr, &vid, &pid);

  if (RT4K_USB_VID == vid && RT4K_USB_PID == pid)
  {
    #ifdef CDC_DBG
      debugPort->println("[CDC] CDC device is RT4K");
    #endif

    return true;
  } else {
    #ifdef CDC_DBG
      debugPort->println("[CDC] CDC device is NOT RT4K");
    #endif
    
    //cdch_close(info.daddr);
    return false;
  }
}

static void usbActivitySet()
{
  usbActivity = true;
  lastActivityTime = millis();
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+1
void tuh_cdc_mount_cb(uint8_t idx) 
{
  #ifdef CDC_DBG
    tuh_itf_info_t info;
    tuh_cdc_itf_get_info(idx, &info);    
    debugPort->println("[CDC] Device ID: " + String(idx) + ", Addr: " + String(info.daddr, HEX));

    uint16_t vid, pid;
    tuh_vid_pid_get(info.daddr, &vid, &pid);
    debugPort->println("[CDC] Device Vid: " + String(vid, HEX) + ", Pid: " + String(pid, HEX));
  #endif
  
  // bind SerialHost object to this interface index
  if (isCdcDeviceRt4k(idx))
  {
    SerialHost.mount(idx);
    SerialHost.begin(RT4K_USBC_UART_BAUD_RATE);
    rt4k_notifyUsbcConnected();

    usbActivitySet();

    #ifdef CDC_DBG
      debugPort->println("[CDC] Mounted CDC device to SerialHost");
    #endif
  }
}

// Invoked when a device with CDC interface is unmounted
void tuh_cdc_umount_cb(uint8_t idx)
{
  if (isCdcDeviceRt4k(idx))
  {
    rt4k_notifyUsbcDisonnected();
    SerialHost.end();
    SerialHost.umount(idx);

    usbActivitySet();

    #ifdef CDC_DBG
      debugPort->println("[CDC] SerialHost is unmounted");
    #endif
  }
}