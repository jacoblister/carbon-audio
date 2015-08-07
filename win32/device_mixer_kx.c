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

typedef struct {
   int pgm;
   int reg;
} TKXRegister;

ATTRIBUTE:typedef<TKXRegister>;
ARRAY:typedef<TKXRegister>;

bool ATTRIBUTE:convert<TKXRegister>(struct tag_CObjPersistent *object,
                                    const TAttributeType *dest_type, const TAttributeType *src_type,
                                    int dest_index, int src_index,
                                    void *dest, const void *src) {
   TKXRegister *value;
   CString *string;
   const char *delim;

   if (dest_type == &ATTRIBUTE:type<TKXRegister> && src_type == &ATTRIBUTE:type<CString>) {
      value  = (TKXRegister *)dest;
      string = (CString *)src;
       
      delim = CString(string).strchr(':');

      if (delim) {
         value->pgm = strtol(CString(string).string(), NULL, 10);
         value->reg = strtol((char *)delim + 1, NULL, 16);
         return TRUE;
      }
      return FALSE;
   }

   if (dest_type == &ATTRIBUTE:type<CString> && src_type == &ATTRIBUTE:type<TKXRegister>) {
      value  = (TKXRegister *)src;
      string = (CString *)dest;
      CString(string).printf("%d:%X", value->pgm, value->reg);
      return TRUE;
   }
   return FALSE;
}

class CDeviceMixerKx : CDeviceMixer {
 private:
   int active_channel;
   void *kx_interface;
   void new(void); 
   void delete(void);  
 public:
   ALIAS<"deviceMixerKx">;
 
   ATTRIBUTE<TKXRegister input>;
   ATTRIBUTE:ARRAY<TKXRegister dspOutput>;
   ATTRIBUTE:ARRAY<TKXRegister dspFx>; 

   void CDeviceMixerKx(void);
 
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);  
};

void CDeviceMixerKx::new(void) {
   _virtual_CDeviceHardware_new(CDeviceHardware(this)); 
    
   ARRAY(&this->dspOutput).new();
   ARRAY(&this->dspFx).new();    
}/*CDeviceMixerKx::new*/

void CDeviceMixerKx::delete(void) {
   ARRAY(&this->dspFx).delete();
   ARRAY(&this->dspOutput).delete();
    
   _virtual_CDeviceHardware___destroy(CDeviceHardware(this)); 
}/*CDeviceMixerKx::delete*/

EDeviceError CDeviceMixerKx::open(TFrameInfo *info) {
   int i;
    
   this->kx_interface = kxapi_open();
    
   for (i = 0; i < ARRAY(&this->dspFx).count(); i++) {
      kxapi_disconnect_microcode(this->kx_interface, 
                                 ARRAY(&this->dspFx).data()[i].pgm, ARRAY(&this->dspFx).data()[i].reg);
      if (i == this->active_channel) {
         kxapi_connect_microcode(this->kx_interface,
                                 this->input.pgm, this->input.reg,
                                 ARRAY(&this->dspFx).data()[i].pgm, ARRAY(&this->dspFx).data()[i].reg
                                 );
      }
      else {      
         kxapi_connect_microcode(this->kx_interface,
                                 ARRAY(&this->dspOutput).data()[i].pgm, ARRAY(&this->dspOutput).data()[i].reg,
                                 ARRAY(&this->dspFx).data()[i].pgm, ARRAY(&this->dspFx).data()[i].reg
                                 );
      }
   }
   return EDeviceError.noError;
}/*CDeviceMixerKx::open*/

EDeviceError CDeviceMixerKx::close(void) {
   kxapi_close(this->kx_interface); 
    
    return EDeviceError.noError;    
}/*CDeviceMixerKx::close*/

bool CDeviceMixerKx::frame(CFrame *frame, EStreamMode mode) {
   int i;
   CMidiEvent *event;
  
   if (mode == EStreamMode.output) {
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
       
         if (CObject(event).obj_class() == &class(CMEControlChange) &&
             CMEControlChange(event)->control == MIDI_CONTROL_ACTIVE_A) {
            if (ARRAY(&this->dspOutput).count() > this->active_channel &&
                ARRAY(&this->dspFx).count() > this->active_channel) {
               kxapi_disconnect_microcode(this->kx_interface, 
                  ARRAY(&this->dspFx).data()[this->active_channel].pgm, ARRAY(&this->dspFx).data()[this->active_channel].reg);

               kxapi_connect_microcode(this->kx_interface,
                  ARRAY(&this->dspOutput).data()[this->active_channel].pgm, ARRAY(&this->dspOutput).data()[this->active_channel].reg,
                  ARRAY(&this->dspFx).data()[this->active_channel].pgm, ARRAY(&this->dspFx).data()[this->active_channel].reg
                  );
            }
            if (ARRAY(&this->dspOutput).count() > (CMidiEvent(event)->channel - 1) &&
                ARRAY(&this->dspFx).count()  > (CMidiEvent(event)->channel - 1)) {
              kxapi_disconnect_microcode(this->kx_interface, 
                 ARRAY(&this->dspFx).data()[CMidiEvent(event)->channel - 1].pgm, ARRAY(&this->dspFx).data()[CMidiEvent(event)->channel - 1].reg);
                 
              kxapi_connect_microcode(this->kx_interface,
                 this->input.pgm, this->input.reg,
                 ARRAY(&this->dspFx).data()[CMidiEvent(event)->channel - 1].pgm, ARRAY(&this->dspFx).data()[CMidiEvent(event)->channel - 1].reg
                 );
              this->active_channel = CMidiEvent(event)->channel - 1;
            }
         }              
      }
   }
   
   return TRUE;
}/*CDeviceMixerKx::*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
