#ifndef RT4K_UARTCOMMANDS_H
#define RT4K_UARTCOMMANDS_H

// RetroTINK 4K UART commands
////SVS commands
#define RT4K_UART_COMMAND_NEW_INPUT "SVS NEW INPUT="
#define RT4K_UART_COMMAND_KEEPALIVE "SVS CURRENT INPUT="

//Command components
#define RT4K_UART_REMOTE_COMMAND_PREFIX  "remote "

//// RetroTINK 4K remote buttons UART commands
#define RT4K_IR_REMOTE_BUTTON_PWR "pwr"

#define RT4K_IR_REMOTE_INPUT      "input"
#define RT4K_IR_REMOTE_OUTPUT     "output"
#define RT4K_IR_REMOTE_SCALER     "scaler"
#define RT4K_IR_REMOTE_SFX        "sfx"
#define RT4K_IR_REMOTE_ADC        "adc"
#define RT4K_IR_REMOTE_PROF       "prof"

#define RT4K_IR_REMOTE_1          "prof1"
#define RT4K_IR_REMOTE_2          "prof2"
#define RT4K_IR_REMOTE_3          "prof3"
#define RT4K_IR_REMOTE_4          "prof4"
#define RT4K_IR_REMOTE_5          "prof5"
#define RT4K_IR_REMOTE_6          "prof6"
#define RT4K_IR_REMOTE_7          "prof7"
#define RT4K_IR_REMOTE_8          "prof8"
#define RT4K_IR_REMOTE_9          "prof9"
#define RT4K_IR_REMOTE_10         "prof10"
#define RT4K_IR_REMOTE_11         "prof11"
#define RT4K_IR_REMOTE_12         "prof12"

#define RT4K_IR_REMOTE_MENU       "menu"
#define RT4K_IR_REMOTE_BACK       "back"

#define RT4K_IR_REMOTE_UP         "up"
#define RT4K_IR_REMOTE_DOWN       "down"
#define RT4K_IR_REMOTE_LEFT       "left"
#define RT4K_IR_REMOTE_RIGHT      "right"
#define RT4K_IR_REMOTE_OK         "ok"

#define RT4K_IR_REMOTE_DIAG       "diag"
#define RT4K_IR_REMOTE_STAT       "stat"

#define RT4K_IR_REMOTE_GAIN       "gain"
#define RT4K_IR_REMOTE_PHASE      "phase"

#define RT4K_IR_REMOTE_PAUSE      "pause"
#define RT4K_IR_REMOTE_SAFE       "safe"

#define RT4K_IR_REMOTE_GENLOCK    "genlock"
#define RT4K_IR_REMOTE_BUFFER     "buffer"

#define RT4K_IR_REMOTE_4K         "res4k"
#define RT4K_IR_REMOTE_1080P      "res1080p"
#define RT4K_IR_REMOTE_1440P      "res1440p"
#define RT4K_IR_REMOTE_480P       "res480p"
#define RT4K_IR_REMOTE_RES1       "res1"
#define RT4K_IR_REMOTE_RES2       "res2"
#define RT4K_IR_REMOTE_RES3       "res3"
#define RT4K_IR_REMOTE_RES4       "res4"

#define RT4K_IR_REMOTE_AUX1       "aux1"
#define RT4K_IR_REMOTE_AUX2       "aux2"
#define RT4K_IR_REMOTE_AUX3       "aux3"
#define RT4K_IR_REMOTE_AUX4       "aux4"
#define RT4K_IR_REMOTE_AUX5       "aux5"
#define RT4K_IR_REMOTE_AXU6       "axu6"
#define RT4K_IR_REMOTE_AUX7       "aux7"
#define RT4K_IR_REMOTE_AUX8       "aux8"

////Remote commands
#define RT4K_UART_COMMAND_POWER_ON_FROM_SLEEP  "pwr on"
#define RT4K_UART_COMMAND_ECHO_VERSION  "ver"

#endif