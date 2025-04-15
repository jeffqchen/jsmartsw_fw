#ifndef USB_HOST_H
#define USB_HOST_H

#define USB_TASK_INTERVAL 10 //ms

#define RT4K_UART_MSG_POWER_ON  "Normal Boot"
#define RT4K_UART_MSG_POWER_OFF "Jumping to Application"

#define RT4K_USB_VID  0x0403
#define RT4K_USB_PID  0x6001

#define USB_NATIVE_HOST_ROOT_PORT 0
#endif

// init
void usbHost_init();

// routine
void routine_usbHostFtdi();

// external func
bool usb_isThereActivity();
void usbHostFtdi_sendToUsbSerial(String);

void usb_stopUsbHost();
void usb_restartUsbHost();