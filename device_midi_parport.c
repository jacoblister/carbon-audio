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
extern short inpout32_enable(short PortAddress, short range, short enable);

class CDeviceMIDIParPort : CDeviceMIDI {
 private:
   void new(void);
   void delete(void);
   int frame_debounce;
   int debounce_count;
   byte last_data;
 public:
   ALIAS<"deviceMIDIParPort">;    
   ATTRIBUTE<int port>;
   ATTRIBUTE<TBitfield invert>;
   ATTRIBUTE<int channel>;
   ATTRIBUTE<ARRAY<int> note>;
   ATTRIBUTE<int debounce>;
 
   void CDeviceMIDIWParPort(void);    
    
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode); 
};

void CDeviceMIDIParPort::new(void) {
   ARRAY(&this->note).new();
	
   class:base.new();
}/*CDeviceMIDIParPort::new*/

void CDeviceMIDIParPort::delete(void) {
   ARRAY(&this->note).delete();
	
   class:base.delete();
}/*CDeviceMIDIParPort::delete*/	

EDeviceError CDeviceMIDIParPort::open(TFrameInfo *info) {
   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.input:
      this->frame_debounce = this->debounce / (info->frame_length * 1000 / info->sampling_rate);	   
      inpout32_enable((short)this->port, 2, 1);
      this->last_data = (byte)inpout32_in((short)this->port + 1);
      break;
   case EStreamMode.output:
      break;
   default:
      break;
   }
    
   return EDeviceError.noError;
}/*CDeviceMIDIParPort::open*/

EDeviceError CDeviceMIDIParPort::close(void) {
   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.input:       
      inpout32_enable((short)this->port, 2, 0);
      break;
   case EStreamMode.output:
      break;
   default:
      break;
   }
    
   return EDeviceError.noError;    
}/*CDeviceMIDIParPort::close*/

bool CDeviceMIDIParPort::frame(CFrame *frame, EStreamMode mode) {
   byte data;	
   int i, value;	
   TMidiEventContainer *new_event;
	
   class:base.frame(frame, mode);

   if (mode == EStreamMode.input && CDeviceHardware(this)->mode == EStreamMode.input) {
      if (!this->debounce_count) {
         data = (byte)inpout32_in((short)this->port + 1);
         for (i = 0; i < 8; i++) {
            if ((data & (1 << i)) != (this->last_data & (1 << i))) {
	       value = (((data & (1 << i)) != 0) ^ BITFIELD(&this->invert).get(i));
    
               ARRAY(&frame->midi).item_add_empty();
               new_event = ARRAY(&frame->midi).item_last();
	       if (value) {
                  new(new_event).CMENote(this->channel, 0, ARRAY(&this->note).data()[i], 127);
	       }
	       else {
                  new(new_event).CMENoteOff(this->channel, 0, ARRAY(&this->note).data()[i]);
	       }
		    
	       this->debounce_count = this->frame_debounce;
	    }
         } 
	 this->last_data = data;
      }
      else {
	 this->debounce_count--;
      }
   }
   
   return TRUE;
}/*CDeviceMIDIParPort::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
