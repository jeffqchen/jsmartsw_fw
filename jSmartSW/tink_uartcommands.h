#ifndef TINK_UARTCOMMANDS_H
#define TINK_UARTCOMMANDS_H

// RetroTINK 4K UART commands
#define TINK_UART_COMMAND_ECHO_STATUS "status"
#define TINK_UART_COMMAND_ECHO_MODEL  "model"
#define TINK_UART_COMMAND_ECHO_INPUT  "input"
#define TINK_UART_COMMAND_ECHO_OUTPUT "output"

////SVS commands
//#define TINK_UART_SVS_COMMAND_PREFIX "SVS "
#define TINK_UART_COMMAND_NEW_INPUT "SVS NEW INPUT="
#define TINK_UART_COMMAND_KEEPALIVE "SVS INPUT="

//Command components
#define TINK_UART_REMOTE_COMMAND_PREFIX  "remote "
#define TINK_UART_INPUT_SELECT_COMMAND_PREFIX   "input  "
#define TINK_UART_OUTPUT_SELECT_COMMAND_PREFIX  "output "

#define TINK_UART_BAUD_RATE_COMMAND_PREFIX  "baud "
#define TINK_CDC_BAUD_CHANGE_CONFIRM "baud ok"
#define TINK_CDC_BAUD_CHANGE_TIMEOUT  5000 //ms

//// RetroTINK 4K remote buttons UART commands
#define TINK_IR_REMOTE_BUTTON_PWR "pwr"
#define TINK_IR_REMOTE_MENU       "menu"
#define TINK_IR_REMOTE_UP         "up"
#define TINK_IR_REMOTE_DOWN       "down"
#define TINK_IR_REMOTE_LEFT       "left"
#define TINK_IR_REMOTE_RIGHT      "right"
#define TINK_IR_REMOTE_OK         "ok"
#define TINK_IR_REMOTE_BACK       "back"
#define TINK_IR_REMOTE_DIAG       "diag"
#define TINK_IR_REMOTE_STAT       "stat"
#define TINK_IR_REMOTE_INPUT      "input"
#define TINK_IR_REMOTE_OUTPUT     "output"
#define TINK_IR_REMOTE_SCALER     "scaler"
#define TINK_IR_REMOTE_SFX        "sfx"
#define TINK_IR_REMOTE_ADC        "adc"
#define TINK_IR_REMOTE_PROF       "prof"
#define TINK_IR_REMOTE_1          "prof1"
#define TINK_IR_REMOTE_2          "prof2"
#define TINK_IR_REMOTE_3          "prof3"
#define TINK_IR_REMOTE_4          "prof4"
#define TINK_IR_REMOTE_5          "prof5"
#define TINK_IR_REMOTE_6          "prof6"
#define TINK_IR_REMOTE_7          "prof7"
#define TINK_IR_REMOTE_8          "prof8"
#define TINK_IR_REMOTE_9          "prof9"
#define TINK_IR_REMOTE_10         "prof10"
#define TINK_IR_REMOTE_11         "prof11"
#define TINK_IR_REMOTE_12         "prof12"
#define TINK_IR_REMOTE_GAIN       "gain"
#define TINK_IR_REMOTE_PHASE      "phase"
#define TINK_IR_REMOTE_PAUSE      "pause"
#define TINK_IR_REMOTE_SAFE       "safe"
#define TINK_IR_REMOTE_GENLOCK    "genlock"
#define TINK_IR_REMOTE_BUFFER     "buffer"
#define TINK_IR_REMOTE_4K         "res4k"
#define TINK_IR_REMOTE_1080P      "res1080p"
#define TINK_IR_REMOTE_1440P      "res1440p"
#define TINK_IR_REMOTE_480P       "res480p"
#define TINK_IR_REMOTE_RES1       "res1"
#define TINK_IR_REMOTE_RES2       "res2"
#define TINK_IR_REMOTE_RES3       "res3"
#define TINK_IR_REMOTE_RES4       "res4"
#define TINK_IR_REMOTE_AUX1       "aux1"
#define TINK_IR_REMOTE_AUX2       "aux2"
#define TINK_IR_REMOTE_AUX3       "aux3"
#define TINK_IR_REMOTE_AUX4       "aux4"
#define TINK_IR_REMOTE_AUX5       "aux5"
#define TINK_IR_REMOTE_AXU6       "axu6"
#define TINK_IR_REMOTE_AUX7       "aux7"
#define TINK_IR_REMOTE_AUX8       "aux8"
#define TINK_IR_REMOTE_COL        "col"
#define TINK_IR_REMOTE_AUD        "aud"

// Special power on from sleep command
#define TINK_UART_COMMAND_POWER_ON_FROM_SLEEP  "pwr on"

// Info echo commands
#define TINK_UART_COMMAND_ECHO_VERSION  "ver"

/////////////////////////////////
// COM Command Replies
/////////////////////////////////

#define TINK_MODEL_MSG_LEAD  "[COM] model="
#define TINK_INPUT_MSG_LEAD  "[COM] input="
#define TINK_OUTPUT_MSG_LEAD "[COM] output sel="

#define TINK_BAUD_CHANGE_REPLY_1 "[COM] baud switching to 115200"
#define TINK_BAUD_CHANGE_CONFIRM_MSG_LEAD "[COM] baud confirmed 115200"

#define TINK_STATUS_MSG_FORMATTED  "[COM] status fw=%*s tag=%*s model=%d ver_int=%*d uptime_s=%*lu baud=%*lu\n\
                                    [COM] status input_source=%d input_ic=%*d output_res=%d sd=%*d xfer=%*d\n\
                                    [COM] status loop_min_us=%*lu loop_avg_us=%*lu loop_max_us=%*lu loop_n=%*lu\n\
                                    [COM] status oerr=%*lu ring_drop=%*lu queue_drop=%*lu linedrop=%*lu u5oerr=%*lu\n\
                                    [COM] status profile=%*s"
#endif