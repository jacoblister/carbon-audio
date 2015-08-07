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

class CDeviceMIDIOSS : CDeviceMIDI {
 private:
   int handle;
   byte status;
   byte expected;
   byte count;
   byte read_buf[4];
 public:
   ALIAS<"deviceMIDIOSS">;    
   void CDeviceMIDIOSS(void);    
    
   virtual EDeviceError open(void);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode); 
};

EDeviceError CDeviceMIDIOSS::open(TFrameInfo *info) {
   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.input:
      this->handle = open(CString(&CDeviceHardware(this)->device).string(), O_RDONLY | O_NONBLOCK | O_NOCTTY, 0);
      break;
   case EStreamMode.output:
      this->handle = open(CString(&CDeviceHardware(this)->device).string(), O_WRONLY | O_NONBLOCK | O_NOCTTY, 0);
      break;
   default:
      this->handle = -1;
   }

   if (this->handle == -1) {
      return EDeviceError.deviceError;
   }
   
   return EDeviceError.noError;
}/*CDeviceMIDIOSS::open*/

EDeviceError CDeviceMIDIOSS::close(void) {
   close(this->handle);
    
   return EDeviceError.noError;    
}/*CDeviceMIDIOSS::close*/

bool CDeviceMIDIOSS::frame(CFrame *frame, EStreamMode mode) {
   byte b;
   TMidiEventContainer event;
   int i;
   ARRAY<byte> outdata;
   
   if (mode == EStreamMode.input && CDeviceHardware(this)->mode == EStreamMode.input) {
      while (read(this->handle, &b, 1) != -1) {
         if (b & 0x80) {                   /* if status byte */
         this->status = b;
             /* New event, get expected data length */
            switch (MIDI_TYPE(b)) {
            case ME_Note:
            case ME_NoteOff:
            case ME_Control:
            case ME_PitchWheel:
               this->expected = 2;
               break;
            case ME_Program:
            case ME_Presure:
               this->expected = 1;
               break;
            default:
               this->expected = 0;
            }
	    this->count = 0;
         }
         else {
	    this->read_buf[this->count] = b;
            this->count++;
            if (this->count == this->expected) {
//	       printf("event %d, %d, %d\n", this->status, this->read_buf[0], this->read_buf[1]);
	       CLEAR(&event);
//               TMidiEventContainer_new(&event, this->status, this->read_buf[0], this->read_buf[1], 0);
               ARRAY(&frame->midi).item_add(event);
	    
               this->count = 0;
            }
         }
      }/*while*/
   }
   if (mode == EStreamMode.output && CDeviceHardware(this)->mode == EStreamMode.output) {   
      ARRAY(&outdata).new();
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         ARRAY(&outdata).used_set(0);
         TMidiEventContainer_encode_append(&ARRAY(&frame->midi).data()[i], &outdata, NULL);
         write(this->handle, ARRAY(&outdata).data(), ARRAY(&outdata).count());
      }
      
      ARRAY(&outdata).delete();
   }
   
   return TRUE;
}/*CDeviceMIDIOSS::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
