/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "..\devices.c"
#include "kxapiwrapper.h"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#include "..\mixer.c"
#include "kxreg.c"

class CDeviceMIDIKx : CDeviceMIDI {
 private:
   int active_channel;
   int active_set;
   void *kx_interface;
 public:
   ALIAS<"deviceMIDIKx">;
 
   ATTRIBUTE:ARRAY<TKXRegister trigger>;
   ATTRIBUTE:ARRAY<int drumSet0>; 
   ATTRIBUTE:ARRAY<int drumSet1>; 
   ATTRIBUTE:ARRAY<int drumNotes>;
   TBitfield state;

   void new(void);
   void CDeviceMIDIKx(void);
   void ~CDeviceMIDIKx(void); 
 
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);  
};

void CDeviceMIDIKx::new(void) {
   _virtual_CDeviceHardware_new(CDeviceHardware(this)); 
    
   ARRAY(&this->trigger).new();
   ARRAY(&this->drumSet0).new();
   ARRAY(&this->drumSet1).new();
   ARRAY(&this->drumNotes).new();
   BITFIELD(&this->state).count_set(4);
}/*CDeviceMIDIKx::new*/

void CDeviceMIDIKx::~CDeviceMIDIKx(void) {
   ARRAY(&this->drumNotes).delete();
   ARRAY(&this->drumSet1).delete();    
   ARRAY(&this->drumSet0).delete();        
   ARRAY(&this->trigger).delete();
    
   _virtual_CDeviceHardware___destroy(CDeviceHardware(this)); 
}/*CDeviceMIDIKx::~CDeviceMIDIKx*/

EDeviceError CDeviceMIDIKx::open(TFrameInfo *info) {
   int i;
    
   this->kx_interface = kxapi_open();
    
   return EDeviceError.noError;
}/*CDeviceMIDIKx::open*/

EDeviceError CDeviceMIDIKx::close(void) {
   kxapi_close(this->kx_interface); 
    
    return EDeviceError.noError;    
}/*CDeviceMIDIKx::close*/

bool CDeviceMIDIKx::frame(CFrame *frame, EStreamMode mode) {
   int i, j, patch;
   CMidiEvent *event;
   TMidiEventContainer *new_event;
   unsigned long trigger_0, trigger_1;
    
   if (mode == EStreamMode.output) {
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(event).obj_class() == &class(CMEProgramChange)) {
            CDeviceMIDI(this)->patch[event->channel - 1] = CMEProgramChange(event)->number;
         }
         if (CObject(event).obj_class() == &class(CMEControlChange)) {
            switch (CMEControlChange(event)->control) {
            case MIDI_CONTROL_ACTIVE_A:
               this->active_channel = CMEControlChange(event)->value;
               this->active_set = -1;
               patch = CDeviceMIDI(this)->patch[this->active_channel];
               for (j = 0; j < ARRAY(&this->drumSet0).count(); j++) {
                  if (patch == ARRAY(&this->drumSet0).data()[j]) {
                     this->active_set = 0;
                  }
               }
               for (j = 0; j < ARRAY(&this->drumSet1).count(); j++) {
                  if (patch == ARRAY(&this->drumSet1).data()[j]) {
                     this->active_set = 1;
                  }
               }
               
               if (this->active_set >= 0) {
                   /* hack>>> reset channel mapping for RPx Audio */
                   CMEControlChange(event)->value = 15;
               }
            }
         }
      }   
   }
   
   if (this->active_set < 0) {
       return TRUE;
   }
  
   if (mode == EStreamMode.input) {
       kxapi_get_efx_register(this->kx_interface, 
                              (ARRAY(&this->trigger).data()[0]).pgm, 
                              (ARRAY(&this->trigger).data()[0]).reg,
                             &trigger_0);
       kxapi_get_efx_register(this->kx_interface, 
                              (ARRAY(&this->trigger).data()[1]).pgm, 
                              (ARRAY(&this->trigger).data()[1]).reg,
                             &trigger_1);
       if (!BITFIELD(&this->state).get(0) && trigger_0) {
          ARRAY(&frame->midi).item_add_empty();
          new_event = ARRAY(&frame->midi).item_last();
          new(new_event).CMENote(this->active_channel + 1, 0, ARRAY(&this->drumNotes).data()[this->active_set * 2], 127);
       }
       BITFIELD(&this->state).set(0, trigger_0);
       
       if (!BITFIELD(&this->state).get(1) && trigger_1) {
          ARRAY(&frame->midi).item_add_empty();
          new_event = ARRAY(&frame->midi).item_last();
          new(new_event).CMENote(this->active_channel + 1, 0, ARRAY(&this->drumNotes).data()[this->active_set * 2 + 1], 127);
       }
       BITFIELD(&this->state).set(1, trigger_1);
   }
   
   return TRUE;
}/*CDeviceMidiKx::*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
