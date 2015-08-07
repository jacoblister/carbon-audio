#include "stdio.h"
//#include "stdafx.h"
#include "windows.h"
#include "interface/kxapi.h"

extern "C" {
    
#include "kxapiwrapper.h"

void *kxapi_open(void) {
   iKX *kx_interface = new iKX();
    
   kx_interface->init();

   return kx_interface;    
};

int kxapi_connect_microcode(void *kx_interface, int pgm1, int src, int pgm2, int dst) {
   iKX *kxi = (iKX *)kx_interface;
    
   return kxi->connect_microcode(pgm1, src, pgm2, dst);
    return 0;
}

int kxapi_disconnect_microcode(void *kx_interface, int pgm1, int src) {
   iKX *kxi = (iKX *)kx_interface;
    
   return kxi->disconnect_microcode(pgm1, src);
}

int kxapi_get_efx_register(void *kx_interface, int pgm, int id, long *val) {
   iKX *kxi = (iKX *)kx_interface;
   word id_word = id;
   dword *val_dword = (dword *)val;
    
   return kxi->get_efx_register(pgm, id_word, val_dword);
}

int kxapi_close(void *kx_interface) {
   iKX *kxi = (iKX *)kx_interface;
   
   delete kxi;    

   return 0;
}

}