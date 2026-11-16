#include "mydebug.h"

#include "gsw_hardware.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/regs/intctrl.h"
#include "SerialUART.h"

#include "tinkComm.h"

#include "mypico.h"
#include "rgbled.h"

#ifdef DEBUG
extern HardwareSerial* debugPort;

#ifdef TINK_DBG  // RT4K debug purpose text
char throbber[] = { '|', '/', '-', '\\' };
unsigned int throbberIndex = 0;
#endif
#endif

/////////////////////////////
// From USB Host
/////////////////////////////

tusb_rhport_init_t dev_init = {
  .role = TUSB_ROLE_HOST,
  .speed = TUSB_SPEED_HIGH
};

#include "Adafruit_TinyUSB.h"

Adafruit_USBH_Host USBHost;
Adafruit_USBH_CDC cdcUart;

bool tinkActivity = false;
unsigned long lastTinkActivityTime = 0;

bool u1lowerBaudRateSet = false;
bool u1BaudRateChangeStep1 = false;
bool u1BaudRateChangeStep2 = false;
bool u1BaudRateChangeStep3 = false;
unsigned long u1BaudChangeTimeoutWindowTime = 0;
unsigned long u1BaudChangeWindowTime = 0;

/////////////// FUNCTION PROTOTYPES ////////////////
static void sniffRt4kState();
static bool isCdcDeviceRt4k(uint8_t);
static void tinkActivitySet();

bool sendMessageToSerialUart(String, SerialUART *);
bool sendMessageToUsbFtdi(String, Adafruit_USBH_CDC*);
bool sendMessageToUsbFtdiNoBlock(String, Adafruit_USBH_CDC*);

#ifdef TINK_MONITOR_STATUS
  static void tink_u5_rx_ISR();
#endif
//////////// END OF FUNCTION PROTOTYPES ////////////

/////////////////////////////
//SerialUART* hd15Uart;

// State variables that are NOT about the Tink device itself
static unsigned int svsInputToKeepAlive = 0;
static unsigned long lastTinkKeepAliveTime = 0;
static unsigned long tinkPowerTransitionMoment = 0;
static bool tinkPowerStateTransition = false;
static unsigned long tinkPowerOnAttemptTime = 0;
static bool tinkPowerUpSvsUpdateSent = true;

static int currentTabNum = 0;
static String quickTabPages[NUM_QUICK_TABS] = { QUICK_TABS_ARRAY };

// Circular Buffer Implementation
// Initialize the buffer
void buffer_init(SerialBuffer* cb) {
  cb->head = 0;
  cb->tail = 0;
  cb->pushLock = false;
}

uart_hw_t* u5UartRegs = uart_get_hw(PICO_U5_UART_PORT_HW); // Hardware register pointer for U5
SerialBuffer u1_rx_buffer;
SerialBuffer u5_rx_buffer;

// Check if buffer is empty
bool buffer_is_empty(SerialBuffer* cb) {
  return (cb->head == cb->tail);
}

// Check if buffer is full
bool buffer_is_full(SerialBuffer* cb) {
  return (((cb->head + 1) & CIRCULAR_BUFFER_MASK) == cb->tail);
}

// Push a byte into the buffer (Called by Serial/UART ISR)
bool buffer_push(SerialBuffer* cb, uint8_t data) {
  cb->pushLock = true;

  uint16_t next_head = (cb->head + 1) & CIRCULAR_BUFFER_MASK;

  if (next_head == cb->tail) {
    cb->pushLock = false;
    return false;  // Buffer overflow! No space left.
  }

  cb->buffer[cb->head] = data;
  cb->head = next_head;

  cb->pushLock = false;
  return true;
}

// Pop a byte from the buffer (Called by Main Loop)
bool buffer_pop(SerialBuffer* cb, uint8_t* data) {
  while(cb->pushLock);

  if (buffer_is_empty(cb)) {
    return false;  // Buffer empty!
  }

  *data = cb->buffer[cb->tail];
  cb->tail = (cb->tail + 1) & CIRCULAR_BUFFER_MASK;
  return true;
}

////////////////////////////////////
// One and only Tink device
////////////////////////////////////
tinkStates myTink = {
                              RT_DEVICE_UNKOWN,
                              TINK_POWER_UNKNOWN,
                              TINK_UART_PORT_TYPE_HD15,
                              TINK_INPUT_HDMI,
                              TINK_OUTPUT_STOCK_RES_480P,
                              &cdcUart,
                              { false,
                                false,
                                0,
                                0,
                                0,
                                &u1_rx_buffer,
                                NULL },
                              &PICO_U5_UART_PORT_SW,
                              {
                                true,
                                false,
                                0,
                                0,
                                0,
                                &u5_rx_buffer,
                                NULL,
                              }
                            };

/////////////// FUNCTION PROTOTYPES ////////////////
static void tinkStateDigest();  //internal power routine
//static void clearRt4kPowerFlags();
static void sendMessageToTink(String);
//////////// END OF FUNCTION PROTOTYPES ////////////

void tink_uartInit() {
  // U5 Software Serial Setup
  pinMode(PICO_U5_RX_PIN, INPUT_PULLUP);
  PICO_U5_UART_PORT_SW.setTX(PICO_U5_TX_PIN);
  PICO_U5_UART_PORT_SW.setRX(PICO_U5_RX_PIN);
  PICO_U5_UART_PORT_SW.begin(TINK_U5_UART_BAUD_RATE, TINK_U5_UART_CONFIG);
  PICO_U5_UART_PORT_SW.println("TINK_UART_COMMAND_ECHO_VERSION");
  //PICO_U5_UART_PORT_SW.flush();

  buffer_init(&u1_rx_buffer);
  buffer_init(&u5_rx_buffer);

  tink_notifyUsbcDisonnected();
}

// Interrupt register to specific core by running in the core's setup()
void tink_uartInterruptRegister()
{
#ifdef TINK_MONITOR_STATUS  
  attachInterrupt(digitalPinToInterrupt(PICO_U5_RX_PIN), tink_u5_rx_ISR, FALLING);
#endif
}

void tink_svsInputChange(unsigned int newInput) {
#ifdef TINK_DBG
  debugPort->println("[TINK] Sending SVS input change command to TINK Device");
#endif

  //Send input change message to RT4K
  sendMessageToTink(String(TINK_UART_COMMAND_NEW_INPUT) + String(newInput));

  svsInputToKeepAlive = newInput;
  lastTinkKeepAliveTime = millis();
}

#ifdef TINK_MONITOR_STATUS
void tuh_cdc_rx_cb(uint8_t idx)
{
  // U1 RX
  if (tuh_cdc_mounted(idx))
  {
    #define U1_CB_BUFFER_SIZE 64
    uint8_t buf[U1_CB_BUFFER_SIZE];
    int readCount = 0;

    if (tuh_cdc_read_available(idx) > 0)
    {
      readCount = tuh_cdc_read(idx, buf, U1_CB_BUFFER_SIZE);
    }

    for (int i = 0; i < readCount; i++)
    {
      buffer_push(&u1_rx_buffer, buf[i]);
    }
  }
}

void tink_u5_rx_ISR() 
{
  if (TINK_UART_PORT_TYPE_HD15 == myTink.activeUartPort)
  {
    // Check if the interrupt flag for Received Data is set
    if (!(u5UartRegs->fr & UART_UARTFR_RXFE_BITS)) 
    {
      uint8_t received_byte = u5UartRegs->dr;  // Read from hardware register

      // Push to buffer. Ignore return unless handling overflows.
      buffer_push(&u5_rx_buffer, received_byte);
    }
  }
}
#endif

void tinkSendStatusUpdateRequest()
{
  static String tinkUpdateMsgArray[TINK_MSG_END]={TINK_UPDATE_MSG_STRING_ARRAY};
  static int tinkUpdateMsgIndex = 0;

  // No power state check, always send
  if (TINK_UART_PORT_TYPE_USBC == myTink.activeUartPort)
  {
    if (millis() - myTink.u1.lastStatusUpdateSendTime > TINK_STATUS_UPDATE_INTERVAL) 
    {
#ifdef TINK_UART_DBG
      debugPort->println("[U1] Sending status update request to U1");
#endif
      if (sendMessageToUsbFtdi(tinkUpdateMsgArray[tinkUpdateMsgIndex], myTink.u1Uart))
      {
        tinkUpdateMsgIndex++;
        tinkUpdateMsgIndex %= TINK_MSG_END;
        myTink.u1.lastMessageSendTime = myTink.u1.lastStatusUpdateSendTime = millis();
      }
    }
  } else {
    if (millis() - myTink.u5.lastStatusUpdateSendTime > TINK_STATUS_UPDATE_INTERVAL) 
    {
#ifdef TINK_UART_DBG
      debugPort->println("[U5] Sending status update request to U5");
#endif
      if(sendMessageToSerialUart(tinkUpdateMsgArray[tinkUpdateMsgIndex], myTink.u5Uart))
      {
        tinkUpdateMsgIndex++;
        tinkUpdateMsgIndex %= TINK_MSG_END;
        myTink.u5.lastMessageSendTime = myTink.u5.lastStatusUpdateSendTime = millis();
      }
    }
  }
}

int extractNumber(const char* msg, int offset)
{
  int result = 99;
  char buf[8];

  int idx = 0;
  while (msg[offset + idx] != ' ' && idx < 7)
  {
    buf[idx] = msg[offset + idx];
    idx++;
  }
  buf[idx] = '\0';

  result = atoi(buf);

  return result;
}

int compareForActivityAndUpdate(uint8_t* oldValue, uint8_t* newValue)
{
  if (NULL == oldValue || NULL == newValue)
  {
    return -1;  // pointer error
  }

  if (*oldValue != *newValue)
  {
    tinkActivitySet();
    *oldValue = *newValue;
    return 1; //different
  }

  return 0; //same
}

void tink_parseSerialMessage(const char* msg, int len, bool* portHasUpdate) {  
  if (0 == strncmp(msg, TINK_MODEL_MSG_LEAD, sizeof(TINK_MODEL_MSG_LEAD) - 1))
  {
    uint8_t device = extractNumber(msg, sizeof(TINK_MODEL_MSG_LEAD) - 1);
    compareForActivityAndUpdate(&myTink.tinkDevice, &device);
#ifdef TINK_UART_DBG
    debugPort->println("[Parse] model is " + String(myTink.tinkDevice));
#endif    
  } else if (0 == strncmp(msg, TINK_INPUT_MSG_LEAD, sizeof(TINK_INPUT_MSG_LEAD) - 1))
  {
    uint8_t inputPort = extractNumber(msg, sizeof(TINK_INPUT_MSG_LEAD) - 1);
    compareForActivityAndUpdate(&myTink.activeInputPort, &inputPort);
#ifdef TINK_UART_DBG
    debugPort->println("[Parse] input is " + String(myTink.activeInputPort));
#endif
  } else if (0 == strncmp(msg, TINK_OUTPUT_MSG_LEAD, sizeof(TINK_OUTPUT_MSG_LEAD) - 1))
  {
    uint8_t outputRes = extractNumber(msg, sizeof(TINK_OUTPUT_MSG_LEAD) - 1);
    compareForActivityAndUpdate(&myTink.activeOutputRes, &outputRes);
#ifdef TINK_UART_DBG
    debugPort->println("[Parse] output is " + String(myTink.activeOutputRes));
#endif
  } else if (0 == strncmp(msg, TINK_BAUD_CHANGE_REPLY_1, sizeof(TINK_BAUD_CHANGE_REPLY_1) - 1))
  {
    tuh_cdc_set_baudrate_sync(myTink.u1Uart->getInterfaceIndex(), (uint32_t)TINK_USBC_UART_BAUD_RATE_LOW);
    u1BaudChangeWindowTime = millis();
    u1BaudRateChangeStep2 = true;
#ifdef TINK_UART_DBG
    debugPort->println("[Baud 2] set window for sending baud ok");
#endif    
  } else if (0 == strncmp(msg, TINK_BAUD_CHANGE_CONFIRM_MSG_LEAD, sizeof(TINK_BAUD_CHANGE_CONFIRM_MSG_LEAD) - 1))
  {
    u1lowerBaudRateSet = true;
    myTink.activeUartPort = TINK_UART_PORT_TYPE_USBC;
    tinkActivitySet();
#ifdef TINK_UART_DBG
    debugPort->println("[Baud 4] lower baud is confirmed");
#endif
  } else {
#ifdef TINK_UART_DBG
      debugPort->println("Unhandled message: " + String(msg));
#endif
  }  
}

void processRxBuffer(SerialBuffer* circularBuf, unsigned long* lastMessageTime, bool* portHasUpdate) {
  uint8_t byte;
  static char msg_frame[1024];
  static unsigned int msg_idx = 0;

#ifdef TINK_UART_DBG
    //debugPort->println("[UART] In buffer processor");
#endif

  while (buffer_pop(circularBuf, &byte)) {

#ifdef TINK_UART_DBG
    //debugPort->printf("0x%02hhX ", byte);
#endif

    if (msg_idx < (sizeof(msg_frame) - 1)) {
      msg_frame[msg_idx++] = (char)byte;
      //*lastMessageTime = millis();
    }
    *lastMessageTime = millis();
  }

  if (millis() - *lastMessageTime > TINK_UART_MESSAGE_WINDOW) {
    if (msg_idx > 0) {
      msg_frame[msg_idx] = '\0';  // Null-terminate

#ifdef TINK_UART_DBG
      debugPort->println(">>>>>>>>>>>>[MSG]<<<<<<<<<<<<\n" + \
                          String(msg_frame) + \
                         "\n>>>>>>>" + String(msg_idx) + " bytes <<<<<");
#endif

      // Route message to your application parser
      tink_parseSerialMessage(msg_frame, msg_idx, portHasUpdate);

      msg_idx = 0;  // Reset index for next message
    }
  }
}

void sendMessageToTink(String msg) 
{
  // NO send unless power is confirmed ON!
  if (TINK_POWER_OFF != myTink.powerState)
  {
    if (TINK_UART_PORT_TYPE_HD15 == myTink.activeUartPort) {
      if (millis() - myTink.u5.lastMessageSendTime > TINK_COMMAND_RATE_CONTROL_PERIOD)
      {
        if(sendMessageToSerialUart(msg, myTink.u5Uart))
        {
          myTink.u5.lastMessageSendTime = millis();
        }
      }
    } else if (TINK_UART_PORT_TYPE_USBC == myTink.activeUartPort) {  //using USB CDC communication
      if (millis() - myTink.u1.lastMessageSendTime > TINK_COMMAND_RATE_CONTROL_PERIOD)
      {
        if (sendMessageToUsbFtdi(msg, myTink.u1Uart))
        {
          myTink.u1.lastMessageSendTime = millis();
        }
      }
    }
  }
}

void routine_tinkCommunication() {

#ifdef TINK_SWITCH_TO_SLOW_BAUD
  // Switch to lower baud rate
  if (true == myTink.u1.portPresent     &&
      false == u1lowerBaudRateSet )
  {
    if (false == u1BaudRateChangeStep1)
    { // step 1 - now request change
      String baudChangeCmd = String(TINK_UART_BAUD_RATE_COMMAND_PREFIX) + String(TINK_USBC_UART_BAUD_RATE_LOW);

      myTink.u1Uart->flush();
      sendMessageToUsbFtdiNoBlock(baudChangeCmd, myTink.u1Uart);
      u1BaudChangeTimeoutWindowTime = millis();

      u1BaudRateChangeStep1 = true;
      u1BaudRateChangeStep2 = false;
      u1BaudRateChangeStep3 = false;
#ifdef TINK_UART_DBG
    debugPort->println("[U1] Step 1 - Sending baud rate change request: " + baudChangeCmd);
#endif         
    } else {
      if ( true == u1BaudRateChangeStep1 &&
           true == u1BaudRateChangeStep2 &&
           false == u1BaudRateChangeStep3 )
      {// step 1 requested, but step 2 hasn't done
        if (millis() - u1BaudChangeWindowTime > 100)
        { // after 100ms, send "baud ok"
          tuh_cdc_read_clear(myTink.u1Uart->getInterfaceIndex());
          sendMessageToUsbFtdiNoBlock(String(TINK_CDC_BAUD_CHANGE_CONFIRM), myTink.u1Uart);
          u1BaudRateChangeStep3 = true;
  #ifdef TINK_UART_DBG
          debugPort->println("[U1] Step 3 - Send baud rate change ok message");
  #endif
        }
      }

      if (millis() - u1BaudChangeTimeoutWindowTime > TINK_CDC_BAUD_CHANGE_TIMEOUT)
      { // step 1 done, but timed out
        myTink.u1Uart->setBaudrate(TINK_USBC_UART_BAUD_RATE_DEFAULT);
        u1BaudRateChangeStep1 = false;  //retry
        //u1lowerBaudRateSet = true;        //no retry
        u1BaudRateChangeStep1 = u1BaudRateChangeStep2 = u1BaudRateChangeStep3 =false;
#ifdef TINK_UART_DBG
        debugPort->println("[U1] Baud rate change timed out");
#endif     
      }
    }          
  }
#else
  if (true == myTink.u1.portPresent)
  {
    myTink.activeUartPort = TINK_UART_PORT_TYPE_USBC;
  }
#endif

  // Tink-side house keeping
#ifdef TINK_MONITOR_STATUS
  // Process the fast response RX buffers
  if (TINK_UART_PORT_TYPE_USBC == myTink.activeUartPort)
  {
    processRxBuffer(&u1_rx_buffer, &myTink.u1.lastMessageReceiveTime, &myTink.u1.hasUpdate);
  } else {
    processRxBuffer(&u5_rx_buffer, &myTink.u5.lastMessageReceiveTime, &myTink.u5.hasUpdate);
  }

  tinkStateDigest();

  //Send update request for next cycle  
  tinkSendStatusUpdateRequest();
#endif

#ifdef TINK_STATE_DBG
  if (true == myTink.u1.hasUpdate || true == myTink.u5.hasUpdate) {
    debugPort->printf("[Tink State] Device=%d, Power=%s, UART=%s, Input=%d, Output=%d\n",
                      myTink.tinkDevice,
                      (myTink.powerState == TINK_POWER_ON) ? "ON" : "OFF",
                      (myTink.activeUartPort == TINK_UART_PORT_TYPE_USBC) ? "USB" : "HD15",
                      myTink.activeInputPort,
                      myTink.activeOutputRes);
  }
#endif

  /////////////////////////////
  //send SVS keepalive message
  if (millis() - lastTinkKeepAliveTime > TINK_SVS_KEEPALIVE_INTERVAL) 
  {
#ifdef TINK_DBG
    debugPort->println("[TINK] Current SVS Input: " + ((svsInputToKeepAlive != 0) ? String(svsInputToKeepAlive) : "No Input!") + " " + throbber[throbberIndex] + " Switch operation mode: " + String(pico_getGswMode() ? "AUTO" : "MANUAL"));
#endif

    //Send SVS keepalive
    if (svsInputToKeepAlive > NO_INPUT) {
#ifdef TINK_DBG
      debugPort->println("[TINK] Keep Alive SVS Input: " + String(svsInputToKeepAlive) + " " + throbber[throbberIndex]);
#endif
      String svsMsg = String(TINK_UART_COMMAND_KEEPALIVE) + String(svsInputToKeepAlive);

      //Send keepalive message to RT4K
      sendMessageToTink(svsMsg);
    }

#ifdef TINK_DBG
    throbberIndex++;
    throbberIndex %= 4;
#endif

    lastTinkKeepAliveTime = millis();
  }
  //end of send tink keepalive message
}

////////////////////////////////
// USB-C Notifications
////////////////////////////////

void tink_notifyUsbcConnected() {
  //myTink.activeUartPort = TINK_UART_PORT_TYPE_USBC;
  myTink.powerState = TINK_POWER_UNKNOWN;
  pico_notifyControlInterfaceUsbc();
}

void tink_notifyUsbcDisonnected() {
  myTink.activeUartPort = TINK_UART_PORT_TYPE_HD15;
  myTink.powerState = TINK_POWER_UNKNOWN;
  pico_notifyControlInterfaceHd15();
}

bool tink_attemptingPowerAction() {
  return tinkPowerStateTransition;
}

static void tinkStateDigest() {
  // Port state switching
  if (true == myTink.u1.portPresent)
  {
    //myTink.activeUartPort = TINK_UART_PORT_TYPE_USBC;
  } else {
    myTink.activeUartPort = TINK_UART_PORT_TYPE_HD15;
  }

  // Power routine
  // Update timeout causing state change
  if (TINK_UART_PORT_TYPE_USBC == myTink.activeUartPort)
  {
    //Update timeout check
    if (millis() - myTink.u1.lastMessageReceiveTime > TINK_UART_TIMEOUT)
    {
      if (millis() - tinkPowerOnAttemptTime > ((RT4K_PRO_BOOT_TIME>RT4K_CE_BOOT_TIME)?RT4K_PRO_BOOT_TIME:RT4K_CE_BOOT_TIME))
      {// U1 present + no update + boot time over = power off, transition time is over
        myTink.powerState = TINK_POWER_OFF;
        tinkPowerStateTransition = false;
      } else {
        // U1 present + no update + within boot time = power unknown, don't change transition state
        myTink.powerState = TINK_POWER_UNKNOWN;
      }
    } else {
      // U1 present + update = power is on
      myTink.powerState = TINK_POWER_ON;
      tinkPowerStateTransition = false;
    }
  } else {
    if (millis() - myTink.u5.lastMessageReceiveTime > TINK_UART_TIMEOUT)
    {
      // No U1 + no U5 update = Power unknown, Tink device present unknown
      myTink.powerState = TINK_POWER_UNKNOWN;

      // No U1 + no update + boot time over
      if (millis() - tinkPowerOnAttemptTime > ((RT4K_PRO_BOOT_TIME>RT4K_CE_BOOT_TIME)?RT4K_PRO_BOOT_TIME:RT4K_CE_BOOT_TIME))
      {
        // turn off power transition
        tinkPowerStateTransition = false;
      }      
    } else {
      // No U1 + U5 update
      myTink.powerState = TINK_POWER_ON;
      tinkPowerStateTransition = false;
    }
  }
}


////////////////////////////////
// RT4K command macros
////////////////////////////////
void tink_cmdPowerToggle() {
  if (TINK_POWER_ON == myTink.powerState) {
    sendMessageToTink(String(TINK_UART_REMOTE_COMMAND_PREFIX) + String(TINK_IR_REMOTE_BUTTON_PWR));
  } else {
    tink_remoteWakeFromSleep();
    tinkPowerStateTransition = true;
    tinkPowerOnAttemptTime = millis();
  }
}

void tink_cmdRes4k() {
  if (RT4K_Pro == myTink.tinkDevice ||
      RT4K_CE  == myTink.tinkDevice )
  {
    tink_outputResChange(TINK_OUTPUT_STOCK_RES_2160P60);
  } else {
    tink_outputResChange(TINK_OUTPUT_STOCK_RES_1440P60);
  }
}

void tink_cmdRes1080p() {
  tink_outputResChange(TINK_OUTPUT_STOCK_RES_1080P60);
}

void tink_cmdInputHD15() {
  tink_inputChange(TINK_INPUT_HD15_RGBS);
}

void tink_cmdInputSCART() {
  tink_inputChange(TINK_INPUT_SCART_RGBS);
}

void tink_remoteButtonSingle(String buttonString) {
  String remoteCommand = String(TINK_UART_REMOTE_COMMAND_PREFIX) + buttonString;

  sendMessageToTink(remoteCommand);
}

void tink_remoteWakeFromSleep() {
  //sendMessageToTink(TINK_UART_COMMAND_POWER_ON_FROM_SLEEP);
  if (false == myTink.u1.portPresent)
  {
    if(sendMessageToSerialUart(String(TINK_UART_COMMAND_POWER_ON_FROM_SLEEP), myTink.u5Uart))
    {
      myTink.u5.lastMessageSendTime = millis();
    }
  } else {
    if (sendMessageToUsbFtdi(String(TINK_UART_COMMAND_POWER_ON_FROM_SLEEP), myTink.u1Uart))
    {
      myTink.u1.lastMessageSendTime = millis();
    }
  }
}

void cycleQuickTabs(int increment) {
  //static int currentTabNum = 0;

  currentTabNum += increment;

  if (currentTabNum > NUM_QUICK_TABS - 1) {
    currentTabNum = 0;
  }

  if (currentTabNum < 0) {
    currentTabNum = 5;
  }

  tink_remoteButtonSingle(quickTabPages[currentTabNum]);
}

void tink_cycleQuickTabsIncrease() {
  cycleQuickTabs(1);
}

void tink_cycleQuickTabsDecrease() {
  cycleQuickTabs(-1);
}

unsigned int tink_getPowerState() {
  return myTink.powerState;
}

unsigned int tink_getConnection() {
  return myTink.activeUartPort;
}

// tink I/O Change functions
void tink_ioChange(String ioCommandPrefix, int ioNumber) {
  String ioChangeCommand = ioCommandPrefix + String(ioNumber);
  sendMessageToTink(ioChangeCommand);
}

bool tink_inputChange(int inputNumber) {
  if (inputNumber < 0 || inputNumber > TINK_INPUT_END - 1 || 
      inputNumber == TINK_INPUT_SPACER_1 || inputNumber == TINK_INPUT_SPACER_2 || inputNumber == TINK_INPUT_SPACER_3 || inputNumber == TINK_INPUT_SPACER_4 || 
      inputNumber == TINK_INPUT_FRONT_HEADER || inputNumber == TINK_INPUT_RCA_HEADER || inputNumber == TINK_INPUT_SCART_HEADER || inputNumber == TINK_INPUT_HD15_HEADER) {
    return false;
  }

  tink_ioChange(TINK_UART_INPUT_SELECT_COMMAND_PREFIX, inputNumber);
  return true;
}

bool tink_outputResChange(int outputResNumber) {
  if (outputResNumber < 0 || (outputResNumber > TINK_OUTPUT_STOCK_END - 1 && outputResNumber < TINK_OUTPUT_CUSTOM_RES_1) || outputResNumber > TINK_OUTPUT_CUSTOM_RES_END - 1) {
    return false;
  }

  tink_ioChange(TINK_UART_OUTPUT_SELECT_COMMAND_PREFIX, outputResNumber);
  return true;
}


////////////////////////
// From USB Host
////////////////////////

void usbHost_init() {
  
  // forcefully reset the USB controller
  reset_block(RESETS_RESET_USBCTRL_BITS);
  unreset_block_wait(RESETS_RESET_USBCTRL_BITS);
  
  usb_stopUsbHost();

  sleep_ms(100); 

  // Init USB Host on native controller roothub port0
  tusb_init(USB_NATIVE_HOST_ROOT_PORT, &dev_init);
  USBHost.begin(USB_NATIVE_HOST_ROOT_PORT);
}

void routine_usbHost() {
  USBHost.task();

  //set USB activity inactive after timeout
  if (millis() - lastTinkActivityTime > 500) {
    tinkActivity = false;
  }
}

bool sendMessageToUsbFtdi(String newMessage, Adafruit_USBH_CDC* usbCdc) {
  if (TINK_UART_PORT_TYPE_USBC == myTink.activeUartPort)
  {
    if ( true == myTink.u1.portPresent && usbCdc->availableForWrite() )
    {
      usbCdc->println(newMessage);
      usbCdc->flush();
      return true;
    }
  }
  return false;
}

bool sendMessageToUsbFtdiNoBlock(String newMessage, Adafruit_USBH_CDC* usbCdc) {
  if ( true == myTink.u1.portPresent && usbCdc->availableForWrite() )
  {
    usbCdc->println(newMessage);
    usbCdc->flush();
    return true;
  }
  return false;
}

bool tink_isThereActivity() {
  return tinkActivity;
}

void usb_stopUsbHost() {
  tuh_rhport_reset_bus(USB_NATIVE_HOST_ROOT_PORT, tuh_rhport_is_active(USB_NATIVE_HOST_ROOT_PORT));
  delay(50);
  tuh_deinit(USB_NATIVE_HOST_ROOT_PORT);
}

void usb_restartUsbHost() {
  delay(100);
  //tuh_init(USB_NATIVE_HOST_ROOT_PORT);
  tusb_init(USB_NATIVE_HOST_ROOT_PORT, &dev_init);
}

static bool isCdcDeviceRt4k(uint8_t idx) {
  tuh_itf_info_t info;
  uint16_t vid, pid;

  tuh_cdc_itf_get_info(idx, &info);
  tuh_vid_pid_get(info.daddr, &vid, &pid);

  if (TINK_USB_VID == vid && TINK_USB_PID == pid) {
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

static void tinkActivitySet() {
  tinkActivity = true;
  lastTinkActivityTime = millis();
}

bool sendMessageToSerialUart(String msg, SerialUART * uartPort)
{
  if (uartPort->availableForWrite())
  {
    uartPort->println(msg);
    uartPort->flush();
    return true;
  }
  return false;
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+1
void tuh_cdc_mount_cb(uint8_t idx) {
#ifdef CDC_DBG
  tuh_itf_info_t info;
  tuh_cdc_itf_get_info(idx, &info);
  debugPort->println("[CDC] Device ID: " + String(idx) + ", Addr: " + String(info.daddr, HEX));

  uint16_t vid, pid;
  tuh_vid_pid_get(info.daddr, &vid, &pid);
  debugPort->println("[CDC] Device Vid: " + String(vid, HEX) + ", Pid: " + String(pid, HEX));
#endif

  // bind cdcUart object to this interface index
  if (isCdcDeviceRt4k(idx)) {
    cdcUart.mount(idx);
    cdcUart.begin(TINK_USBC_UART_BAUD_RATE_DEFAULT);
    tink_notifyUsbcConnected();

    tinkActivitySet();

    myTink.u1.portPresent = true;
    //myTink.activeUartPort = TINK_UART_PORT_TYPE_USBC;
    u1lowerBaudRateSet = u1BaudRateChangeStep1 = u1BaudRateChangeStep2 = false;

#ifdef CDC_DBG
    debugPort->println("[CDC] Mounted CDC device to cdcUart");
#endif
  }
}

// Invoked when a device with CDC interface is unmounted
void tuh_cdc_umount_cb(uint8_t idx) {
  if (isCdcDeviceRt4k(idx)) {
    tink_notifyUsbcDisonnected();
    //cdcUart.end();
    cdcUart.umount(idx);

    tinkActivitySet();

    myTink.u1.portPresent = false;
    myTink.activeUartPort = TINK_UART_PORT_TYPE_HD15;
    u1lowerBaudRateSet = u1BaudRateChangeStep1 = u1BaudRateChangeStep2 = false;

#ifdef CDC_DBG
    debugPort->println("[CDC] cdcUart is unmounted");
#endif
  }
}

int tink_getCurrentDevice()
{
  return myTink.tinkDevice;
}

int tink_getCurrentInputPort() 
{
  return myTink.activeInputPort;
}

int tink_getCurrentOutputRes() 
{
  return myTink.activeOutputRes;
}