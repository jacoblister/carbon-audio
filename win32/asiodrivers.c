#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"

extern "C"
{

extern AsioDrivers* asioDrivers;    
    
void removeCurrentAsioDriver(void) {
    asioDrivers->removeCurrentDriver();
}    
    
#include "asiodrivers.cpp"

}

