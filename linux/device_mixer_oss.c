/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "../devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#include <sys/soundcard.h>
#include "../mixer.c"

class CDeviceMixerOSS : CDeviceMixer {
 private:
   TChannelMix channel[MIXER_CHANNELS];
   int active_channel;
   int handle;

   void new(void); 
   void delete(void);  
   void mixer_set_level(int level, int pan);
 public:
   ALIAS<"deviceMixerOSS">;
 
   void CDeviceMixerOSS(void);
 
   virtual EDeviceError open(void);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);  
};

void CDeviceMixerOSS::new(void) {
   _virtual_CDeviceHardware_new(CDeviceHardware(this)); 
}/*CDeviceMixerOSS::new*/

void CDeviceMixerOSS::delete(void) {
//   _virtual_CDeviceHardware___destroy(CDeviceHardware(this)); 
}/*CDeviceMixerOSS::delete*/

void CDeviceMixerOSS::mixer_set_level(int level, int pan) {
   int volume[2], write_vol;
	
   if (pan < 64) {
      volume[0] = (level * 100 / 127);
      volume[1] = ((level * 100 / 127) * pan / 64);
   }
   else {
      volume[0] = ((level * 100 / 127) * (127 - pan) / 64);
      volume[1] = (level * 100 / 127);
   }
   
   write_vol = volume[0] | (volume[1] << 8);
   ioctl(this->handle, MIXER_WRITE(SOUND_MIXER_LINE), &write_vol);
}/*CDeviceMixerOSS::mixer_set_level*/

EDeviceError CDeviceMixerOSS::open(TFrameInfo *info) {
   this->handle = open(CString(&CDeviceHardware(this)->device).string(), O_WRONLY | O_NONBLOCK);

   if (this->handle == -1) {
      return EDeviceError.deviceError;
   }	   

   return EDeviceError.noError;
}/*CDeviceMixerOSS::open*/

EDeviceError CDeviceMixerOSS::close(void) {
   close (this->handle);	
	
   return EDeviceError.noError;    
}/*CDeviceMixerOSS::close*/

bool CDeviceMixerOSS::frame(CFrame *frame, EStreamMode mode) {
   int i;
   CMidiEvent *event;

   if (mode == EStreamMode.output) {
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(event).obj_class() == &class(CMEControlChange)) {
            switch (CMEControlChange(event)->control) {
            case MIDI_CONTROL_ACTIVE_A:
               this->active_channel = CMidiEvent(event)->channel;
               CDeviceMixerOSS(this).mixer_set_level(this->channel[CMidiEvent(event)->channel].level,
                                                     this->channel[CMidiEvent(event)->channel].pan);
               break;
            case MIDI_CONTROL_LEVEL:
               this->channel[CMidiEvent(event)->channel].level = CMEControlChange(event)->value;
               if (this->active_channel == CMidiEvent(event)->channel) {
               CDeviceMixerOSS(this).mixer_set_level(this->channel[CMidiEvent(event)->channel].level,
                                                     this->channel[CMidiEvent(event)->channel].pan);
               }
               break;
            case MIDI_CONTROL_PAN:
               this->channel[CMidiEvent(event)->channel].pan = CMEControlChange(event)->value;
               if (this->active_channel == CMidiEvent(event)->channel) {
               CDeviceMixerOSS(this).mixer_set_level(this->channel[CMidiEvent(event)->channel].level,
                                                     this->channel[CMidiEvent(event)->channel].pan);
               }
               break;
            }
         }
      }
   }
    
   return TRUE;
}/*CDeviceMixerOSS::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
