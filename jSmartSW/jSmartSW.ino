#include "mypico.h"

bool core1_separate_stack = true;

////////////////////////
/// SETUP //////////////
////////////////////////
void setup() 
{
  jsmartsw_core1Setup();
}

void setup1()
{
  jsmartsw_core2Setup();
}

////////////////////////
/// MAIN LOOPS  ////////
////////////////////////
void loop() 
{
  jsmartsw_core1Loop();
}

void loop1()
{ 
  jsmartsw_core2Loop();
}
