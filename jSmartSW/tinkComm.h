#include "mydebug.h"

#include <Arduino.h>

#include "Adafruit_TinyUSB.h"

#include "tink_uartcommands.h"

#ifndef TINKCOMM_H
#define TINKCOMM_H

#define TINK_MONITOR_STATUS // Monitor RetroTINK device status for adjusting LED status

// Comment out to opt not to switch to a slower baud rate for U1 (USB-C) connection
//#define TINK_SWITCH_TO_SLOW_BAUD

#define RT4K_PRO_BOOT_TIME (unsigned long)8000
#define RT4K_CE_BOOT_TIME (unsigned long)5500

#define NUM_QUICK_TABS 6
#define QUICK_TABS_ARRAY  TINK_IR_REMOTE_INPUT,   \
                          TINK_IR_REMOTE_OUTPUT,  \
                          TINK_IR_REMOTE_ADC,     \
                          TINK_IR_REMOTE_SCALER,  \
                          TINK_IR_REMOTE_SFX,     \
                          TINK_IR_REMOTE_PROF

#define TINK_UPDATE_MSG_STRING_ARRAY  TINK_UART_COMMAND_ECHO_MODEL, \
                                      TINK_UART_COMMAND_ECHO_INPUT, \
                                      TINK_UART_COMMAND_ECHO_OUTPUT

enum {
  TINK_MSG_MODEL = 0,
  TINK_MSG_INPUT,
  TINK_MSG_OUTPUT,
  TINK_MSG_END
};

#define TINK_COMMAND_RATE_CONTROL_PERIOD  10 //ms

//////////////////////
// Serial
//////////////////////

#define PICO_U5_UART_PORT_HW uart0
#define PICO_U5_UART_PORT_SW Serial1
#define PICO_U5_HW_IRQ  UART0_IRQ
#define PICO_U5_TX_PIN  0 //GPIO
#define PICO_U5_RX_PIN  1 //GPIO
#define TINK_U5_UART_BAUD_RATE 9600
#define TINK_U5_UART_CONFIG SERIAL_8N1

#define TINK_UART_MESSAGE_WINDOW 1 //ms

#define TINK_STATUS_UPDATE_INTERVAL (unsigned long)777  //ms
#define TINK_SVS_KEEPALIVE_INTERVAL (unsigned long)1000   //ms
#define TINK_UART_TIMEOUT 2000 //ms

//////////////////////
// USB
//////////////////////
#define TINK_USBC_UART_BAUD_RATE_DEFAULT  2000000
#define TINK_USBC_UART_BAUD_RATE_LOW      115200

#define TINK_UART_MSG_POWER_ON  "FPGA"
#define TINK_UART_MSG_POWER_OFF "Jumping to Application"

#define TINK_USB_VID  0x0403
#define TINK_USB_PID  0x6001

#define USB_NATIVE_HOST_ROOT_PORT 0

// Circular Buffer Definition
#define CIRCULAR_BUFFER_SIZE  512 // Must be a power of 2 (e.g., 32, 64, 128, 256)
#define CIRCULAR_BUFFER_MASK  (CIRCULAR_BUFFER_SIZE - 1)

typedef struct {
    uint8_t buffer[CIRCULAR_BUFFER_SIZE];
    volatile uint16_t head; // Written by ISR/Producer
    volatile uint16_t tail; // Read by Main Loop/Consumer
    volatile bool pushLock;
} SerialBuffer;

// Tink device type
enum {
  RT4K_Pro = 0,
  RT4K_CE,
  RT6X_Pro,
  RT6X_CE,
  RT_DEVICE_END,
  RT_DEVICE_UNKOWN = 255
};

// RT4K power states
enum{
  TINK_POWER_OFF = 0,
  TINK_POWER_ON,  
  TINK_POWER_UNKNOWN,
};

// RT4K UART connection types
enum{
  TINK_UART_PORT_TYPE_HD15  = 0,
  TINK_UART_PORT_TYPE_USBC,
  TINK_UART_PORT_TYPE_END
};


// TINK Input Emuneration
enum{
  TINK_INPUT_HDMI = 0,
  TINK_INPUT_SPACER_1,
  TINK_INPUT_FRONT_HEADER,
  TINK_INPUT_FRONT_CVBS,
  TINK_INPUT_FRONT_YC,
  TINK_INPUT_SPACER_2,
  TINK_INPUT_RCA_HEADER,
  TINK_INPUT_RCA_YUV,
  TINK_INPUT_RCA_RGSB,
  TINK_INPUT_RCA_CVBSONG,
  TINK_INPUT_SPACER_3,
  TINK_INPUT_SCART_HEADER,
  TINK_INPUT_SCART_RGBS,
  TINK_INPUT_SCART_RGSB,
  TINK_INPUT_SCART_YUV,
  TINK_INPUT_SCART_CVBS,
  TINK_INPUT_SCART_CVBSONG,
  TINK_INPUT_SCART_YC,
  TINK_INPUT_SPACER_4,
  TINK_INPUT_HD15_HEADER,
  TINK_INPUT_HD15_RGBHV,
  TINK_INPUT_HD15_RGBS,
  TINK_INPUT_HD15_RGSB,
  TINK_INPUT_HD15_YUV,
  TINK_INPUT_HD15_CVBSONH,
  TINK_INPUT_HD15_CVBSONG,
  TINK_INPUT_HD15_YCONGR,
  TINK_INPUT_HD15_ENHANCEDYC,
  TINK_INPUT_END
};

//TINK Stock Output Emuneration
enum{
  TINK_OUTPUT_STOCK_RES_2160P60 = 0,
  TINK_OUTPUT_STOCK_RES_2160P50,
  TINK_OUTPUT_STOCK_RES_1080P60,
  TINK_OUTPUT_STOCK_RES_1080P50,
  TINK_OUTPUT_STOCK_RES_1440P60,
  TINK_OUTPUT_STOCK_RES_1440P50,
  TINK_OUTPUT_STOCK_RES_1080P100,
  TINK_OUTPUT_STOCK_RES_1440P100,
  TINK_OUTPUT_STOCK_RES_1080P120,
  TINK_OUTPUT_STOCK_RES_1440P120,
  TINK_OUTPUT_STOCK_RES_1080P144,
  TINK_OUTPUT_STOCK_RES_1440P144,
  TINK_OUTPUT_STOCK_RES_720P,
  TINK_OUTPUT_STOCK_RES_480P,
  TINK_OUTPUT_STOCK_RES_240P,
  TINK_OUTPUT_STOCK_RES_240P120,
  TINK_OUTPUT_STOCK_END
};

enum{
  TINK_OUTPUT_CUSTOM_RES_1 = 69,
  TINK_OUTPUT_CUSTOM_RES_2,
  TINK_OUTPUT_CUSTOM_RES_3,
  TINK_OUTPUT_CUSTOM_RES_4,
  TINK_OUTPUT_CUSTOM_RES_5,
  TINK_OUTPUT_CUSTOM_RES_6,
  TINK_OUTPUT_CUSTOM_RES_END
};

/*
union tinkUartPort {
  Adafruit_USBH_CDC*  u1UartPort;
  SerialUART*         u5UartPort;
};
*/

typedef struct {
  bool portPresent;
  bool hasUpdate;
  unsigned long lastStatusUpdateSendTime;
  unsigned long lastMessageSendTime;
  unsigned long lastMessageReceiveTime;
  SerialBuffer* circularBuf;
  char* messageFrame;
} tinkUartStateEntry;

typedef struct {
  //device
  uint8_t tinkDevice;

  //power state
  uint8_t powerState;

  //current uart
  uint8_t activeUartPort;

  //current input
  uint8_t activeInputPort;

  //current output resolution
  uint8_t activeOutputRes;

  Adafruit_USBH_CDC* u1Uart;
  tinkUartStateEntry u1;

  SerialUART* u5Uart;
  tinkUartStateEntry u5;
} tinkStates;

#endif

// init
void tink_uartInit();
void tink_uartInterruptRegister();

// external function
void routine_tinkCommunication();   //Serial keepalive message to RT4K
void tink_svsInputChange(unsigned int);

void tink_cycleQuickTabsIncrease();
void tink_cycleQuickTabsDecrease();

void tink_notifyUsbcConnected();
void tink_notifyUsbcDisonnected();

void tink_setPowerState(unsigned int);
unsigned int tink_getPowerState();
bool tink_attemptingPowerAction();
unsigned int tink_getConnection();

int tink_getCurrentDevice();
int tink_getCurrentInputPort();
int tink_getCurrentOutputRes();

//tink serial macros
void tink_cmdPowerToggle();
void tink_cmdRes4k();
void tink_cmdRes1080p();
void tink_cmdInputHD15();
void tink_cmdInputSCART();
void tink_remoteButtonSingle(String);
void tink_remoteWakeFromSleep();

//tink I/O change
void tink_ioChange(String, String);
bool tink_inputChange(int);
bool tink_outputResChange(int);

//////////////////////
// USB
//////////////////////

// init
void usbHost_init();

// routine
void routine_usbHost();

// external func
bool tink_isThereActivity();

void usb_stopUsbHost();
void usb_restartUsbHost();