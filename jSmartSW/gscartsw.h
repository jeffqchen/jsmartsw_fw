#include "mydebug.h"

#include "gsw_hardware.h"

#ifndef GSCARTSW_H
#define GSCARTSW_H

#endif

//external functions
void routine_gswAutoInputUpdate();

////mode related
void gsw_setModeAuto();
void gsw_setModeManual();

////input related
void gsw_writeInput (unsigned int);
unsigned int gsw_queryInput();