/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "../devices.c"
#include "../xmidi.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

class CDeviceSerial : CDeviceMIDI {
 private:
   int status;
   int handle;
   int op;
 public:
   ALIAS<"deviceSerial">;    
   void CDeviceSerial(void);    
    
   virtual EDeviceError open(void);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode); 
};

EDeviceError CDeviceSerial::open(TFrameInfo *info) {
    this->handle = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (this->handle == -1) {
        return EDeviceError.deviceError;
    }

    ioctl(this->handle, TIOCMGET, &this->status);

    return EDeviceError.noError;
}/*CDeviceSerial::open*/

EDeviceError CDeviceSerial::close(void) {
   close(this->handle);
    
   return EDeviceError.noError;    
}/*CDeviceSerial::close*/

bool CDeviceSerial::frame(CFrame *frame, EStreamMode mode) {
    int i;
    if (mode == EStreamMode.output) {
        for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
            if (CObject(&ARRAY(&frame->midi).data()[i]).obj_class() == &class(CMETimingClock)) {
               this->op = 50;
            }
        }
    }
    switch (this->op) {
    case 0:
       break;
    case 1:
       this->status &= ~TIOCM_RTS;
       ioctl(this->handle, TIOCMSET, &this->status);
       goto beat;
    default:
       if ((this->status | TIOCM_RTS) != this->status) {
          this->status |= TIOCM_RTS;
          ioctl(this->handle, TIOCMSET, &this->status);
       }
       goto beat;
    beat:
       this->op--;
       break;
    }
   
    return TRUE;
}/*CDeviceSerial::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/