#include <sys/_stdint.h>
#include "mydebug.h"

#include <Arduino.h>

#include "rt4k_uartcommands.h"

#ifndef RT4K_H
#define RT4K_H

#define RT4K_PRO_BOOT_TIME (unsigned long)8000
#define RT4K_CE_BOOT_TIME (unsigned long)5500

#define RT4K_HD15_UART_BAUD_RATE 9600
#define RT4K_HD15_UART_CONFIG SERIAL_8N1

#define RT4K_USBC_UART_BAUD_RATE 115200
#define TINK_KEEPALIVE_INTERVAL (unsigned long)1000

#define NUM_QUICK_TABS 6

// RT4K power states
enum{
  RT4K_OFF = 0,
  RT4K_ON,  
  RT4K_UNKNOWN,
};

// RT4K UART connection types
enum{
  RT4K_UART_PORT_TYPE_HD15  = 0,
  RT4K_UART_PORT_TYPE_USBC,
  RT4K_UART_PORT_TYPE_END
};

#endif

// init
void rt4k_uartInit();

// external function
void routine_rt4kCommunication();   //Serial keepalive message to RT4K
void rt4k_inputChange(unsigned int);

void rt4k_cycleQuickTabsIncrease();
void rt4k_cycleQuickTabsDecrease();

void rt4k_notifyUsbcConnected();
void rt4k_notifyUsbcDisonnected();

void rt4k_setPowerState(unsigned int);
unsigned int rt4k_getPowerState();
bool rt4k_attemptingPowerAction();
unsigned int rt4k_getConnection();

//rt4k serial macros
void rt4k_cmdPowerToggle();
void rt4k_cmdRes4k();
void rt4k_cmdRes1080p();
void rt4k_cmdInputHD15();
void rt4k_cmdInputSCART();
void rt4k_remoteButtonSingle(String);
void rt4k_remoteWakeFromSleep();