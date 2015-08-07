/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

//short _stdcall Inp32(short PortAddress);
//void _stdcall Out32(short PortAddress, short data);
extern short inpout32_in(short PortAddress);

class CDeviceMIDIParPort : CDeviceMIDI {
 public:
   ALIAS<"deviceMIDIParPort">;    
   void CDeviceMIDIWParPort(void);    
    
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode); 
};

EDeviceError CDeviceMIDIParPort::open(TFrameInfo *info) {
   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.input:
      break;
   case EStreamMode.output:
      break;
   }
    
   return EDeviceError.noError;
}/*CDeviceMIDIParPort::open*/

EDeviceError CDeviceMIDIParPort::close(void) {
   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.input:       
      break;
   case EStreamMode.output:
      break;
   }
    
   return EDeviceError.noError;    
}/*CDeviceMIDIParPort::close*/

bool CDeviceMIDIParPort::frame(CFrame *frame, EStreamMode mode) {
   class:base.frame(frame, mode);

   if (mode == EStreamMode.input && CDeviceHardware(this)->mode == EStreamMode.input) {
      printf("parport input frame %d\n", inpout32_in(0x379));
   }
   
   return TRUE;
}/*CDeviceMIDIParPort::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
